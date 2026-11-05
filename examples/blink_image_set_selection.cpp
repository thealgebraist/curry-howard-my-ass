#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

using namespace lp;

struct Option {
  unsigned image;
  unsigned resolution;
  std::optional<std::string> mime_type;
};
struct MimeFeatures { bool avif_build; bool jxl_build; bool jxl_runtime; };
using Options = std::vector<Option>;

static std::string ascii_lower(std::string value) {
  for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return value;
}
static bool cpp_mime_supported(const std::string& mime, MimeFeatures features) {
  const std::string normalized = ascii_lower(mime);
  static const std::unordered_set<std::string> supported = {
      "image/jpeg", "image/pjpeg", "image/jpg", "image/webp", "image/png",
      "image/apng", "image/gif", "image/bmp", "image/vnd.microsoft.icon",
      "image/x-icon", "image/x-xbitmap", "image/x-png"};
  if (normalized == "image/avif") return features.avif_build;
  if (normalized == "image/jxl") return features.jxl_build && features.jxl_runtime;
  return supported.contains(normalized);
}
static bool supported(const Option& option, MimeFeatures features) {
  return (!option.mime_type || cpp_mime_supported(*option.mime_type, features)) &&
         option.resolution > 0;
}
static Options cpp_filter(const Options& options, MimeFeatures features) {
  Options result;
  for (const auto& option : options) if (supported(option, features)) result.push_back(option);
  return result;
}
static Options cpp_stable_sort_unique(Options options) {
  std::stable_sort(options.begin(), options.end(),
      [](const Option& a, const Option& b) { return a.resolution < b.resolution; });
  Options unique;
  for (const auto& option : options)
    if (unique.empty() || unique.back().resolution != option.resolution)
      unique.push_back(option);
  return unique;
}
static int cpp_best_option(const Options& options, unsigned device_scale, MimeFeatures features) {
  Options filtered = cpp_filter(options, features);
  Options normalized = cpp_stable_sort_unique(std::move(filtered));
  for (const auto& option : normalized)
    if (option.resolution >= device_scale) return static_cast<int>(option.image);
  return normalized.empty() ? -1 : static_cast<int>(normalized.back().image);
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr apps(Ptr f, const std::vector<Ptr>& args) {
  for (const auto& arg : args) f = app(f, arg);
  return f;
}
static Ptr bool_term(bool value) { return sym(value ? "true" : "false"); }
static Ptr resolution_term(unsigned value) { return sym(("r" + std::to_string(value)).c_str()); }
static Ptr scale_term(unsigned value) { return sym(("scale" + std::to_string(value)).c_str()); }
static Ptr features_term(MimeFeatures f) {
  return apps(sym("mimeFeatures"), {bool_term(f.avif_build), bool_term(f.jxl_build), bool_term(f.jxl_runtime)});
}
static Ptr mime_term(const std::string& mime) {
  const std::string lower = ascii_lower(mime);
  if (lower == "image/jpeg") return sym(mime == lower ? "mimeJpeg" : "mimeJpegUpper");
  if (lower == "image/pjpeg") return sym(mime == lower ? "mimePjpeg" : "mimePjpegUpper");
  if (lower == "image/jpg") return sym(mime == lower ? "mimeJpg" : "mimeJpgUpper");
  if (lower == "image/webp") return sym(mime == lower ? "mimeWebp" : "mimeWebpUpper");
  if (lower == "image/png") return sym(mime == lower ? "mimePng" : "mimePngUpper");
  if (lower == "image/apng") return sym(mime == lower ? "mimeApng" : "mimeApngUpper");
  if (lower == "image/gif") return sym(mime == lower ? "mimeGif" : "mimeGifUpper");
  if (lower == "image/bmp") return sym(mime == lower ? "mimeBmp" : "mimeBmpUpper");
  if (lower == "image/vnd.microsoft.icon") return sym(mime == lower ? "mimeMicrosoftIcon" : "mimeMicrosoftIconUpper");
  if (lower == "image/x-icon") return sym(mime == lower ? "mimeXIcon" : "mimeXIconUpper");
  if (lower == "image/x-xbitmap") return sym(mime == lower ? "mimeXXbitmap" : "mimeXXbitmapUpper");
  if (lower == "image/x-png") return sym(mime == lower ? "mimeXPng" : "mimeXPngUpper");
  if (lower == "image/svg+xml") return sym("mimeSvg");
  if (lower == "image/avif") return sym("mimeAvif");
  if (lower == "image/jxl") return sym("mimeJxl");
  return sym("mimeUnknown");
}
static Ptr optional_mime_term(const std::optional<std::string>& mime) {
  return mime ? app(sym("someMime"), mime_term(*mime)) : sym("noMime");
}
static Ptr option_term(const Option& option) {
  return apps(sym("option"), {sym(("image" + std::to_string(option.image)).c_str()),
      resolution_term(option.resolution), optional_mime_term(option.mime_type)});
}
static Ptr options_term(const Options& options) {
  Ptr result = sym("nilOptions");
  for (auto it = options.rbegin(); it != options.rend(); ++it)
    result = apps(sym("consOption"), {option_term(*it), result});
  return result;
}
static Ptr best_term(const Options& options, unsigned scale, MimeFeatures features) {
  return apps(sym("bestOption"), {options_term(options), scale_term(scale), features_term(features)});
}

static void add_selection_rules(Signature& signature, std::vector<Rule>& rules) {
  const Ptr B=sym("Bool"), R=sym("Resolution"), D=sym("DeviceScale");
  const Ptr I=sym("ImageId"), O=sym("ImageSetOption"), L=sym("OptionList");
  const Ptr M=sym("MimeInput"), C=sym("CanonicalMime"), OM=sym("OptionalMime"), F=sym("MimeFeatures");

  signature["Bool"]=type_sort(); signature["true"]=B; signature["false"]=B;
  signature["Resolution"]=type_sort();
  for(unsigned i=0;i<=3;++i) signature["r"+std::to_string(i)]=R;
  signature["DeviceScale"]=type_sort();
  for(unsigned i=1;i<=4;++i) signature["scale"+std::to_string(i)]=D;
  signature["ImageId"]=type_sort();
  for(unsigned i=0;i<6;++i) signature["image"+std::to_string(i)]=I;
  signature["MimeFeatures"]=type_sort(); signature["mimeFeatures"]=fn(B,fn(B,fn(B,F)));
  signature["MimeInput"]=type_sort();
  for(const char* mime:{"mimeJpeg", "mimeJpegUpper","mimePjpeg", "mimePjpegUpper","mimeJpg", "mimeJpgUpper","mimeWebp", "mimeWebpUpper","mimePng", "mimePngUpper","mimeApng", "mimeApngUpper","mimeGif", "mimeGifUpper","mimeBmp", "mimeBmpUpper","mimeMicrosoftIcon", "mimeMicrosoftIconUpper","mimeXIcon", "mimeXIconUpper","mimeXXbitmap", "mimeXXbitmapUpper","mimeXPng", "mimeXPngUpper","mimeSvg","mimeAvif","mimeJxl","mimeUnknown"}) signature[mime]=M;
  signature["CanonicalMime"]=type_sort();
  for(const char* mime:{"canonicalStaticImage","canonicalSvg","canonicalAvif","canonicalJxl","canonicalOther"}) signature[mime]=C;
  signature["OptionalMime"]=type_sort(); signature["noMime"]=OM; signature["someMime"]=fn(M,OM);
  signature["ImageSetOption"]=type_sort(); signature["option"]=fn(I,fn(R,fn(OM,O)));
  signature["OptionList"]=type_sort(); signature["nilOptions"]=L; signature["consOption"]=fn(O,fn(L,L));
  signature["SelectedOption"]=type_sort(); signature["noOption"]=sym("SelectedOption");
  signature["someOption"]=fn(O,sym("SelectedOption"));
  signature["andB"]=fn(B,fn(B,B)); signature["notB"]=fn(B,B);
  signature["positiveResolution"]=fn(R,B); signature["optionResolution"]=fn(O,R);
  signature["normalizeMime"]=fn(M,C); signature["canonicalMimeSupported"]=fn(C,fn(F,B));
  signature["mimeSupported"]=fn(M,fn(F,B)); signature["optionalMimeSupported"]=fn(OM,fn(F,B));
  signature["supportedOption"]=fn(O,fn(F,B));
  signature["filterOptions"]=fn(L,fn(F,L)); signature["filterStep"]=fn(B,fn(O,fn(F,fn(L,L))));
  signature["lessResolution"]=fn(R,fn(R,B)); signature["equalResolution"]=fn(R,fn(R,B));
  signature["insertOption"]=fn(O,fn(L,L)); signature["insertStep"]=fn(B,fn(O,fn(O,fn(L,L))));
  signature["sortOptions"]=fn(L,L); signature["dropResolution"]=fn(R,fn(L,L));
  signature["uniqueOptions"]=fn(L,L); signature["atLeastScale"]=fn(R,fn(D,B));
  signature["selectOption"]=fn(D,fn(L,fn(sym("SelectedOption"),sym("SelectedOption"))));
  signature["selectStep"]=fn(B,fn(D,fn(O,fn(L,fn(sym("SelectedOption"),sym("SelectedOption"))))));
  signature["bestOption"]=fn(L,fn(D,fn(F,sym("SelectedOption"))));

  for(bool a:{false,true}) for(bool b:{false,true}) rules.push_back(rule(apps(sym("andB"),{bool_term(a),bool_term(b)}),bool_term(a&&b)));
  for(unsigned r=0;r<=3;++r) rules.push_back(rule(app(sym("positiveResolution"),resolution_term(r)),bool_term(r>0)));
  for(const char* mime:{"mimeJpeg", "mimeJpegUpper","mimePjpeg", "mimePjpegUpper","mimeJpg", "mimeJpgUpper","mimeWebp", "mimeWebpUpper","mimePng", "mimePngUpper","mimeApng", "mimeApngUpper","mimeGif", "mimeGifUpper","mimeBmp", "mimeBmpUpper","mimeMicrosoftIcon", "mimeMicrosoftIconUpper","mimeXIcon", "mimeXIconUpper","mimeXXbitmap", "mimeXXbitmapUpper","mimeXPng", "mimeXPngUpper"}) rules.push_back(rule(app(sym("normalizeMime"),sym(mime)),sym("canonicalStaticImage")));
  rules.push_back(rule(app(sym("normalizeMime"),sym("mimeSvg")),sym("canonicalSvg")));
  rules.push_back(rule(app(sym("normalizeMime"),sym("mimeAvif")),sym("canonicalAvif")));
  rules.push_back(rule(app(sym("normalizeMime"),sym("mimeJxl")),sym("canonicalJxl")));
  rules.push_back(rule(app(sym("normalizeMime"),pvar(0)),sym("canonicalOther")));
  for(const char* mime:{"canonicalStaticImage"})
    rules.push_back(rule(apps(sym("canonicalMimeSupported"),{sym(mime),pvar(1)}),sym("true")));
  rules.push_back(rule(apps(sym("canonicalMimeSupported"),{sym("canonicalSvg"),pvar(2)}),sym("false")));
  rules.push_back(rule(apps(sym("canonicalMimeSupported"),{sym("canonicalOther"),pvar(3)}),sym("false")));
  rules.push_back(rule(apps(sym("canonicalMimeSupported"),
      {sym("canonicalAvif"),apps(sym("mimeFeatures"),{pvar(4),pvar(5),pvar(6)})}),pvar(4)));
  rules.push_back(rule(apps(sym("canonicalMimeSupported"),
      {sym("canonicalJxl"),apps(sym("mimeFeatures"),{pvar(7),pvar(8),pvar(9)})}),apps(sym("andB"),{pvar(8),pvar(9)})));
  rules.push_back(rule(apps(sym("mimeSupported"),{pvar(10),pvar(11)}),
      apps(sym("canonicalMimeSupported"),{app(sym("normalizeMime"),pvar(10)),pvar(11)})));
  rules.push_back(rule(apps(sym("optionalMimeSupported"),{sym("noMime"),pvar(12)}),sym("true")));
  rules.push_back(rule(apps(sym("optionalMimeSupported"),{app(sym("someMime"),pvar(13)),pvar(14)}),apps(sym("mimeSupported"),{pvar(13),pvar(14)})));
  rules.push_back(rule(apps(sym("supportedOption"),
      {apps(sym("option"),{pvar(15),pvar(16),pvar(17)}),pvar(18)}),
      apps(sym("andB"),{apps(sym("optionalMimeSupported"),{pvar(17),pvar(18)}),app(sym("positiveResolution"),pvar(16))})));

  rules.push_back(rule(apps(sym("filterOptions"),{sym("nilOptions"),pvar(19)}),sym("nilOptions")));
  rules.push_back(rule(apps(sym("filterOptions"),
      {apps(sym("consOption"),{pvar(20),pvar(21)}),pvar(22)}),
      apps(sym("filterStep"),{apps(sym("supportedOption"),{pvar(20),pvar(22)}),pvar(20),pvar(22),
                                apps(sym("filterOptions"),{pvar(21),pvar(22)})})));
  rules.push_back(rule(apps(sym("filterStep"),{sym("true"),pvar(23),pvar(24),pvar(25)}),apps(sym("consOption"),{pvar(23),pvar(25)})));
  rules.push_back(rule(apps(sym("filterStep"),{sym("false"),pvar(26),pvar(27),pvar(28)}),pvar(28)));

  for(unsigned a=0;a<=3;++a) for(unsigned b=0;b<=3;++b){
    rules.push_back(rule(apps(sym("lessResolution"),{resolution_term(a),resolution_term(b)}),bool_term(a<b)));
    rules.push_back(rule(apps(sym("equalResolution"),{resolution_term(a),resolution_term(b)}),bool_term(a==b)));
  }
  rules.push_back(rule(apps(sym("insertOption"),{pvar(30),sym("nilOptions")}),apps(sym("consOption"),{pvar(30),sym("nilOptions")})));
  rules.push_back(rule(apps(sym("insertOption"),{pvar(31),apps(sym("consOption"),{pvar(32),pvar(33)})}),
      apps(sym("insertStep"),{apps(sym("lessResolution"),{app(sym("optionResolution"),pvar(31)),app(sym("optionResolution"),pvar(32))}),pvar(31),pvar(32),pvar(33)})));
  rules.push_back(rule(apps(sym("optionResolution"),{apps(sym("option"),{pvar(34),pvar(35),pvar(36)})}),pvar(35)));
  rules.push_back(rule(apps(sym("insertStep"),{sym("true"),pvar(37),pvar(38),pvar(39)}),apps(sym("consOption"),{pvar(37),apps(sym("consOption"),{pvar(38),pvar(39)})})));
  rules.push_back(rule(apps(sym("insertStep"),{sym("false"),pvar(40),pvar(41),pvar(42)}),apps(sym("consOption"),{pvar(41),apps(sym("insertOption"),{pvar(40),pvar(42)})})));
  rules.push_back(rule(app(sym("sortOptions"),sym("nilOptions")),sym("nilOptions")));
  rules.push_back(rule(app(sym("sortOptions"),apps(sym("consOption"),{pvar(43),pvar(44)})),apps(sym("insertOption"),{pvar(43),app(sym("sortOptions"),pvar(44))})));

  rules.push_back(rule(apps(sym("dropResolution"),{pvar(45),sym("nilOptions")}),sym("nilOptions")));
  rules.push_back(rule(apps(sym("dropResolution"),{pvar(46),apps(sym("consOption"),{apps(sym("option"),{pvar(47),pvar(48),pvar(49)}),pvar(50)})}),
      apps(sym("dropResolutionStep"),{apps(sym("equalResolution"),{pvar(46),pvar(48)}),pvar(46),apps(sym("option"),{pvar(47),pvar(48),pvar(49)}),pvar(50)})));
  signature["dropResolutionStep"]=fn(B,fn(R,fn(O,fn(L,L))));
  rules.push_back(rule(apps(sym("dropResolutionStep"),{sym("true"),pvar(51),pvar(52),pvar(53)}),apps(sym("dropResolution"),{pvar(51),pvar(53)})));
  rules.push_back(rule(apps(sym("dropResolutionStep"),{sym("false"),pvar(54),pvar(55),pvar(56)}),apps(sym("consOption"),{pvar(55),apps(sym("dropResolution"),{pvar(54),pvar(56)})})));
  rules.push_back(rule(app(sym("uniqueOptions"),sym("nilOptions")),sym("nilOptions")));
  rules.push_back(rule(app(sym("uniqueOptions"),apps(sym("consOption"),{apps(sym("option"),{pvar(57),pvar(58),pvar(59)}),pvar(60)})),
      apps(sym("consOption"),{apps(sym("option"),{pvar(57),pvar(58),pvar(59)}),app(sym("uniqueOptions"),apps(sym("dropResolution"),{pvar(58),pvar(60)}))})));

  for(unsigned r=0;r<=3;++r) for(unsigned d=1;d<=4;++d)
    rules.push_back(rule(apps(sym("atLeastScale"),{resolution_term(r),scale_term(d)}),bool_term(r>=d)));
  rules.push_back(rule(apps(sym("selectOption"),{pvar(61),sym("nilOptions"),pvar(62)}),pvar(62)));
  rules.push_back(rule(apps(sym("selectOption"),{pvar(63),apps(sym("consOption"),{apps(sym("option"),{pvar(64),pvar(65),pvar(66)}),pvar(67)}),pvar(68)}),
      apps(sym("selectStep"),{apps(sym("atLeastScale"),{pvar(65),pvar(63)}),pvar(63),apps(sym("option"),{pvar(64),pvar(65),pvar(66)}),pvar(67),pvar(68)})));
  rules.push_back(rule(apps(sym("selectStep"),{sym("true"),pvar(69),pvar(70),pvar(71),pvar(72)}),app(sym("someOption"),pvar(70))));
  rules.push_back(rule(apps(sym("selectStep"),{sym("false"),pvar(73),pvar(74),pvar(75),pvar(76)}),apps(sym("selectOption"),{pvar(73),pvar(75),app(sym("someOption"),pvar(74))})));
  rules.push_back(rule(apps(sym("bestOption"),{pvar(77),pvar(78),pvar(79)}),
      apps(sym("selectOption"),{pvar(78),app(sym("uniqueOptions"),app(sym("sortOptions"),apps(sym("filterOptions"),{pvar(77),pvar(79)}))),sym("noOption")})));
  return;
}

int main() {
  Signature signature;
  std::vector<Rule> rules;
  add_selection_rules(signature, rules);
  RuleIndex indexed_rules(rules);
  Options empty;
  auto sample=best_term(empty,1,{false,false,false});
  auto inferred=infer({},sample,signature,rules);
  assert(inferred&&conv(*inferred,sym("SelectedOption"),indexed_rules));

  const std::vector<Options> cases={
    {},
    {{0,1,std::nullopt}},
    {{1,2,"image/jpeg"},{2,1,"image/webp"}},
    {{3,1,"image/jpeg"},{4,1,"IMAGE/WEBP"},{5,2,"image/png"}},
    {{0,0,std::nullopt},{1,2,"application/octet-stream"}},
    {{0,2,"image/svg+xml"},{1,1,"image/jpeg"},{2,3,"image/avif"}},
    {{0,3,"image/jxl"},{1,1,"image/webp"},{2,2,"image/png"}},
    {{0,1,"IMAGE/JPEG"},{1,2,"IMAGE/WEBP"}},
    {{0,1,"image/jpeg"}},
    {{0,1,"IMAGE/PJPEG"}},
    {{0,1,"image/jpg"}},
    {{0,1,"IMAGE/WEBP"}},
    {{0,1,"image/png"}},
    {{0,1,"IMAGE/APNG"}},
    {{0,1,"image/gif"}},
    {{0,1,"IMAGE/BMP"}},
    {{0,1,"image/vnd.microsoft.icon"}},
    {{0,1,"IMAGE/X-ICON"}},
    {{0,1,"image/x-xbitmap"}},
    {{0,1,"IMAGE/X-PNG"}},
    {{0,1,"image/svg+xml"},{1,2,"application/octet-stream"}}
  };
  unsigned states=0;
  for(const auto& options:cases) for(unsigned scale=1;scale<=4;++scale)
    for(unsigned avif=0;avif<2;++avif)for(unsigned jxl_build=0;jxl_build<2;++jxl_build)for(unsigned jxl_runtime=0;jxl_runtime<2;++jxl_runtime){
      MimeFeatures features{avif!=0,jxl_build!=0,jxl_runtime!=0};
      const int expected=cpp_best_option(options,scale,features);
      Ptr result=best_term(options,scale,features);
      Ptr expected_term=expected<0?sym("noOption"):app(sym("someOption"),option_term(options[0]));
      if(expected>=0){bool found=false;for(const auto& option:options)if(static_cast<int>(option.image)==expected){expected_term=app(sym("someOption"),option_term(option));found=true;break;}assert(found);}
      if(!conv(nf(result,indexed_rules),expected_term,indexed_rules)){
        std::cerr<<"CSSImageSetValue::GetBestOption mismatch at device scale "<<scale<<"\n";return 2;
      }
      ++states;
    }
  std::cout<<"CSSImageSetValue MIME-aware filtering, stable resolution ordering/deduplication, and device-scale selection cover "<<states<<" finite states.\n";
  std::cout<<"Scope: resolution values are 0..3, device scales 1..4, all twelve pinned static image MIME entries and ASCII case variants are represented, AVIF/JXL feature flags vary, and each fixture has at most three options; arbitrary MIME strings, floating-point resolution computation, lazy options_ cache mutation, and later image loading remain omitted.\n";
  return 0;
}
