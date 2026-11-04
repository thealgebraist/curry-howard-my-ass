#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

using namespace lp;

struct Features { bool avif_build; bool jxl_build; bool jxl_runtime; };
struct OptionSupportCase { std::optional<std::string> mime; float resolution; };
static std::string ascii_lower(std::string value) {
  for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return value;
}
static bool cpp_mime_supported(const std::string& mime, Features features) {
  const std::string normalized = ascii_lower(mime);
  static const std::unordered_set<std::string> supported = {
      "image/jpeg", "image/pjpeg", "image/jpg", "image/webp", "image/png",
      "image/apng", "image/gif", "image/bmp", "image/vnd.microsoft.icon",
      "image/x-icon", "image/x-xbitmap", "image/x-png"};
  if (normalized == "image/avif") return features.avif_build;
  if (normalized == "image/jxl") return features.jxl_build && features.jxl_runtime;
  return supported.contains(normalized);
}
static bool cpp_option_supported(const OptionSupportCase& option, Features features) {
  return (!option.mime || cpp_mime_supported(*option.mime, features)) && option.resolution > 0.0f;
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr apps(Ptr f, const std::vector<Ptr>& xs) {
  for (const auto& x : xs) f = app(f, x);
  return f;
}
static Ptr bt(bool value) { return sym(value ? "true" : "false"); }
static Ptr features_term(Features f) {
  return apps(sym("mimeFeatures"), {bt(f.avif_build), bt(f.jxl_build), bt(f.jxl_runtime)});
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

int main() {
  const Ptr B=sym("Bool"), F=sym("MimeFeatures"), M=sym("MimeInput");
  const Ptr C=sym("CanonicalMime"), O=sym("OptionalMime"), R=sym("Resolution");
  Signature signature;
  std::vector<Rule> rules;
  signature["Bool"]=type_sort(); signature["true"]=B; signature["false"]=B;
  signature["MimeFeatures"]=type_sort();
  signature["mimeFeatures"]=fn(B,fn(B,fn(B,F)));
  signature["MimeInput"]=type_sort();
  for(const char* mime:{"mimeJpeg", "mimeJpegUpper","mimePjpeg", "mimePjpegUpper","mimeJpg", "mimeJpgUpper","mimeWebp", "mimeWebpUpper","mimePng", "mimePngUpper","mimeApng", "mimeApngUpper","mimeGif", "mimeGifUpper","mimeBmp", "mimeBmpUpper","mimeMicrosoftIcon", "mimeMicrosoftIconUpper","mimeXIcon", "mimeXIconUpper","mimeXXbitmap", "mimeXXbitmapUpper","mimeXPng", "mimeXPngUpper","mimeSvg","mimeAvif","mimeJxl","mimeUnknown"}) signature[mime]=M;
  signature["CanonicalMime"]=type_sort();
  for(const char* mime:{"canonicalStaticImage","canonicalSvg","canonicalAvif","canonicalJxl","canonicalOther"}) signature[mime]=C;
  signature["OptionalMime"]=type_sort(); signature["noMime"]=O;
  signature["someMime"]=fn(M,O);
  signature["Resolution"]=type_sort(); signature["resolutionZero"]=R; signature["resolutionPositive"]=R;
  signature["andB"]=fn(B,fn(B,B));
  signature["normalizeMime"]=fn(M,C);
  signature["canonicalMimeSupported"]=fn(C,fn(F,B));
  signature["mimeSupported"]=fn(M,fn(F,B));
  signature["optionalMimeSupported"]=fn(O,fn(F,B));
  signature["positiveResolution"]=fn(R,B);
  signature["imageSetOptionSupported"]=fn(O,fn(R,fn(F,B)));

  for(bool a:{false,true})for(bool b:{false,true})
    rules.push_back(rule(apps(sym("andB"),{bt(a),bt(b)}),bt(a&&b)));
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
      {sym("canonicalJxl"),apps(sym("mimeFeatures"),{pvar(7),pvar(8),pvar(9)})}),
      apps(sym("andB"),{pvar(8),pvar(9)})));
  rules.push_back(rule(apps(sym("mimeSupported"),{pvar(10),pvar(11)}),
      apps(sym("canonicalMimeSupported"),{app(sym("normalizeMime"),pvar(10)),pvar(11)})));
  rules.push_back(rule(apps(sym("optionalMimeSupported"),{sym("noMime"),pvar(12)}),sym("true")));
  rules.push_back(rule(apps(sym("optionalMimeSupported"),{app(sym("someMime"),pvar(13)),pvar(14)}),
      apps(sym("mimeSupported"),{pvar(13),pvar(14)})));
  rules.push_back(rule(app(sym("positiveResolution"),sym("resolutionZero")),sym("false")));
  rules.push_back(rule(app(sym("positiveResolution"),sym("resolutionPositive")),sym("true")));
  rules.push_back(rule(apps(sym("imageSetOptionSupported"),{pvar(15),pvar(16),pvar(17)}),
      apps(sym("andB"),{apps(sym("optionalMimeSupported"),{pvar(15),pvar(17)}),
                         app(sym("positiveResolution"),pvar(16))})));

  RuleIndex indexed_rules(rules);
  Features base{false,false,false};
  Ptr sample=apps(sym("imageSetOptionSupported"),{sym("noMime"),sym("resolutionPositive"),features_term(base)});
  auto inferred=infer({},sample,signature,rules);
  assert(inferred&&conv(*inferred,B,indexed_rules));

  const std::vector<OptionSupportCase> cases={
      {std::nullopt,0.0f},{std::nullopt,1.0f},
      {"image/jpeg",1.0f},{"IMAGE/PJPEG",1.0f},{"image/jpg",1.0f},
      {"IMAGE/WEBP",1.0f},{"image/png",1.0f},{"IMAGE/APNG",1.0f},
      {"image/gif",1.0f},{"IMAGE/BMP",1.0f},
      {"image/vnd.microsoft.icon",1.0f},{"IMAGE/X-ICON",1.0f},
      {"image/x-xbitmap",1.0f},{"IMAGE/X-PNG",1.0f},
      {"image/svg+xml",1.0f},{"image/avif",1.0f},{"image/jxl",1.0f},
      {"application/octet-stream",1.0f},{"image/jpeg",0.0f},{"IMAGE/WEBP",0.0f}};
  unsigned states=0;
  for(unsigned avif_build=0;avif_build<2;++avif_build)
    for(unsigned jxl_build=0;jxl_build<2;++jxl_build)
      for(unsigned jxl_runtime=0;jxl_runtime<2;++jxl_runtime){
        Features config{avif_build!=0,jxl_build!=0,jxl_runtime!=0};
        for(const auto& option:cases){
          Ptr mime=optional_mime_term(option.mime);
          Ptr resolution=sym(option.resolution>0?"resolutionPositive":"resolutionZero");
          Ptr term=apps(sym("imageSetOptionSupported"),{mime,resolution,features_term(config)});
          if(!conv(nf(term,indexed_rules),bt(cpp_option_supported(option,config)),indexed_rules)){
            std::cerr<<"CSSImageSetOptionValue::IsSupported mismatch\n";return 2;
          }
          ++states;
        }
      }
  std::cout<<"CSSImageSetTypeValue MIME normalization/support and option positive-resolution gating cover "<<states<<" finite states.\n";
  std::cout<<"Scope: all twelve pinned static supported-image MIME entries, unsupported and case-variant values, and AVIF/JXL feature-gated MIME values and resolutions 0/1 are modeled; full Chromium build configuration and arbitrary MIME strings are not.\n";
}
