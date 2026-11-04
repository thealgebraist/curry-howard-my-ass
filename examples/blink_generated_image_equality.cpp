#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace lp;

struct ContainerSizes {
  std::optional<unsigned> width;
  std::optional<unsigned> height;
};
using ViewportSize = std::array<unsigned, 6>;
struct CSSPaintValue {
  const char* name_constructor;
  const char* name;
  const char* argument_constructor;
  const char* argument;
};
struct GradientStop {
  const char* color;
  const char* offset;
};
struct GradientBase {
  bool repeating;
  const char* interpolation_space;
  const char* hue_method;
  std::vector<GradientStop> stops;
};

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr apps(Ptr f, const std::vector<Ptr>& args) {
  for (const auto& arg : args) f = app(f, arg);
  return f;
}
static Ptr bool_term(bool value) { return sym(value ? "true" : "false"); }
static Ptr normalize(Ptr term, const RuleIndex& rules) {
  for (unsigned i = 0; i < 16; ++i) term = nf(term, rules);
  return term;
}

static Ptr dimension_term(unsigned value) { return sym(value ? "d1" : "d0"); }
static Ptr optional_size_term(const std::optional<unsigned>& value) {
  return value ? app(sym("someSize"), dimension_term(*value)) : sym("noneSize");
}
static Ptr container_term(const ContainerSizes& sizes) {
  return apps(sym("containerSizes"),
               {optional_size_term(sizes.width), optional_size_term(sizes.height)});
}
static Ptr viewport_term(const ViewportSize& size) {
  std::vector<Ptr> dims;
  for (unsigned value : size) dims.push_back(dimension_term(value));
  return apps(sym("viewportSize"), dims);
}
static Ptr paint_value_term(const CSSPaintValue& value) {
  return apps(sym("cssPaintValue"),
              {sym(value.name_constructor), sym(value.argument_constructor)});
}
static std::string paint_custom_css_text(const CSSPaintValue& value) {
  std::string text = "paint(";
  text += value.name;
  if (value.argument) {
    text += ", ";
    text += value.argument;
  }
  text += ')';
  return text;
}
static Ptr gradient_stops_term(const std::vector<GradientStop>& stops) {
  Ptr result = sym("nilStops");
  for (auto it = stops.rbegin(); it != stops.rend(); ++it)
    result = apps(sym("consStop"),
                  {apps(sym("gradientStop"), {sym(it->color), sym(it->offset)}), result});
  return result;
}
static Ptr gradient_base_term(const GradientBase& value) {
  return apps(sym("gradientBase"),
              {bool_term(value.repeating), sym(value.interpolation_space),
               sym(value.hue_method), gradient_stops_term(value.stops)});
}
static Ptr generated_equal_term(const ContainerSizes& left_container,
                                const ContainerSizes& right_container,
                                const ViewportSize& left_viewport,
                                const ViewportSize& right_viewport,
                                bool generator_value_equivalent) {
  return apps(sym("generatedImageEqual"),
               {container_term(left_container), container_term(right_container),
                viewport_term(left_viewport), viewport_term(right_viewport),
                bool_term(generator_value_equivalent)});
}
static Ptr generated_paint_equal_term(const ContainerSizes& left_container,
                                     const ContainerSizes& right_container,
                                     const ViewportSize& left_viewport,
                                     const ViewportSize& right_viewport,
                                     const CSSPaintValue& left_paint,
                                     const CSSPaintValue& right_paint) {
  return apps(sym("generatedPaintImageEqual"),
              {container_term(left_container), container_term(right_container),
               viewport_term(left_viewport), viewport_term(right_viewport),
               paint_value_term(left_paint), paint_value_term(right_paint)});
}

