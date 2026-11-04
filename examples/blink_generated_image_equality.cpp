#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
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
struct LinearGradient {
  const char* type;
  GradientBase base;
  const char* first_x;
  const char* first_y;
  const char* second_x;
  const char* second_y;
  const char* angle;
};
struct RadialGradient {
  const char* type;
  GradientBase base;
  const char* first_x;
  const char* first_y;
  const char* second_x;
  const char* second_y;
  const char* first_radius;
  const char* second_radius;
  const char* shape;
  const char* sizing_behavior;
  const char* end_horizontal_size;
  const char* end_vertical_size;
};
struct ConicGradient {
  GradientBase base;
  const char* x;
  const char* y;
  const char* from_angle;
};
struct CSSColorValue {
  unsigned color_space;
  std::array<bool,4> none_flags;
  std::array<float,4> components;
};
enum class GeneratorKind { Paint, Linear, Radial, Conic, Constant, ColorImage };
using GeneratorPayload=std::variant<CSSPaintValue,LinearGradient,RadialGradient,ConicGradient,CSSColorValue>;
struct GeneratorValue {
  GeneratorKind kind;
  GeneratorPayload payload;
  Ptr term;
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
static Ptr optional_value_term(const char* value) {
  return std::string(value)=="noneValue" ? sym("noneValue") : app(sym("someValue"),sym(value));
}
static Ptr linear_gradient_term(const LinearGradient& value) {
  return apps(sym("linearGradient"),
              {sym(value.type),gradient_base_term(value.base),optional_value_term(value.first_x),
               optional_value_term(value.first_y),optional_value_term(value.second_x),
               optional_value_term(value.second_y),optional_value_term(value.angle)});
}
static Ptr optional_identifier_term(const char* value) {
  return std::string(value)=="noneIdentifier" ? sym("noneIdentifier")
    : app(sym("someIdentifier"),sym(value));
}
static Ptr radial_gradient_term(const RadialGradient& value) {
  return apps(sym("radialGradient"),
    {sym(value.type),gradient_base_term(value.base),optional_value_term(value.first_x),
     optional_value_term(value.first_y),optional_value_term(value.second_x),
     optional_value_term(value.second_y),optional_value_term(value.first_radius),
     optional_value_term(value.second_radius),optional_identifier_term(value.shape),
     optional_identifier_term(value.sizing_behavior),optional_value_term(value.end_horizontal_size),
     optional_value_term(value.end_vertical_size)});
}
static Ptr conic_gradient_term(const ConicGradient& value) {
  return apps(sym("conicGradient"),
    {gradient_base_term(value.base),optional_value_term(value.x),optional_value_term(value.y),
     optional_value_term(value.from_angle)});
}
static Ptr css_color_term(const CSSColorValue& color) {
  std::vector<Ptr> fields={sym(color.color_space==0?"cssSpaceSRGBLegacy":"cssSpaceDisplayP3")};
  for(bool flag:color.none_flags)fields.push_back(bool_term(flag));
  for(float component:color.components)fields.push_back(sym(component==0.f?"cssComponent0":"cssComponent1"));
  return apps(sym("cssColor"),fields);
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
  const Ptr ColorSpace=sym("CSSColorSpace"), ColorComponent=sym("CSSColorComponent"), Color=sym("CSSColorValue");
  signature["CSSColorSpace"]=type_sort();
  signature["cssSpaceSRGBLegacy"]=ColorSpace; signature["cssSpaceDisplayP3"]=ColorSpace;
  signature["CSSColorComponent"]=type_sort();
  signature["cssComponent0"]=ColorComponent; signature["cssComponent1"]=ColorComponent;
  signature["CSSColorValue"]=type_sort();
  Ptr color_ctor=Color; for(unsigned i=0;i<4;++i)color_ctor=fn(ColorComponent,color_ctor);
  for(unsigned i=0;i<4;++i)color_ctor=fn(B,color_ctor); color_ctor=fn(ColorSpace,color_ctor);
  signature["cssColor"]=color_ctor;
  signature["cssColorSpaceEqual"]=fn(ColorSpace,fn(ColorSpace,B));
  signature["cssColorComponentEqual"]=fn(ColorComponent,fn(ColorComponent,B));
  signature["cssColorValueEqual"]=fn(Color,fn(Color,B));
  signature["gradientOffsetEqual"] = fn(Off, fn(Off, B));
  signature["gradientStopEqual"] = fn(S, fn(S, B));
  signature["gradientStopListEqual"] = fn(SL, fn(SL, B));
  signature["gradientBaseEqual"] = fn(G, fn(G, B));
  signature["GradientValueAtom"] = type_sort();
  for (const char* atom : {"coordA","coordB","angleA","angleB","radiusA","radiusB","sizeA","sizeB"}) signature[atom] = sym("GradientValueAtom");
  signature["OptionalCSSValue"] = type_sort();
  signature["noneValue"] = sym("OptionalCSSValue");
  signature["someValue"] = fn(sym("GradientValueAtom"),sym("OptionalCSSValue"));
  signature["LinearGradientType"] = type_sort();
  for (const char* type : {"deprecatedLinear","prefixedLinear","standardLinear"}) signature[type] = sym("LinearGradientType");
  signature["linearGradient"] = fn(sym("LinearGradientType"),fn(G,fn(sym("OptionalCSSValue"),fn(sym("OptionalCSSValue"),fn(sym("OptionalCSSValue"),fn(sym("OptionalCSSValue"),fn(sym("OptionalCSSValue"),sym("LinearGradient"))))))));
  signature["LinearGradient"] = type_sort();
  signature["gradientValueAtomEqual"] = fn(sym("GradientValueAtom"),fn(sym("GradientValueAtom"),B));
  signature["optionalCSSValueEqual"] = fn(sym("OptionalCSSValue"),fn(sym("OptionalCSSValue"),B));
  signature["gradientStopsEqual"] = fn(G,fn(G,B));
  signature["linearGradientEqual"] = fn(sym("LinearGradient"),fn(sym("LinearGradient"),B));
  signature["IdentifierAtom"] = type_sort();
  for (const char* atom : {"shapeEllipse","shapeCircle","sizingFar","sizingClose"}) signature[atom] = sym("IdentifierAtom");
  signature["OptionalIdentifier"] = type_sort();
  signature["noneIdentifier"] = sym("OptionalIdentifier");
  signature["someIdentifier"] = fn(sym("IdentifierAtom"),sym("OptionalIdentifier"));
  signature["identifierEqual"] = fn(sym("IdentifierAtom"),fn(sym("IdentifierAtom"),B));
  signature["canonicalIdentifier"] = fn(sym("OptionalIdentifier"),fn(sym("IdentifierAtom"),sym("IdentifierAtom")));
  signature["identifierEqualWithDefault"] = fn(sym("OptionalIdentifier"),fn(sym("OptionalIdentifier"),fn(sym("IdentifierAtom"),B)));
  signature["RadialGradientType"] = type_sort();
  for (const char* type : {"deprecatedRadial","prefixedRadial","standardRadial"}) signature[type] = sym("RadialGradientType");
  signature["RadialGradient"] = type_sort();
  auto curried_type=[](Ptr result,std::vector<Ptr> domains){
    for(auto it=domains.rbegin();it!=domains.rend();++it) result=fn(*it,result);
    return result;
  };
  signature["radialGradient"] = curried_type(sym("RadialGradient"),
    {sym("RadialGradientType"),G,sym("OptionalCSSValue"),sym("OptionalCSSValue"),
     sym("OptionalCSSValue"),sym("OptionalCSSValue"),sym("OptionalCSSValue"),
     sym("OptionalCSSValue"),sym("OptionalIdentifier"),sym("OptionalIdentifier"),
     sym("OptionalCSSValue"),sym("OptionalCSSValue")});
  signature["radialGradientEqual"] = fn(sym("RadialGradient"),fn(sym("RadialGradient"),B));
  signature["ConicGradient"] = type_sort();
  signature["conicGradient"] = curried_type(sym("ConicGradient"),
    {G,sym("OptionalCSSValue"),sym("OptionalCSSValue"),sym("OptionalCSSValue")});
  signature["conicGradientEqual"] = fn(sym("ConicGradient"),fn(sym("ConicGradient"),B));
  signature["constantGradientEqual"] = fn(Color,fn(Color,B));
  signature["colorImageEqual"] = fn(Color,fn(Color,B));
  signature["GeneratorValue"] = type_sort();
  signature["paintGenerator"] = fn(Q,sym("GeneratorValue"));
  signature["linearGenerator"] = fn(sym("LinearGradient"),sym("GeneratorValue"));
  signature["radialGenerator"] = fn(sym("RadialGradient"),sym("GeneratorValue"));
  signature["conicGenerator"] = fn(sym("ConicGradient"),sym("GeneratorValue"));
  signature["constantGenerator"] = fn(Color,sym("GeneratorValue"));
  signature["colorImageGenerator"] = fn(Color,sym("GeneratorValue"));
  signature["generatorValueEqual"] = fn(sym("GeneratorValue"),fn(sym("GeneratorValue"),B));
  signature["generatedImageValuesEqual"] = curried_type(B,
    {C,C,V,V,sym("GeneratorValue"),sym("GeneratorValue")});
  signature["generatedImageValuesChanged"] = curried_type(B,
    {C,C,V,V,sym("GeneratorValue"),sym("GeneratorValue")});

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

  for (const char* atom : {"coordA","coordB","angleA","angleB","radiusA","radiusB","sizeA","sizeB"})
    rules.push_back(rule(apps(sym("gradientValueAtomEqual"),{sym(atom),sym(atom)}),sym("true")));
  rules.push_back(rule(apps(sym("gradientValueAtomEqual"),{pvar(80),pvar(81)}),sym("false")));
  rules.push_back(rule(apps(sym("optionalCSSValueEqual"),{sym("noneValue"),sym("noneValue")}),sym("true")));
  rules.push_back(rule(apps(sym("optionalCSSValueEqual"),
                            {sym("noneValue"),app(sym("someValue"),pvar(82))}),sym("false")));
  rules.push_back(rule(apps(sym("optionalCSSValueEqual"),
                            {app(sym("someValue"),pvar(82)),sym("noneValue")}),sym("false")));
  rules.push_back(rule(apps(sym("optionalCSSValueEqual"),
                            {app(sym("someValue"),pvar(82)),app(sym("someValue"),pvar(83))}),
                       apps(sym("gradientValueAtomEqual"),{pvar(82),pvar(83)})));
  rules.push_back(rule(apps(sym("gradientStopsEqual"),
                            {apps(sym("gradientBase"),{pvar(84),pvar(85),pvar(86),pvar(87)}),
                             apps(sym("gradientBase"),{pvar(88),pvar(89),pvar(90),pvar(91)})}),
                       apps(sym("gradientStopListEqual"),{pvar(87),pvar(91)})));
  auto gradient_and = [](std::vector<Ptr> terms) {
    Ptr result=terms.back();
    for (int i=static_cast<int>(terms.size())-2;i>=0;--i)
      result=apps(sym("andB"),{terms[i],result});
    return result;
  };
  std::vector<Ptr> deprecated_left={sym("deprecatedLinear"),pvar(100),pvar(101),pvar(102),pvar(103),pvar(104),pvar(105)};
  std::vector<Ptr> deprecated_right={sym("deprecatedLinear"),pvar(110),pvar(111),pvar(112),pvar(113),pvar(114),pvar(115)};
  Ptr deprecated_result=gradient_and({
    apps(sym("optionalCSSValueEqual"),{pvar(101),pvar(111)}),
    apps(sym("optionalCSSValueEqual"),{pvar(102),pvar(112)}),
    apps(sym("optionalCSSValueEqual"),{pvar(103),pvar(113)}),
    apps(sym("optionalCSSValueEqual"),{pvar(104),pvar(114)}),
    apps(sym("gradientStopsEqual"),{pvar(100),pvar(110)})});
  rules.push_back(rule(apps(sym("linearGradientEqual"),
                            {apps(sym("linearGradient"),deprecated_left),
                             apps(sym("linearGradient"),deprecated_right)}),deprecated_result));
  for (const char* type : {"prefixedLinear","standardLinear"}) {
    std::vector<Ptr> left={sym(type),pvar(120),pvar(121),pvar(122),pvar(123),pvar(124),pvar(125)};
    std::vector<Ptr> right={sym(type),pvar(130),pvar(131),pvar(132),pvar(133),pvar(134),pvar(135)};
    Ptr base_equal=apps(sym("gradientBaseEqual"),{pvar(120),pvar(130)});
    Ptr angle_some=app(sym("someValue"),pvar(136));
    auto left_angle_case=left;left_angle_case[6]=angle_some;
    Ptr angle_result=gradient_and({base_equal,
      apps(sym("optionalCSSValueEqual"),{angle_some,pvar(135)}),
      apps(sym("gradientStopsEqual"),{pvar(120),pvar(130)})});
    rules.push_back(rule(apps(sym("linearGradientEqual"),
                              {apps(sym("linearGradient"),left_angle_case),
                               apps(sym("linearGradient"),right)}),angle_result));
    auto left_no_angle=left;left_no_angle[6]=sym("noneValue");
    auto right_has_angle=right;right_has_angle[6]=app(sym("someValue"),pvar(137));
    rules.push_back(rule(apps(sym("linearGradientEqual"),
                              {apps(sym("linearGradient"),left_no_angle),
                               apps(sym("linearGradient"),right_has_angle)}),sym("false")));
    auto right_no_angle=right;right_no_angle[6]=sym("noneValue");
    Ptr no_angle_result=gradient_and({base_equal,
      apps(sym("optionalCSSValueEqual"),{pvar(122),pvar(132)}),
      apps(sym("optionalCSSValueEqual"),{pvar(123),pvar(133)})});
    rules.push_back(rule(apps(sym("linearGradientEqual"),
                              {apps(sym("linearGradient"),left_no_angle),
                               apps(sym("linearGradient"),right_no_angle)}),no_angle_result));
  }
  rules.push_back(rule(apps(sym("linearGradientEqual"),{pvar(140),pvar(141)}),sym("false")));

  rules.push_back(rule(apps(sym("identifierEqual"),{sym("shapeEllipse"),sym("shapeEllipse")}),sym("true")));
  rules.push_back(rule(apps(sym("identifierEqual"),{sym("shapeCircle"),sym("shapeCircle")}),sym("true")));
  rules.push_back(rule(apps(sym("identifierEqual"),{sym("sizingFar"),sym("sizingFar")}),sym("true")));
  rules.push_back(rule(apps(sym("identifierEqual"),{sym("sizingClose"),sym("sizingClose")}),sym("true")));
  rules.push_back(rule(apps(sym("identifierEqual"),{pvar(142),pvar(143)}),sym("false")));
  rules.push_back(rule(apps(sym("canonicalIdentifier"),{sym("noneIdentifier"),pvar(144)}),pvar(144)));
  rules.push_back(rule(apps(sym("canonicalIdentifier"),
                            {app(sym("someIdentifier"),pvar(145)),pvar(146)}),pvar(145)));
  rules.push_back(rule(apps(sym("identifierEqualWithDefault"),{pvar(147),pvar(148),pvar(149)}),
    apps(sym("identifierEqual"),
      {apps(sym("canonicalIdentifier"),{pvar(147),pvar(149)}),
       apps(sym("canonicalIdentifier"),{pvar(148),pvar(149)})})));

  std::vector<Ptr> deprecated_radial_left={sym("deprecatedRadial"),pvar(150),pvar(151),pvar(152),
    pvar(153),pvar(154),pvar(155),pvar(156),pvar(157),pvar(158),pvar(159),pvar(160)};
  std::vector<Ptr> deprecated_radial_right={sym("deprecatedRadial"),pvar(170),pvar(171),pvar(172),
    pvar(173),pvar(174),pvar(175),pvar(176),pvar(177),pvar(178),pvar(179),pvar(180)};
  Ptr deprecated_radial_result=gradient_and({
    apps(sym("optionalCSSValueEqual"),{pvar(151),pvar(171)}),
    apps(sym("optionalCSSValueEqual"),{pvar(152),pvar(172)}),
    apps(sym("optionalCSSValueEqual"),{pvar(153),pvar(173)}),
    apps(sym("optionalCSSValueEqual"),{pvar(154),pvar(174)}),
    apps(sym("optionalCSSValueEqual"),{pvar(155),pvar(175)}),
    apps(sym("optionalCSSValueEqual"),{pvar(156),pvar(176)}),
    apps(sym("gradientStopsEqual"),{pvar(150),pvar(170)})});
  rules.push_back(rule(apps(sym("radialGradientEqual"),
                            {apps(sym("radialGradient"),deprecated_radial_left),
                             apps(sym("radialGradient"),deprecated_radial_right)}),deprecated_radial_result));
  for (const char* type : {"prefixedRadial","standardRadial"}) {
    std::vector<Ptr> left={sym(type),pvar(200),pvar(201),pvar(202),pvar(203),pvar(204),
      pvar(205),pvar(206),pvar(207),pvar(208),pvar(209),pvar(210)};
    std::vector<Ptr> right={pvar(220),pvar(221),pvar(222),pvar(223),pvar(224),pvar(225),
      pvar(226),pvar(227),pvar(228),pvar(229),pvar(230),pvar(231)};
    Ptr base_and_position=gradient_and({
      apps(sym("gradientBaseEqual"),{pvar(200),pvar(221)}),
      apps(sym("optionalCSSValueEqual"),{pvar(201),pvar(222)}),
      apps(sym("optionalCSSValueEqual"),{pvar(202),pvar(223)})});
    auto left_explicit=left;left_explicit[10]=app(sym("someValue"),pvar(232));
    Ptr explicit_result=gradient_and({base_and_position,
      apps(sym("optionalCSSValueEqual"),{app(sym("someValue"),pvar(232)),pvar(230)}),
      apps(sym("optionalCSSValueEqual"),{pvar(210),pvar(231)})});
    rules.push_back(rule(apps(sym("radialGradientEqual"),
                              {apps(sym("radialGradient"),left_explicit),
                               apps(sym("radialGradient"),right)}),explicit_result));
    auto left_no_horizontal=left;left_no_horizontal[10]=sym("noneValue");
    auto right_explicit=right;right_explicit[10]=app(sym("someValue"),pvar(233));
    rules.push_back(rule(apps(sym("radialGradientEqual"),
                              {apps(sym("radialGradient"),left_no_horizontal),
                               apps(sym("radialGradient"),right_explicit)}),sym("false")));
    auto right_no_horizontal=right;right_no_horizontal[10]=sym("noneValue");
    Ptr keyword_result=gradient_and({base_and_position,
      apps(sym("identifierEqualWithDefault"),{pvar(208),pvar(229),sym("sizingFar")}),
      apps(sym("identifierEqualWithDefault"),{pvar(207),pvar(228),sym("shapeEllipse")})});
    rules.push_back(rule(apps(sym("radialGradientEqual"),
                              {apps(sym("radialGradient"),left_no_horizontal),
                               apps(sym("radialGradient"),right_no_horizontal)}),keyword_result));
  }
  rules.push_back(rule(apps(sym("radialGradientEqual"),{pvar(240),pvar(241)}),sym("false")));
  rules.push_back(rule(apps(sym("conicGradientEqual"),
    {apps(sym("conicGradient"),{pvar(250),pvar(251),pvar(252),pvar(253)}),
     apps(sym("conicGradient"),{pvar(260),pvar(261),pvar(262),pvar(263)})}),
    gradient_and({apps(sym("gradientBaseEqual"),{pvar(250),pvar(260)}),
      apps(sym("optionalCSSValueEqual"),{pvar(251),pvar(261)}),
      apps(sym("optionalCSSValueEqual"),{pvar(252),pvar(262)}),
      apps(sym("optionalCSSValueEqual"),{pvar(253),pvar(263)})})));
  for(const char* color_space:{"cssSpaceSRGBLegacy","cssSpaceDisplayP3"})
    rules.push_back(rule(apps(sym("cssColorSpaceEqual"),{sym(color_space),sym(color_space)}),sym("true")));
  rules.push_back(rule(apps(sym("cssColorSpaceEqual"),{pvar(2600),pvar(2601)}),sym("false")));
  for(const char* component:{"cssComponent0","cssComponent1"})
    rules.push_back(rule(apps(sym("cssColorComponentEqual"),{sym(component),sym(component)}),sym("true")));
  rules.push_back(rule(apps(sym("cssColorComponentEqual"),{pvar(2602),pvar(2603)}),sym("false")));
  std::vector<Ptr> left_color_fields={pvar(2610)},right_color_fields={pvar(2620)};
  std::vector<Ptr> color_equalities={apps(sym("cssColorSpaceEqual"),{pvar(2610),pvar(2620)})};
  for(unsigned i=0;i<4;++i){left_color_fields.push_back(pvar(2611+i));right_color_fields.push_back(pvar(2621+i));color_equalities.push_back(apps(sym("boolEqual"),{pvar(2611+i),pvar(2621+i)}));}
  for(unsigned i=0;i<4;++i){left_color_fields.push_back(pvar(2615+i));right_color_fields.push_back(pvar(2625+i));color_equalities.push_back(apps(sym("cssColorComponentEqual"),{pvar(2615+i),pvar(2625+i)}));}
  rules.push_back(rule(apps(sym("cssColorValueEqual"),{apps(sym("cssColor"),left_color_fields),apps(sym("cssColor"),right_color_fields)}),gradient_and(color_equalities)));
  rules.push_back(rule(apps(sym("constantGradientEqual"),{pvar(270),pvar(271)}),
                       apps(sym("cssColorValueEqual"),{pvar(270),pvar(271)})));
  rules.push_back(rule(apps(sym("colorImageEqual"),{pvar(272),pvar(273)}),
                       apps(sym("constantGradientEqual"),{pvar(272),pvar(273)})));
  for (const auto& [tag,equality] : std::vector<std::pair<const char*,const char*>>{
         {"paintGenerator","paintValueEqual"},{"linearGenerator","linearGradientEqual"},
         {"radialGenerator","radialGradientEqual"},{"conicGenerator","conicGradientEqual"},
         {"constantGenerator","constantGradientEqual"},{"colorImageGenerator","colorImageEqual"}})
    rules.push_back(rule(apps(sym("generatorValueEqual"),
                              {app(sym(tag),pvar(280)),app(sym(tag),pvar(281))}),
                         apps(sym(equality),{pvar(280),pvar(281)})));
  rules.push_back(rule(apps(sym("generatorValueEqual"),{pvar(282),pvar(283)}),sym("false")));
  std::vector<Ptr> generated_values_args={pvar(290),pvar(291),pvar(292),pvar(293),pvar(294),pvar(295)};
  Ptr generated_values_equal=gradient_and({
    apps(sym("containerSizesEqual"),{pvar(290),pvar(291)}),
    apps(sym("viewportSizeEqual"),{pvar(292),pvar(293)}),
    apps(sym("generatorValueEqual"),{pvar(294),pvar(295)})});
  rules.push_back(rule(apps(sym("generatedImageValuesEqual"),generated_values_args),generated_values_equal));
  rules.push_back(rule(apps(sym("generatedImageValuesChanged"),generated_values_args),
                       app(sym("notB"),apps(sym("generatedImageValuesEqual"),generated_values_args))));

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
  auto linear_type = infer({}, apps(sym("linearGradientEqual"),
                                    {linear_gradient_term({"standardLinear",{false,"spaceNone","hueShorter",{}},
                                                           "noneValue","noneValue","noneValue","noneValue","noneValue"}),
                                     linear_gradient_term({"standardLinear",{false,"spaceNone","hueShorter",{}},
                                                           "noneValue","noneValue","noneValue","noneValue","noneValue"})}),
                           signature, rules);
  assert(linear_type && conv(*linear_type, B, indexed_rules));
  auto radial_type = infer({}, apps(sym("radialGradientEqual"),
                                    {radial_gradient_term({"prefixedRadial",{false,"spaceNone","hueShorter",{}},
                                      "noneValue","noneValue","noneValue","noneValue","noneValue","noneValue",
                                      "noneIdentifier","noneIdentifier","noneValue","noneValue"}),
                                     radial_gradient_term({"prefixedRadial",{false,"spaceNone","hueShorter",{}},
                                      "noneValue","noneValue","noneValue","noneValue","noneValue","noneValue",
                                      "noneIdentifier","noneIdentifier","noneValue","noneValue"})}),
                           signature, rules);
  assert(radial_type && conv(*radial_type, B, indexed_rules));
  auto conic_type = infer({}, apps(sym("conicGradientEqual"),
                                   {conic_gradient_term({{false,"spaceNone","hueShorter",{}},
                                                        "noneValue","noneValue","noneValue"}),
                                    conic_gradient_term({{false,"spaceNone","hueShorter",{}},
                                                        "noneValue","noneValue","noneValue"})}),
                          signature, rules);
  assert(conic_type && conv(*conic_type, B, indexed_rules));
  const CSSColorValue red_color{0,{false,false,false,false},{1.f,0.f,0.f,1.f}};
  auto constant_type = infer({}, apps(sym("constantGradientEqual"),{css_color_term(red_color),css_color_term(red_color)}),
                             signature, rules);
  assert(constant_type && conv(*constant_type, B, indexed_rules));
  auto color_image_type = infer({}, apps(sym("colorImageEqual"),{css_color_term(red_color),css_color_term(red_color)}),
                                signature, rules);
  assert(color_image_type && conv(*color_image_type, B, indexed_rules));
  Ptr paint_generator_sample=app(sym("paintGenerator"),
    paint_value_term({"paintNameA","alpha","noPaintArgument",nullptr}));
  auto generated_semantic_type = infer({}, apps(sym("generatedImageValuesEqual"),
    {container_term({std::nullopt,0u}),container_term({std::nullopt,0u}),
     viewport_term({0,0,0,0,0,0}),viewport_term({0,0,0,0,0,0}),
     paint_generator_sample,paint_generator_sample}),signature,rules);
  assert(generated_semantic_type && conv(*generated_semantic_type,B,indexed_rules));

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
  const std::vector<LinearGradient> linear_gradients={
    {"deprecatedLinear",{false,"spaceNone","hueShorter",{}},"noneValue","noneValue","noneValue","noneValue","noneValue"},
    {"deprecatedLinear",{true,"spaceLab","hueLonger",{}},"noneValue","noneValue","noneValue","noneValue","noneValue"},
    {"deprecatedLinear",{false,"spaceNone","hueShorter",{}},"coordA","coordB","coordA","coordB","noneValue"},
    {"deprecatedLinear",{false,"spaceNone","hueShorter",{}},"coordA","coordB","coordB","coordB","noneValue"},
    {"prefixedLinear",{false,"spaceNone","hueShorter",{}},"noneValue","noneValue","noneValue","noneValue","angleA"},
    {"prefixedLinear",{true,"spaceNone","hueShorter",{}},"coordB","coordB","noneValue","noneValue","angleA"},
    {"prefixedLinear",{false,"spaceNone","hueShorter",{}},"noneValue","noneValue","noneValue","noneValue","angleB"},
    {"prefixedLinear",{false,"spaceNone","hueShorter",{}},"coordA","noneValue","noneValue","noneValue","noneValue"},
    {"prefixedLinear",{false,"spaceNone","hueShorter",{}},"coordB","coordA","noneValue","noneValue","noneValue"},
    {"prefixedLinear",{false,"spaceNone","hueShorter",{{"colorRed","offsetZero"}}},"noneValue","noneValue","noneValue","noneValue","noneValue"},
    {"standardLinear",{false,"spaceNone","hueShorter",{}},"coordA","noneValue","noneValue","noneValue","noneValue"},
    {"standardLinear",{false,"spaceNone","hueShorter",{}},"noneValue","noneValue","noneValue","noneValue","angleA"}
  };
  auto optional_equal=[](const char* a,const char* b){
    return std::string(a)==b;
  };
  auto stop_lists_equal=[](const std::vector<GradientStop>& a,const std::vector<GradientStop>& b){
    if(a.size()!=b.size()) return false;
    for(size_t i=0;i<a.size();++i)
      if(std::string(a[i].color)!=b[i].color || std::string(a[i].offset)!=b[i].offset) return false;
    return true;
  };
  auto gradient_bases_equal=[&](const GradientBase& a,const GradientBase& b){
    return a.repeating==b.repeating && std::string(a.interpolation_space)==b.interpolation_space &&
      std::string(a.hue_method)==b.hue_method && stop_lists_equal(a.stops,b.stops);
  };
  auto linear_equals=[&](const LinearGradient& a,const LinearGradient& b){
    if(std::string(a.type)!=b.type) return false;
    if(std::string(a.type)=="deprecatedLinear")
      return optional_equal(a.first_x,b.first_x)&&optional_equal(a.first_y,b.first_y)&&
        optional_equal(a.second_x,b.second_x)&&optional_equal(a.second_y,b.second_y)&&
        stop_lists_equal(a.base.stops,b.base.stops);
    if(!gradient_bases_equal(a.base,b.base)) return false;
    if(std::string(a.angle)!="noneValue")
      return optional_equal(a.angle,b.angle)&&stop_lists_equal(a.base.stops,b.base.stops);
    if(std::string(b.angle)!="noneValue") return false;
    return optional_equal(a.first_x,b.first_x)&&optional_equal(a.first_y,b.first_y);
  };
  unsigned linear_pairs=0;
  for(const auto& left:linear_gradients) for(const auto& right:linear_gradients){
    const bool expected=linear_equals(left,right);
    Ptr term=apps(sym("linearGradientEqual"),{linear_gradient_term(left),linear_gradient_term(right)});
    if(!conv(normalize(term,indexed_rules),bool_term(expected),indexed_rules)){
      std::cerr<<"CSSLinearGradientValue equality mismatch\n";return 8;
    }
    ++linear_pairs;
  }
  std::cout<<"CSSLinearGradientValue deprecated/modern branch equality compares "
           <<linear_pairs<<" targeted finite value pairs.\n";
  const GradientBase base0={false,"spaceNone","hueShorter",{}};
  const GradientBase base1={true,"spaceLab","hueLonger",{}};
  const std::vector<RadialGradient> radial_gradients={
    {"deprecatedRadial",base0,"noneValue","noneValue","noneValue","noneValue","noneValue","noneValue","noneIdentifier","noneIdentifier","noneValue","noneValue"},
    {"deprecatedRadial",base1,"noneValue","noneValue","noneValue","noneValue","noneValue","noneValue","shapeCircle","sizingClose","noneValue","noneValue"},
    {"deprecatedRadial",base0,"coordA","noneValue","coordB","coordA","radiusA","radiusB","noneIdentifier","noneIdentifier","noneValue","noneValue"},
    {"deprecatedRadial",base0,"coordA","noneValue","coordB","coordA","radiusA","radiusA","noneIdentifier","noneIdentifier","noneValue","noneValue"},
    {"prefixedRadial",base0,"noneValue","noneValue","noneValue","noneValue","noneValue","noneValue","noneIdentifier","noneIdentifier","noneValue","noneValue"},
    {"prefixedRadial",base1,"noneValue","noneValue","noneValue","noneValue","noneValue","noneValue","noneIdentifier","noneIdentifier","noneValue","noneValue"},
    {"standardRadial",base0,"noneValue","noneValue","coordB","coordA","radiusB","radiusA","shapeEllipse","sizingFar","noneValue","noneValue"},
    {"prefixedRadial",base0,"noneValue","noneValue","noneValue","noneValue","noneValue","noneValue","shapeCircle","sizingClose","noneValue","noneValue"},
    {"prefixedRadial",base0,"noneValue","noneValue","noneValue","noneValue","noneValue","noneValue","noneIdentifier","noneIdentifier","sizeA","sizeB"},
    {"prefixedRadial",base0,"noneValue","noneValue","noneValue","noneValue","noneValue","noneValue","noneIdentifier","noneIdentifier","sizeA","sizeA"},
    {"prefixedRadial",base0,"noneValue","noneValue","noneValue","noneValue","noneValue","noneValue","noneIdentifier","noneIdentifier","noneValue","sizeB"}
  };
  auto identifier_equal_default=[](const char* a,const char* b,const char* default_value){
    const std::string left=std::string(a)=="noneIdentifier"?default_value:a;
    const std::string right=std::string(b)=="noneIdentifier"?default_value:b;
    return left==right;
  };
  auto radial_equals=[&](const RadialGradient& a,const RadialGradient& b){
    if(std::string(a.type)=="deprecatedRadial")
      return std::string(b.type)=="deprecatedRadial" &&
        optional_equal(a.first_x,b.first_x)&&optional_equal(a.first_y,b.first_y)&&
        optional_equal(a.second_x,b.second_x)&&optional_equal(a.second_y,b.second_y)&&
        optional_equal(a.first_radius,b.first_radius)&&optional_equal(a.second_radius,b.second_radius)&&
        stop_lists_equal(a.base.stops,b.base.stops);
    if(!gradient_bases_equal(a.base,b.base)||!optional_equal(a.first_x,b.first_x)||
       !optional_equal(a.first_y,b.first_y)) return false;
    if(std::string(a.end_horizontal_size)!="noneValue")
      return optional_equal(a.end_horizontal_size,b.end_horizontal_size)&&
             optional_equal(a.end_vertical_size,b.end_vertical_size);
    if(std::string(b.end_horizontal_size)!="noneValue") return false;
    return identifier_equal_default(a.sizing_behavior,b.sizing_behavior,"sizingFar")&&
           identifier_equal_default(a.shape,b.shape,"shapeEllipse");
  };
  unsigned radial_pairs=0;
  for(const auto& left:radial_gradients) for(const auto& right:radial_gradients){
    const bool expected=radial_equals(left,right);
    Ptr term=apps(sym("radialGradientEqual"),{radial_gradient_term(left),radial_gradient_term(right)});
    if(!conv(normalize(term,indexed_rules),bool_term(expected),indexed_rules)){
      std::cerr<<"CSSRadialGradientValue equality mismatch\n";return 9;
    }
    ++radial_pairs;
  }
  std::cout<<"CSSRadialGradientValue deprecated/modern branch equality compares "
           <<radial_pairs<<" targeted finite value pairs.\n";
  const std::vector<ConicGradient> conic_gradients={
    {base0,"noneValue","noneValue","noneValue"},
    {base0,"coordA","coordB","angleA"},
    {base0,"coordA","coordB","angleA"},
    {base0,"coordA","coordB","angleB"},
    {base1,"coordA","coordB","angleA"},
    {{false,"spaceNone","hueShorter",{{"colorRed","offsetZero"}}},"coordA","coordB","angleA"},
    {base0,"noneValue","coordA","noneValue"},
    {base0,"coordB","noneValue","angleB"}
  };
  unsigned conic_pairs=0;
  for(const auto& left:conic_gradients) for(const auto& right:conic_gradients){
    const bool expected=gradient_bases_equal(left.base,right.base)&&
      optional_equal(left.x,right.x)&&optional_equal(left.y,right.y)&&
      optional_equal(left.from_angle,right.from_angle);
    Ptr term=apps(sym("conicGradientEqual"),{conic_gradient_term(left),conic_gradient_term(right)});
    if(!conv(normalize(term,indexed_rules),bool_term(expected),indexed_rules)){
      std::cerr<<"CSSConicGradientValue equality mismatch\n";return 10;
    }
    ++conic_pairs;
  }
  std::cout<<"CSSConicGradientValue common-field/position/angle equality compares "
           <<conic_pairs<<" targeted finite value pairs.\n";
  unsigned constant_pairs=0,color_image_pairs=0;
  const std::vector<CSSColorValue> color_values={red_color,{0,{false,false,false,false},{0.f,0.f,1.f,1.f}}};
  auto css_colors_equal=[](const CSSColorValue& x,const CSSColorValue& y){return x.color_space==y.color_space&&x.none_flags==y.none_flags&&x.components==y.components;};
  for(const auto& left:color_values) for(const auto& right:color_values){
    const bool expected=css_colors_equal(left,right);
    if(!conv(normalize(apps(sym("constantGradientEqual"),{css_color_term(left),css_color_term(right)}),indexed_rules),
             bool_term(expected),indexed_rules) ||
       !conv(normalize(apps(sym("colorImageEqual"),{css_color_term(left),css_color_term(right)}),indexed_rules),
             bool_term(expected),indexed_rules)){
      std::cerr<<"constant/color image equality mismatch\n";return 11;
    }
    ++constant_pairs;++color_image_pairs;
  }
  std::cout<<"CSSConstantGradientValue and CSSColorImageValue delegated color equality compare "
           <<constant_pairs<<" and "<<color_image_pairs<<" pairs respectively.\n";

  std::vector<GeneratorValue> generator_values;
  auto add_generator=[&](GeneratorKind kind,GeneratorPayload payload){
    Ptr term;
    switch(kind){
      case GeneratorKind::Paint: term=app(sym("paintGenerator"),paint_value_term(std::get<CSSPaintValue>(payload)));break;
      case GeneratorKind::Linear: term=app(sym("linearGenerator"),linear_gradient_term(std::get<LinearGradient>(payload)));break;
      case GeneratorKind::Radial: term=app(sym("radialGenerator"),radial_gradient_term(std::get<RadialGradient>(payload)));break;
      case GeneratorKind::Conic: term=app(sym("conicGenerator"),conic_gradient_term(std::get<ConicGradient>(payload)));break;
      case GeneratorKind::Constant: term=app(sym("constantGenerator"),css_color_term(std::get<CSSColorValue>(payload)));break;
      case GeneratorKind::ColorImage: term=app(sym("colorImageGenerator"),css_color_term(std::get<CSSColorValue>(payload)));break;
    }
    generator_values.push_back({kind,std::move(payload),std::move(term)});
  };
  for(const auto& value:paint_values) add_generator(GeneratorKind::Paint,value);
  for(const auto& value:linear_gradients) add_generator(GeneratorKind::Linear,value);
  for(const auto& value:radial_gradients) add_generator(GeneratorKind::Radial,value);
  for(const auto& value:conic_gradients) add_generator(GeneratorKind::Conic,value);
  for(const auto& color:color_values){
    add_generator(GeneratorKind::Constant,color);
    add_generator(GeneratorKind::ColorImage,color);
  }
  auto generator_equal=[&](const GeneratorValue& a,const GeneratorValue& b){
    if(a.kind!=b.kind) return false;
    switch(a.kind){
      case GeneratorKind::Paint:{
        const auto& x=std::get<CSSPaintValue>(a.payload);const auto& y=std::get<CSSPaintValue>(b.payload);
        return std::string(x.name)==y.name&&paint_custom_css_text(x)==paint_custom_css_text(y);
      }
      case GeneratorKind::Linear:return linear_equals(std::get<LinearGradient>(a.payload),std::get<LinearGradient>(b.payload));
      case GeneratorKind::Radial:return radial_equals(std::get<RadialGradient>(a.payload),std::get<RadialGradient>(b.payload));
      case GeneratorKind::Conic:{
        const auto& x=std::get<ConicGradient>(a.payload);const auto& y=std::get<ConicGradient>(b.payload);
        return gradient_bases_equal(x.base,y.base)&&optional_equal(x.x,y.x)&&
          optional_equal(x.y,y.y)&&optional_equal(x.from_angle,y.from_angle);
      }
      case GeneratorKind::Constant:case GeneratorKind::ColorImage:
        const auto& x=std::get<CSSColorValue>(a.payload);const auto& y=std::get<CSSColorValue>(b.payload);
        return x.color_space==y.color_space&&x.none_flags==y.none_flags&&x.components==y.components;
    }
    return false;
  };
  const std::vector<std::pair<ContainerSizes,ContainerSizes>> image_container_pairs={
    {{std::nullopt,0u},{std::nullopt,0u}},{{std::nullopt,0u},{0u,0u}}};
  const std::vector<std::pair<ViewportSize,ViewportSize>> image_viewport_pairs={
    {{{0,0,0,0,0,0}},{{0,0,0,0,0,0}}},{{{0,0,0,0,0,0}},{{1,0,0,0,0,0}}}};
  unsigned composed_image_pairs=0,composed_reattach_pairs=0;
  for(const auto& left_generator:generator_values) for(const auto& right_generator:generator_values)
    for(const auto& [left_container,right_container]:image_container_pairs)
      for(const auto& [left_viewport,right_viewport]:image_viewport_pairs){
        const bool sizes_equal=left_container.width==right_container.width&&
          left_container.height==right_container.height&&left_viewport==right_viewport;
        const bool expected=sizes_equal&&generator_equal(left_generator,right_generator);
        std::vector<Ptr> args={container_term(left_container),container_term(right_container),
          viewport_term(left_viewport),viewport_term(right_viewport),left_generator.term,right_generator.term};
        Ptr equal_term=apps(sym("generatedImageValuesEqual"),args);
        if(!conv(normalize(equal_term,indexed_rules),bool_term(expected),indexed_rules)){
          std::cerr<<"composed StyleGeneratedImage equality mismatch\n";return 12;
        }
        ++composed_image_pairs;
        Ptr changed_term=apps(sym("generatedImageValuesChanged"),args);
        if(!conv(normalize(changed_term,indexed_rules),bool_term(!expected),indexed_rules)){
          std::cerr<<"composed StyleGeneratedImage change trigger mismatch\n";return 13;
        }
        ++composed_reattach_pairs;
      }
  std::cout<<"Composed StyleGeneratedImage size plus tagged CSS generator equality compares "
           <<composed_image_pairs<<" states and "<<composed_reattach_pairs<<" change decisions.\n";
}
