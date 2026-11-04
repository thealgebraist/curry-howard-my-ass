#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <optional>
#include <vector>

using namespace lp;

enum class SelectedKind { CSSImage, Gradient, Unsupported };
struct SelectedOption {
  SelectedKind kind;
  unsigned image;
  unsigned resolution;
};
struct LoadResult {
  enum Kind { NullStyleImage, CachedCSSImage, GeneratedGradient } kind;
  unsigned image = 0;
  unsigned resolution = 0;
};
static LoadResult cpp_resolve_image_set(const std::optional<SelectedOption>& selected) {
  if (!selected) return {LoadResult::NullStyleImage};
  switch (selected->kind) {
    case SelectedKind::CSSImage:
      return {LoadResult::CachedCSSImage, selected->image, selected->resolution};
    case SelectedKind::Gradient:
      return {LoadResult::GeneratedGradient, selected->image, 0};
    case SelectedKind::Unsupported:
      return {LoadResult::NullStyleImage};
  }
  return {LoadResult::NullStyleImage};
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr apps(Ptr f, const std::vector<Ptr>& xs) {
  for (const auto& x : xs) f = app(f, x);
  return f;
}
static Ptr selected_term(const std::optional<SelectedOption>& selected) {
  if (!selected) return sym("noSelectedOption");
  const char* kind = selected->kind == SelectedKind::CSSImage ? "cssImageValue" :
      selected->kind == SelectedKind::Gradient ? "cssGradientValue" : "unsupportedCSSValue";
  return apps(sym("selectedOption"), {sym(kind), sym(("image" + std::to_string(selected->image)).c_str()),
                                       sym(("resolution" + std::to_string(selected->resolution)).c_str())});
}
static Ptr result_term(const LoadResult& result) {
  if (result.kind == LoadResult::NullStyleImage) return sym("nullStyleImage");
  if (result.kind == LoadResult::GeneratedGradient)
    return app(sym("generatedGradient"), sym(("image" + std::to_string(result.image)).c_str()));
  return apps(sym("cachedCSSImage"), {sym(("image" + std::to_string(result.image)).c_str()),
                                       sym(("resolution" + std::to_string(result.resolution)).c_str())});
}
int main() {
  const Ptr K = sym("ImageValueKind"), I = sym("ImageId");
  const Ptr R = sym("Resolution"), S = sym("SelectedOption"), O = sym("StyleImageResult");
  Signature signature;
  std::vector<Rule> rules;
  signature["ImageValueKind"] = type_sort();
  for (const char* kind : {"cssImageValue", "cssGradientValue", "unsupportedCSSValue"}) signature[kind] = K;
  signature["ImageId"] = type_sort();
  for (unsigned image = 0; image < 4; ++image) signature["image" + std::to_string(image)] = I;
  signature["Resolution"] = type_sort();
  for (unsigned resolution = 0; resolution < 4; ++resolution)
    signature["resolution" + std::to_string(resolution)] = R;
  signature["SelectedOption"] = type_sort(); signature["noSelectedOption"] = S;
  signature["selectedOption"] = fn(K, fn(I, fn(R, S)));
  signature["StyleImageResult"] = type_sort(); signature["nullStyleImage"] = O;
  signature["cachedCSSImage"] = fn(I, fn(R, O));
  signature["generatedGradient"] = fn(I, O);
  signature["resolveImageSetSelection"] = fn(S, O);

  rules.push_back(rule(app(sym("resolveImageSetSelection"), sym("noSelectedOption")),
                       sym("nullStyleImage")));
  rules.push_back(rule(app(sym("resolveImageSetSelection"),
      apps(sym("selectedOption"), {sym("cssImageValue"), pvar(0), pvar(1)})),
      apps(sym("cachedCSSImage"), {pvar(0), pvar(1)})));
  rules.push_back(rule(app(sym("resolveImageSetSelection"),
      apps(sym("selectedOption"), {sym("cssGradientValue"), pvar(2), pvar(3)})),
      app(sym("generatedGradient"), pvar(2))));
  rules.push_back(rule(app(sym("resolveImageSetSelection"),
      apps(sym("selectedOption"), {sym("unsupportedCSSValue"), pvar(4), pvar(5)})),
      sym("nullStyleImage")));

  RuleIndex indexed_rules(rules);
  std::optional<SelectedOption> sample = SelectedOption{SelectedKind::CSSImage, 0, 1};
  auto sample_term = app(sym("resolveImageSetSelection"), selected_term(sample));
  auto inferred = infer({}, sample_term, signature, rules);
  assert(inferred && conv(*inferred, O, indexed_rules));

  std::vector<std::optional<SelectedOption>> cases = {std::nullopt};
  for (unsigned image = 0; image < 2; ++image) for (unsigned resolution = 1; resolution <= 3; ++resolution) {
    cases.push_back(SelectedOption{SelectedKind::CSSImage, image, resolution});
    cases.push_back(SelectedOption{SelectedKind::Gradient, image, resolution});
  }
  cases.push_back(SelectedOption{SelectedKind::Unsupported, 2, 1});
  cases.push_back(SelectedOption{SelectedKind::Unsupported, 3, 3});
  unsigned states = 0;
  for (const auto& selected : cases) {
    const LoadResult expected = cpp_resolve_image_set(selected);
    const Ptr output = nf(app(sym("resolveImageSetSelection"), selected_term(selected)), indexed_rules);
    if (!conv(output, result_term(expected), indexed_rules)) {
      std::cerr << "StyleImageLoader ResolveImageSet dispatch mismatch\n";
      return 3;
    }
    ++states;
  }
  std::cout << "StyleImageLoader image-set null/image/gradient/rejection dispatch covers "
            << states << " finite selected-option states.\n";
  std::cout << "Scope: CSSImageValue cache behavior and CSSGradientValue container/viewport resolution are abstract outputs; only the fetched-image branch carries the selected resolution override.\n";
}