int main() {
  Signature signature;
  std::vector<Rule> rules;
  const Ptr B = sym("Bool"), D = sym("Dimension"), O = sym("OptionalSize");
  const Ptr C = sym("ContainerSizes"), V = sym("ViewportSize");
  const Ptr P = sym("CSSPaintName"), A = sym("CSSPaintArgument");
  const Ptr Q = sym("CSSPaintValue"), T = sym("CSSPaintText");
  const Ptr G = sym("CSSGradientBase"), S = sym("GradientStop"), SL = sym("GradientStopList");
  const Ptr CS = sym("GradientColorSpace"), H = sym("HueInterpolationMethod"), Col = sym("GradientColor"), Off = sym("GradientOffset");
  signature["Bool"] = type_sort();
  signature["true"] = B;
  signature["false"] = B;
  signature["Dimension"] = type_sort();
  signature["d0"] = D;
  signature["d1"] = D;
  signature["OptionalSize"] = type_sort();
  signature["noneSize"] = O;
  signature["someSize"] = fn(D, O);
  signature["ContainerSizes"] = type_sort();
  signature["containerSizes"] = fn(O, fn(O, C));
  signature["ViewportSize"] = type_sort();
  Ptr viewport_constructor = D;
  for (unsigned i = 0; i < 6; ++i) viewport_constructor = fn(D, viewport_constructor);
  signature["viewportSize"] = viewport_constructor;
  signature["CSSPaintName"] = type_sort();
  signature["paintNameA"] = P;
  signature["paintNameB"] = P;
  signature["CSSPaintArgument"] = type_sort();
  signature["noPaintArgument"] = A;
  signature["paintArgumentRed"] = A;
  signature["paintArgumentBlue"] = A;
  signature["CSSPaintValue"] = type_sort();
  signature["cssPaintValue"] = fn(P, fn(A, Q));
  signature["CSSPaintText"] = type_sort();
  const char* paint_texts[]={"paintTextAlphaNone", "paintTextAlphaRed", "paintTextAlphaBlue",
                             "paintTextBetaNone", "paintTextBetaRed", "paintTextBetaBlue"};
  for (const char* text : paint_texts) signature[text] = T;
  signature["andB"] = fn(B, fn(B, B));
  signature["notB"] = fn(B, B);
  signature["dimensionEqual"] = fn(D, fn(D, B));
  signature["optionalSizeEqual"] = fn(O, fn(O, B));
  signature["containerSizesEqual"] = fn(C, fn(C, B));
  signature["viewportSizeEqual"] = fn(V, fn(V, B));
  signature["generatedImageEqual"] = fn(C, fn(C, fn(V, fn(V, fn(B, B)))));
  signature["generatedImageNeedsReattach"] = fn(C, fn(C, fn(V, fn(V, fn(B, B)))));
  signature["paintNameEqual"] = fn(P, fn(P, B));
  signature["paintTextEqual"] = fn(T, fn(T, B));
  signature["paintCustomCSSText"] = fn(P, fn(A, T));
  signature["paintValueEqual"] = fn(Q, fn(Q, B));
  signature["generatedPaintImageEqual"] = fn(C, fn(C, fn(V, fn(V, fn(Q, fn(Q, B))))));
  signature["generatedPaintImageNeedsReattach"] = fn(C, fn(C, fn(V, fn(V, fn(Q, fn(Q, B))))));
  signature["CSSGradientBase"] = type_sort();
  signature["CSSGradientRepeat"] = type_sort();
  signature["nonRepeating"] = sym("CSSGradientRepeat");
  signature["repeating"] = sym("CSSGradientRepeat");
  signature["GradientColorSpace"] = type_sort();
  for (const char* space : {"spaceNone", "spaceSRGB", "spaceLab"}) signature[space] = CS;
  signature["HueInterpolationMethod"] = type_sort();
  for (const char* hue : {"hueShorter", "hueLonger", "hueIncreasing", "hueDecreasing"}) signature[hue] = H;
  signature["GradientColor"] = type_sort();
  signature["colorRed"] = Col; signature["colorBlue"] = Col;
  signature["GradientOffset"] = type_sort();
  signature["offsetZero"] = Off; signature["offsetHalf"] = Off;
  signature["GradientStop"] = type_sort();
  signature["gradientStop"] = fn(Col, fn(Off, S));
  signature["GradientStopList"] = type_sort();
  signature["nilStops"] = SL;
  signature["consStop"] = fn(S, fn(SL, SL));
  signature["gradientBase"] = fn(B, fn(CS, fn(H, fn(SL, G))));
  signature["boolEqual"] = fn(B, fn(B, B));
  signature["gradientSpaceEqual"] = fn(CS, fn(CS, B));
  signature["hueMethodEqual"] = fn(H, fn(H, B));
  signature["gradientColorEqual"] = fn(Col, fn(Col, B));
  signature["gradientOffsetEqual"] = fn(Off, fn(Off, B));
  signature["gradientStopEqual"] = fn(S, fn(S, B));
  signature["gradientStopListEqual"] = fn(SL, fn(SL, B));
  signature["gradientBaseEqual"] = fn(G, fn(G, B));

  for (bool a : {false, true})
    for (bool b : {false, true})
      rules.push_back(rule(apps(sym("andB"), {bool_term(a), bool_term(b)}),
                           bool_term(a && b)));
  rules.push_back(rule(app(sym("notB"), sym("true")), sym("false")));
  rules.push_back(rule(app(sym("notB"), sym("false")), sym("true")));

  rules.push_back(rule(apps(sym("dimensionEqual"), {sym("d0"), sym("d0")} ), sym("true")));
  rules.push_back(rule(apps(sym("dimensionEqual"), {sym("d1"), sym("d1")} ), sym("true")));
  rules.push_back(rule(apps(sym("dimensionEqual"), {pvar(0), pvar(1)}), sym("false")));

  rules.push_back(rule(apps(sym("optionalSizeEqual"), {sym("noneSize"), sym("noneSize")} ), sym("true")));
  rules.push_back(rule(apps(sym("optionalSizeEqual"),
                             {sym("noneSize"), app(sym("someSize"), pvar(0))}), sym("false")));
  rules.push_back(rule(apps(sym("optionalSizeEqual"),
                             {app(sym("someSize"), pvar(0)), sym("noneSize")}), sym("false")));
  rules.push_back(rule(apps(sym("optionalSizeEqual"),
                             {app(sym("someSize"), pvar(0)), app(sym("someSize"), pvar(1))}),
                       apps(sym("dimensionEqual"), {pvar(0), pvar(1)})));

  rules.push_back(rule(apps(sym("containerSizesEqual"),
                             {apps(sym("containerSizes"), {pvar(0), pvar(1)}),
                              apps(sym("containerSizes"), {pvar(2), pvar(3)})}),
                       apps(sym("andB"),
                             {apps(sym("optionalSizeEqual"), {pvar(0), pvar(2)}),
                              apps(sym("optionalSizeEqual"), {pvar(1), pvar(3)})})));

  std::vector<Ptr> left_viewport, right_viewport, component_equalities;
  for (unsigned i = 0; i < 6; ++i) {
    left_viewport.push_back(pvar(10 + i));
    right_viewport.push_back(pvar(20 + i));
    component_equalities.push_back(apps(sym("dimensionEqual"),
                                          {pvar(10 + i), pvar(20 + i)}));
  }
  Ptr viewport_equal = component_equalities.back();
  for (int i = 4; i >= 0; --i)
    viewport_equal = apps(sym("andB"), {component_equalities[i], viewport_equal});
  rules.push_back(rule(apps(sym("viewportSizeEqual"),
                             {apps(sym("viewportSize"), left_viewport),
                              apps(sym("viewportSize"), right_viewport)}),
                       viewport_equal));

  std::vector<Ptr> generated_args={pvar(30), pvar(31), pvar(32), pvar(33), pvar(34)};
  Ptr generated_rhs = apps(sym("andB"),
                            {apps(sym("containerSizesEqual"), {pvar(30), pvar(31)}),
                             apps(sym("andB"),
                                   {apps(sym("viewportSizeEqual"), {pvar(32), pvar(33)}), pvar(34)})});
  rules.push_back(rule(apps(sym("generatedImageEqual"), generated_args), generated_rhs));
  rules.push_back(rule(apps(sym("generatedImageNeedsReattach"), generated_args),
                       app(sym("notB"), apps(sym("generatedImageEqual"), generated_args))));

  for (const char* name : {"paintNameA", "paintNameB"})
    for (const char* arg : {"noPaintArgument", "paintArgumentRed", "paintArgumentBlue"}) {
      const char* text = (std::string(name)=="paintNameA")
        ? (std::string(arg)=="noPaintArgument" ? paint_texts[0]
           : std::string(arg)=="paintArgumentRed" ? paint_texts[1] : paint_texts[2])
        : (std::string(arg)=="noPaintArgument" ? paint_texts[3]
           : std::string(arg)=="paintArgumentRed" ? paint_texts[4] : paint_texts[5]);
      rules.push_back(rule(apps(sym("paintCustomCSSText"), {sym(name),sym(arg)}), sym(text)));
    }
  for (const char* name : {"paintNameA", "paintNameB"})
    rules.push_back(rule(apps(sym("paintNameEqual"), {sym(name),sym(name)}), sym("true")));
  rules.push_back(rule(apps(sym("paintNameEqual"), {pvar(40),pvar(41)}), sym("false")));
  for (const char* text : paint_texts)
    rules.push_back(rule(apps(sym("paintTextEqual"), {sym(text),sym(text)}), sym("true")));
  rules.push_back(rule(apps(sym("paintTextEqual"), {pvar(42),pvar(43)}), sym("false")));
  rules.push_back(rule(apps(sym("paintValueEqual"),
                            {apps(sym("cssPaintValue"), {pvar(44),pvar(45)}),
                             apps(sym("cssPaintValue"), {pvar(46),pvar(47)})}),
                       apps(sym("andB"),
                         {apps(sym("paintNameEqual"), {pvar(44),pvar(46)}),
                          apps(sym("paintTextEqual"),
                            {apps(sym("paintCustomCSSText"), {pvar(44),pvar(45)}),
                             apps(sym("paintCustomCSSText"), {pvar(46),pvar(47)})})})));
  std::vector<Ptr> generated_paint_args={pvar(50),pvar(51),pvar(52),pvar(53),pvar(54),pvar(55)};
  Ptr generated_paint_rhs=apps(sym("andB"),
    {apps(sym("containerSizesEqual"), {pvar(50),pvar(51)}),
     apps(sym("andB"),
       {apps(sym("viewportSizeEqual"), {pvar(52),pvar(53)}),
        apps(sym("paintValueEqual"), {pvar(54),pvar(55)})})});
  rules.push_back(rule(apps(sym("generatedPaintImageEqual"), generated_paint_args), generated_paint_rhs));
  rules.push_back(rule(apps(sym("generatedPaintImageNeedsReattach"), generated_paint_args),
                       app(sym("notB"), apps(sym("generatedPaintImageEqual"), generated_paint_args))));

  rules.push_back(rule(apps(sym("boolEqual"), {sym("true"),sym("true")}), sym("true")));
  rules.push_back(rule(apps(sym("boolEqual"), {sym("false"),sym("false")}), sym("true")));
  rules.push_back(rule(apps(sym("boolEqual"), {pvar(60),pvar(61)}), sym("false")));
  auto add_finite_equality = [&](const char* function, const std::vector<const char*>& values) {
    for (const char* value : values)
      rules.push_back(rule(apps(sym(function), {sym(value),sym(value)}), sym("true")));
    rules.push_back(rule(apps(sym(function), {pvar(62),pvar(63)}), sym("false")));
  };
  add_finite_equality("gradientSpaceEqual", {"spaceNone","spaceSRGB","spaceLab"});
  add_finite_equality("hueMethodEqual", {"hueShorter","hueLonger","hueIncreasing","hueDecreasing"});
  add_finite_equality("gradientColorEqual", {"colorRed","colorBlue"});
  add_finite_equality("gradientOffsetEqual", {"offsetZero","offsetHalf"});
  rules.push_back(rule(apps(sym("gradientStopEqual"),
                            {apps(sym("gradientStop"), {pvar(64),pvar(65)}),
                             apps(sym("gradientStop"), {pvar(66),pvar(67)})}),
                       apps(sym("andB"),
                         {apps(sym("gradientColorEqual"), {pvar(64),pvar(66)}),
                          apps(sym("gradientOffsetEqual"), {pvar(65),pvar(67)})})));
  rules.push_back(rule(apps(sym("gradientStopListEqual"), {sym("nilStops"),sym("nilStops")}), sym("true")));
  rules.push_back(rule(apps(sym("gradientStopListEqual"),
                            {sym("nilStops"),apps(sym("consStop"),{pvar(68),pvar(69)})}), sym("false")));
  rules.push_back(rule(apps(sym("gradientStopListEqual"),
                            {apps(sym("consStop"),{pvar(68),pvar(69)}),sym("nilStops")}), sym("false")));
  rules.push_back(rule(apps(sym("gradientStopListEqual"),
                            {apps(sym("consStop"),{pvar(68),pvar(69)}),
                             apps(sym("consStop"),{pvar(70),pvar(71)})}),
                       apps(sym("andB"),
                         {apps(sym("gradientStopEqual"),{pvar(68),pvar(70)}),
                          apps(sym("gradientStopListEqual"),{pvar(69),pvar(71)})})));
  rules.push_back(rule(apps(sym("gradientBaseEqual"),
                            {apps(sym("gradientBase"),{pvar(72),pvar(73),pvar(74),pvar(75)}),
                             apps(sym("gradientBase"),{pvar(76),pvar(77),pvar(78),pvar(79)})}),
                       apps(sym("andB"),
                         {apps(sym("boolEqual"),{pvar(72),pvar(76)}),
                          apps(sym("andB"),
                            {apps(sym("gradientSpaceEqual"),{pvar(73),pvar(77)}),
                             apps(sym("andB"),
                               {apps(sym("hueMethodEqual"),{pvar(74),pvar(78)}),
                                apps(sym("gradientStopListEqual"),{pvar(75),pvar(79)})})})})));

  RuleIndex indexed_rules(rules);
  auto optional_type = infer({}, apps(sym("optionalSizeEqual"), {sym("noneSize"), sym("noneSize")}), signature, rules);
  assert(optional_type && conv(*optional_type, B, indexed_rules));
  auto generated_type = infer({}, generated_equal_term({std::nullopt, 0u}, {std::nullopt, 0u},
                                                        {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, true),
                              signature, rules);
  assert(generated_type && conv(*generated_type, B, indexed_rules));
  auto paint_type = infer({}, generated_paint_equal_term({std::nullopt,0u}, {std::nullopt,0u},
                                                          {0,0,0,0,0,0}, {0,0,0,0,0,0},
                                                          {"paintNameA","alpha","noPaintArgument",nullptr},
                                                          {"paintNameA","alpha","noPaintArgument",nullptr}),
                          signature, rules);
  assert(paint_type && conv(*paint_type, B, indexed_rules));
  auto gradient_type = infer({}, apps(sym("gradientBaseEqual"),
                                      {gradient_base_term({false,"spaceNone","hueShorter",{}}),
                                       gradient_base_term({false,"spaceNone","hueShorter",{}})}),
                             signature, rules);
  assert(gradient_type && conv(*gradient_type, B, indexed_rules));

  std::vector<ContainerSizes> containers;
  const std::vector<std::optional<unsigned>> optional_values={std::nullopt, 0u, 1u};
  for (const auto& width : optional_values)
    for (const auto& height : optional_values) containers.push_back({width, height});
  std::vector<ViewportSize> viewports;
  for (unsigned bits = 0; bits < (1u << 6); ++bits) {
    ViewportSize value{};
    for (unsigned i = 0; i < 6; ++i) value[i] = (bits >> i) & 1u;
    viewports.push_back(value);
  }

  unsigned image_pairs = 0, reattach_pairs = 0;
  for (const auto& left_container : containers)
    for (const auto& right_container : containers)
      for (const auto& left_viewport : viewports)
        for (const auto& right_viewport : viewports)
          for (bool generator_equal : {false, true}) {
            const bool container_equal = left_container.width == right_container.width &&
                                         left_container.height == right_container.height;
            const bool viewport_equal = left_viewport == right_viewport;
            const bool expected = container_equal && viewport_equal && generator_equal;
            auto equal_term = generated_equal_term(left_container, right_container,
                                                   left_viewport, right_viewport,
                                                   generator_equal);
            if (!conv(normalize(equal_term, indexed_rules), bool_term(expected), indexed_rules)) {
              std::cerr << "generated image equality mismatch\n";
              return 2;
            }
            ++image_pairs;
            Ptr reattach = apps(sym("generatedImageNeedsReattach"),
                                 {container_term(left_container), container_term(right_container),
                                  viewport_term(left_viewport), viewport_term(right_viewport),
                                  bool_term(generator_equal)});
            if (!conv(normalize(reattach, indexed_rules), bool_term(!expected), indexed_rules)) {
              std::cerr << "generated image reattach mismatch\n";
              return 3;
            }
            ++reattach_pairs;
          }

  std::cout << "StyleGeneratedImage equality compares C++ and typed lambda-Pi over "
            << image_pairs << " size/value states.\n";
  std::cout << "Generated-image inequality reattach composition covers " << reattach_pairs
            << " corresponding states.\n";
  std::cout << "Container dimensions use absent/0/1 values and six viewport dimensions use 0/1; "
               "CSSImageGeneratorValue equivalence is an input.\n";
  const std::vector<CSSPaintValue> paint_values={
    {"paintNameA","alpha","noPaintArgument",nullptr},
    {"paintNameA","alpha","paintArgumentRed","red"},
    {"paintNameA","alpha","paintArgumentBlue","blue"},
    {"paintNameB","beta","noPaintArgument",nullptr},
    {"paintNameB","beta","paintArgumentRed","red"},
    {"paintNameB","beta","paintArgumentBlue","blue"}
  };
  unsigned paint_value_pairs=0, paint_image_compositions=0;
  for (const auto& left_paint : paint_values) for (const auto& right_paint : paint_values) {
      const bool expected=std::string(left_paint.name)==right_paint.name &&
                          paint_custom_css_text(left_paint)==paint_custom_css_text(right_paint);
      Ptr value_equal=apps(sym("paintValueEqual"),
                           {paint_value_term(left_paint),paint_value_term(right_paint)});
      if (!conv(normalize(value_equal,indexed_rules),bool_term(expected),indexed_rules)) {
        std::cerr << "CSSPaintValue string equality mismatch\n";
        return 4;
      }
      ++paint_value_pairs;
      for (const auto& [left_container,right_container] :
           std::vector<std::pair<ContainerSizes,ContainerSizes>>{
             {{std::nullopt,0u},{std::nullopt,0u}}, {{0u,0u},{1u,0u}}})
        for (const auto& [left_viewport,right_viewport] :
             std::vector<std::pair<ViewportSize,ViewportSize>>{
               {{{0,0,0,0,0,0}},{{0,0,0,0,0,0}}},
               {{{0,0,0,0,0,0}},{{1,0,0,0,0,0}}}}) {
          const bool generated_expected = expected &&
            left_container.width==right_container.width && left_container.height==right_container.height &&
            left_viewport==right_viewport;
          auto term=generated_paint_equal_term(left_container,right_container,left_viewport,right_viewport,
                                               left_paint,right_paint);
          if (!conv(normalize(term,indexed_rules),bool_term(generated_expected),indexed_rules)) {
            std::cerr << "generated paint image equality mismatch\n";
            return 5;
          }
          Ptr reattach=apps(sym("generatedPaintImageNeedsReattach"),
                            {container_term(left_container),container_term(right_container),
                             viewport_term(left_viewport),viewport_term(right_viewport),
                             paint_value_term(left_paint),paint_value_term(right_paint)});
          if (!conv(normalize(reattach,indexed_rules),bool_term(!generated_expected),indexed_rules)) {
            std::cerr << "generated paint reattach mismatch\n";
            return 6;
          }
          ++paint_image_compositions;
        }
    }
  std::cout << "CSSPaintValue name/custom-CSS-text equality compares " << paint_value_pairs
            << " finite string-atom pairs; generated paint image composition covers "
            << paint_image_compositions << " size/value states.\n";

  std::vector<std::vector<GradientStop>> stop_lists(1);
  const std::vector<GradientStop> stop_domain={{"colorRed","offsetZero"},
                                                {"colorRed","offsetHalf"},
                                                {"colorBlue","offsetZero"},
                                                {"colorBlue","offsetHalf"}};
  for (const auto& stop : stop_domain) stop_lists.push_back({stop});
  for (const auto& first : stop_domain)
    for (const auto& second : stop_domain) stop_lists.push_back({first,second});
  std::vector<GradientBase> gradients;
  const char* spaces[]={"spaceNone","spaceSRGB","spaceLab"};
  const char* hues[]={"hueShorter","hueLonger","hueIncreasing","hueDecreasing"};
  for (bool repeating : {false,true})
    for (const char* space : spaces)
      for (const char* hue : hues)
        for (const auto& stops : stop_lists)
          gradients.push_back({repeating,space,hue,stops});
  unsigned gradient_pairs=0;
  for (const auto& left : gradients)
    for (const auto& right : gradients) {
      bool expected=left.repeating==right.repeating &&
                    std::string(left.interpolation_space)==right.interpolation_space &&
                    std::string(left.hue_method)==right.hue_method &&
                    left.stops.size()==right.stops.size();
      for (size_t i=0; expected && i<left.stops.size(); ++i)
        expected=std::string(left.stops[i].color)==right.stops[i].color &&
                 std::string(left.stops[i].offset)==right.stops[i].offset;
      Ptr term=apps(sym("gradientBaseEqual"),{gradient_base_term(left),gradient_base_term(right)});
      if (!conv(normalize(term,indexed_rules),bool_term(expected),indexed_rules)) {
        std::cerr << "CSSGradientValue base equality mismatch\n";
        return 7;
      }
      ++gradient_pairs;
    }
  std::cout << "CSSGradientValue common-field and bounded color-stop-list equality compares "
            << gradient_pairs << " finite gradient pairs.\n";
}
