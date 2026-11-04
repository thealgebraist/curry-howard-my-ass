#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

struct CrossfadeItem {
  unsigned image;
  unsigned percentage;
};
using CrossfadeValue = std::vector<CrossfadeItem>;

enum class StyleImageKind { Crossfade, Other };

static bool cpp_crossfade_equal(const CrossfadeValue& a, const CrossfadeValue& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (a[i].image != b[i].image || a[i].percentage != b[i].percentage) return false;
  return true;
}
static bool cpp_style_crossfade_equal(StyleImageKind other, const CrossfadeValue& a,
                                      const CrossfadeValue& b) {
  return other == StyleImageKind::Crossfade && cpp_crossfade_equal(a, b);
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr apps(Ptr f, const std::vector<Ptr>& args) {
  for (const auto& arg : args) f = app(f, arg);
  return f;
}
static Ptr item_term(const CrossfadeItem& item) {
  return apps(sym("crossfadeItem"),
              {sym(item.image == 0 ? "imageA" : "imageB"),
               sym(item.percentage == 0 ? "percentage0" : "percentage50")});
}
static Ptr crossfade_term(const CrossfadeValue& value) {
  Ptr result = sym("nilItems");
  for (auto it = value.rbegin(); it != value.rend(); ++it)
    result = apps(sym("consItem"), {item_term(*it), result});
  return app(sym("cssCrossfade"), result);
}
static Ptr bool_term(bool value) { return sym(value ? "true" : "false"); }

int main() {
  const Ptr B = sym("Bool"), I = sym("CSSImageValue"), P = sym("CSSPercentageValue");
  const Ptr X = sym("CrossfadeItem"), L = sym("CrossfadeItemList");
  const Ptr C = sym("CSSCrossfadeValue"), K = sym("StyleImageKind");
  Signature signature;
  std::vector<Rule> rules;
  signature["Bool"] = type_sort(); signature["true"] = B; signature["false"] = B;
  signature["ImageA"] = type_sort(); signature["imageA"] = I;
  signature["imageB"] = I;
  signature["Percentage"] = type_sort(); signature["percentage0"] = P;
  signature["percentage50"] = P;
  signature["CrossfadeItem"] = type_sort();
  signature["crossfadeItem"] = fn(I, fn(P, X));
  signature["CrossfadeItemList"] = type_sort();
  signature["nilItems"] = L; signature["consItem"] = fn(X, fn(L, L));
  signature["CSSCrossfadeValue"] = type_sort();
  signature["cssCrossfade"] = fn(L, C);
  signature["StyleImageKind"] = type_sort();
  signature["crossfadeImage"] = K; signature["otherImage"] = K;
  signature["boolEqual"] = fn(B, fn(B, B));
  signature["imageValueEqual"] = fn(I, fn(I, B));
  signature["percentageEqual"] = fn(P, fn(P, B));
  signature["crossfadeItemEqual"] = fn(X, fn(X, B));
  signature["crossfadeItemsEqual"] = fn(L, fn(L, B));
  signature["cssCrossfadeEqual"] = fn(C, fn(C, B));
  signature["styleCrossfadeEqual"] = fn(K, fn(C, fn(C, B)));
  signature["andB"] = fn(B, fn(B, B));

  for (bool a : {false, true}) for (bool b : {false, true})
    rules.push_back(rule(apps(sym("andB"), {bool_term(a), bool_term(b)}), bool_term(a && b)));
  for (const char* atom : {"true", "false"})
    rules.push_back(rule(apps(sym("boolEqual"), {sym(atom), sym(atom)}), sym("true")));
  rules.push_back(rule(apps(sym("boolEqual"), {pvar(0), pvar(1)}), sym("false")));
  for (const char* atom : {"imageA", "imageB"})
    rules.push_back(rule(apps(sym("imageValueEqual"), {sym(atom), sym(atom)}), sym("true")));
  rules.push_back(rule(apps(sym("imageValueEqual"), {pvar(2), pvar(3)}), sym("false")));
  for (const char* atom : {"percentage0", "percentage50"})
    rules.push_back(rule(apps(sym("percentageEqual"), {sym(atom), sym(atom)}), sym("true")));
  rules.push_back(rule(apps(sym("percentageEqual"), {pvar(4), pvar(5)}), sym("false")));

  rules.push_back(rule(apps(sym("crossfadeItemEqual"),
      {apps(sym("crossfadeItem"), {pvar(10), pvar(11)}),
       apps(sym("crossfadeItem"), {pvar(12), pvar(13)})}),
      apps(sym("andB"), {apps(sym("imageValueEqual"), {pvar(10), pvar(12)}),
                          apps(sym("percentageEqual"), {pvar(11), pvar(13)})})));
  rules.push_back(rule(apps(sym("crossfadeItemsEqual"), {sym("nilItems"), sym("nilItems")}), sym("true")));
  rules.push_back(rule(apps(sym("crossfadeItemsEqual"),
      {sym("nilItems"), apps(sym("consItem"), {pvar(14), pvar(15)})}), sym("false")));
  rules.push_back(rule(apps(sym("crossfadeItemsEqual"),
      {apps(sym("consItem"), {pvar(14), pvar(15)}), sym("nilItems")}), sym("false")));
  rules.push_back(rule(apps(sym("crossfadeItemsEqual"),
      {apps(sym("consItem"), {pvar(16), pvar(17)}),
       apps(sym("consItem"), {pvar(18), pvar(19)})}),
      apps(sym("andB"), {apps(sym("crossfadeItemEqual"), {pvar(16), pvar(18)}),
                          apps(sym("crossfadeItemsEqual"), {pvar(17), pvar(19)})})));
  rules.push_back(rule(apps(sym("cssCrossfadeEqual"),
      {app(sym("cssCrossfade"), pvar(20)), app(sym("cssCrossfade"), pvar(21))}),
      apps(sym("crossfadeItemsEqual"), {pvar(20), pvar(21)})));
  rules.push_back(rule(apps(sym("styleCrossfadeEqual"),
      {sym("crossfadeImage"), pvar(22), pvar(23)}),
      apps(sym("cssCrossfadeEqual"), {pvar(22), pvar(23)})));
  rules.push_back(rule(apps(sym("styleCrossfadeEqual"),
      {sym("otherImage"), pvar(24), pvar(25)}), sym("false")));

  RuleIndex indexed_rules(rules);
  const auto empty = crossfade_term({});
  auto inferred = infer({}, apps(sym("styleCrossfadeEqual"),
      {sym("crossfadeImage"), empty, empty}), signature, rules);
  assert(inferred && conv(*inferred, B, indexed_rules));

  std::vector<CrossfadeValue> values = {{}, {{0, 0}}, {{0, 0}, {1, 1}},
                                        {{1, 1}, {0, 0}}, {{0, 1}}};
  unsigned css_pairs = 0, style_pairs = 0;
  for (const auto& left : values) for (const auto& right : values) {
    const bool expected = cpp_crossfade_equal(left, right);
    auto term = apps(sym("cssCrossfadeEqual"), {crossfade_term(left), crossfade_term(right)});
    if (!conv(nf(term, indexed_rules), bool_term(expected), indexed_rules)) {
      std::cerr << "CSSCrossfadeValue equality mismatch\n";
      return 2;
    }
    ++css_pairs;
    for (StyleImageKind kind : {StyleImageKind::Crossfade, StyleImageKind::Other}) {
      const bool style_expected = cpp_style_crossfade_equal(kind, left, right);
      auto style_term = apps(sym("styleCrossfadeEqual"),
          {sym(kind == StyleImageKind::Crossfade ? "crossfadeImage" : "otherImage"),
           crossfade_term(left), crossfade_term(right)});
      if (!conv(nf(style_term, indexed_rules), bool_term(style_expected), indexed_rules)) {
        std::cerr << "StyleCrossfadeImage equality mismatch\n";
        return 3;
      }
      ++style_pairs;
    }
  }
  std::cout << "CSSCrossfadeValue pair-list equality covers " << css_pairs
            << " finite ordered value pairs.\n";
  std::cout << "StyleCrossfadeImage type check plus original-value equality covers "
            << style_pairs << " finite states.\n";
  std::cout << "Scope: image and percentage CSSValue relations use finite atoms; list fixtures cover lengths zero through two.\n";
}
