#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <optional>
#include <vector>

using namespace lp;

struct ImageSetOption {
  unsigned image;
  unsigned resolution;
  std::optional<unsigned> type;
};
using ImageSetValue = std::vector<ImageSetOption>;
enum class StyleImageKind { ImageSet, Other };

static bool cpp_option_equal(const ImageSetOption& a, const ImageSetOption& b) {
  return a.image == b.image && a.resolution == b.resolution && a.type == b.type;
}
static bool cpp_image_set_equal(const ImageSetValue& a, const ImageSetValue& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (!cpp_option_equal(a[i], b[i])) return false;
  return true;
}
static bool cpp_style_image_set_equal(StyleImageKind right_kind, bool best_fit_equal,
                                      const ImageSetValue& a, const ImageSetValue& b) {
  return right_kind == StyleImageKind::ImageSet && best_fit_equal &&
         cpp_image_set_equal(a, b);
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr apps(Ptr f, const std::vector<Ptr>& xs) {
  for (const auto& x : xs) f = app(f, x);
  return f;
}
static Ptr bool_term(bool value) { return sym(value ? "true" : "false"); }
static Ptr option_term(const ImageSetOption& option) {
  return apps(sym("imageSetOption"), {
      sym(option.image == 0 ? "imageA" : "imageB"),
      sym(option.resolution == 0 ? "resolution1x" : "resolution2x"),
      !option.type ? sym("noType") : sym(*option.type == 0 ? "typeAvif" : "typeWebp")});
}
static Ptr image_set_term(const ImageSetValue& value) {
  Ptr list = sym("nilOptions");
  for (auto it = value.rbegin(); it != value.rend(); ++it)
    list = apps(sym("consOption"), {option_term(*it), list});
  return app(sym("cssImageSetValue"), list);
}

int main() {
  const Ptr B = sym("Bool"), I = sym("CSSImageValue"), R = sym("ResolutionValue");
  const Ptr T = sym("ImageSetType"), OT = sym("OptionalImageSetType");
  const Ptr O = sym("ImageSetOption"), L = sym("ImageSetOptionList");
  const Ptr V = sym("CSSImageSetValue"), K = sym("StyleImageKind");
  Signature signature;
  std::vector<Rule> rules;
  signature["Bool"] = type_sort(); signature["true"] = B; signature["false"] = B;
  signature["CSSImageValue"] = type_sort(); signature["imageA"] = I; signature["imageB"] = I;
  signature["ResolutionValue"] = type_sort(); signature["resolution1x"] = R; signature["resolution2x"] = R;
  signature["ImageSetType"] = type_sort(); signature["typeAvif"] = T; signature["typeWebp"] = T;
  signature["OptionalImageSetType"] = type_sort(); signature["noType"] = OT;
  signature["someType"] = fn(T, OT);
  signature["ImageSetOption"] = type_sort();
  signature["imageSetOption"] = fn(I, fn(R, fn(OT, O)));
  signature["ImageSetOptionList"] = type_sort(); signature["nilOptions"] = L;
  signature["consOption"] = fn(O, fn(L, L));
  signature["CSSImageSetValue"] = type_sort(); signature["cssImageSetValue"] = fn(L, V);
  signature["StyleImageKind"] = type_sort(); signature["imageSetImage"] = K; signature["otherImage"] = K;
  signature["andB"] = fn(B, fn(B, B)); signature["notB"] = fn(B, B);
  signature["imageValueEqual"] = fn(I, fn(I, B));
  signature["resolutionEqual"] = fn(R, fn(R, B));
  signature["imageSetTypeEqual"] = fn(T, fn(T, B));
  signature["optionalTypeEqual"] = fn(OT, fn(OT, B));
  signature["imageSetOptionEqual"] = fn(O, fn(O, B));
  signature["imageSetOptionsEqual"] = fn(L, fn(L, B));
  signature["cssImageSetValueEqual"] = fn(V, fn(V, B));
  signature["styleImageSetEqual"] = fn(K, fn(B, fn(V, fn(V, B))));

  for (bool a : {false, true}) for (bool b : {false, true})
    rules.push_back(rule(apps(sym("andB"), {bool_term(a), bool_term(b)}), bool_term(a && b)));
  for (const char* atom : {"imageA", "imageB"})
    rules.push_back(rule(apps(sym("imageValueEqual"), {sym(atom), sym(atom)}), sym("true")));
  rules.push_back(rule(apps(sym("imageValueEqual"), {pvar(0), pvar(1)}), sym("false")));
  for (const char* atom : {"resolution1x", "resolution2x"})
    rules.push_back(rule(apps(sym("resolutionEqual"), {sym(atom), sym(atom)}), sym("true")));
  rules.push_back(rule(apps(sym("resolutionEqual"), {pvar(2), pvar(3)}), sym("false")));
  for (const char* atom : {"typeAvif", "typeWebp"})
    rules.push_back(rule(apps(sym("imageSetTypeEqual"), {sym(atom), sym(atom)}), sym("true")));
  rules.push_back(rule(apps(sym("imageSetTypeEqual"), {pvar(4), pvar(5)}), sym("false")));
  rules.push_back(rule(apps(sym("optionalTypeEqual"), {sym("noType"), sym("noType")}), sym("true")));
  for (const char* a : {"typeAvif", "typeWebp"})
    for (const char* b : {"typeAvif", "typeWebp"})
      rules.push_back(rule(apps(sym("optionalTypeEqual"),
          {app(sym("someType"), sym(a)), app(sym("someType"), sym(b))}),
          apps(sym("imageSetTypeEqual"), {sym(a), sym(b)})));
  rules.push_back(rule(apps(sym("optionalTypeEqual"), {pvar(6), pvar(7)}), sym("false")));
  rules.push_back(rule(apps(sym("imageSetOptionEqual"),
      {apps(sym("imageSetOption"), {pvar(10), pvar(11), pvar(12)}),
       apps(sym("imageSetOption"), {pvar(13), pvar(14), pvar(15)})}),
      apps(sym("andB"), {apps(sym("imageValueEqual"), {pvar(10), pvar(13)}),
        apps(sym("andB"), {apps(sym("resolutionEqual"), {pvar(11), pvar(14)}),
                            apps(sym("optionalTypeEqual"), {pvar(12), pvar(15)})})})));
  rules.push_back(rule(apps(sym("imageSetOptionsEqual"), {sym("nilOptions"), sym("nilOptions")}), sym("true")));
  rules.push_back(rule(apps(sym("imageSetOptionsEqual"),
      {sym("nilOptions"), apps(sym("consOption"), {pvar(16), pvar(17)})}), sym("false")));
  rules.push_back(rule(apps(sym("imageSetOptionsEqual"),
      {apps(sym("consOption"), {pvar(16), pvar(17)}), sym("nilOptions")}), sym("false")));
  rules.push_back(rule(apps(sym("imageSetOptionsEqual"),
      {apps(sym("consOption"), {pvar(18), pvar(19)}),
       apps(sym("consOption"), {pvar(20), pvar(21)})}),
      apps(sym("andB"), {apps(sym("imageSetOptionEqual"), {pvar(18), pvar(20)}),
                          apps(sym("imageSetOptionsEqual"), {pvar(19), pvar(21)})})));
  rules.push_back(rule(apps(sym("cssImageSetValueEqual"),
      {app(sym("cssImageSetValue"), pvar(22)), app(sym("cssImageSetValue"), pvar(23))}),
      apps(sym("imageSetOptionsEqual"), {pvar(22), pvar(23)})));
  rules.push_back(rule(apps(sym("styleImageSetEqual"),
      {sym("imageSetImage"), pvar(24), pvar(25), pvar(26)}),
      apps(sym("andB"), {pvar(24), apps(sym("cssImageSetValueEqual"), {pvar(25), pvar(26)})})));
  rules.push_back(rule(apps(sym("styleImageSetEqual"),
      {sym("otherImage"), pvar(27), pvar(28), pvar(29)}), sym("false")));

  RuleIndex indexed_rules(rules);
  const ImageSetValue empty;
  auto sample = apps(sym("styleImageSetEqual"),
      {sym("imageSetImage"), sym("true"), image_set_term(empty), image_set_term(empty)});
  auto inferred = infer({}, sample, signature, rules);
  assert(inferred && conv(*inferred, B, indexed_rules));

  const std::vector<ImageSetValue> values = {
      {}, {{0, 0, std::nullopt}}, {{0, 0, 0}}, {{1, 1, 1}},
      {{0, 0, std::nullopt}, {1, 1, 1}}, {{1, 1, 1}, {0, 0, std::nullopt}}};
  unsigned list_pairs = 0, style_states = 0;
  for (const auto& left : values) for (const auto& right : values) {
    bool expected = cpp_image_set_equal(left, right);
    auto term = apps(sym("cssImageSetValueEqual"), {image_set_term(left), image_set_term(right)});
    if (!conv(nf(term, indexed_rules), bool_term(expected), indexed_rules)) {
      std::cerr << "CSSImageSetValue list equality mismatch\n";
      return 2;
    }
    ++list_pairs;
    for (StyleImageKind right_kind : {StyleImageKind::ImageSet, StyleImageKind::Other})
      for (bool best_fit_equal : {false, true}) {
        bool style_expected = cpp_style_image_set_equal(right_kind, best_fit_equal, left, right);
        auto style_term = apps(sym("styleImageSetEqual"),
          {sym(right_kind == StyleImageKind::ImageSet ? "imageSetImage" : "otherImage"),
           bool_term(best_fit_equal), image_set_term(left), image_set_term(right)});
        if (!conv(nf(style_term, indexed_rules), bool_term(style_expected), indexed_rules)) {
          std::cerr << "StyleImageSet equality composition mismatch\n";
          return 3;
        }
        ++style_states;
      }
  }
  std::cout << "CSSImageSetValue ordered option-list equality covers " << list_pairs
            << " finite value pairs.\n";
  std::cout << "StyleImageSet type, best-fit relation, and CSS-value equality cover "
            << style_states << " finite states.\n";
  std::cout << "Scope: option image/resolution/type values are finite atoms; lists are bounded to length two and separator kind is fixed to the source constructor's comma value.\n";
}
