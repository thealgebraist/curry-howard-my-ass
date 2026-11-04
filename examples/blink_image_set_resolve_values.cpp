#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

struct Option {
  unsigned image;
  unsigned resolution;
  int type;
};
struct InputOption {
  Option option;
  bool image_reference_unchanged;
  unsigned resolved_image;
};
using InputOptions = std::vector<InputOption>;
struct ResolveResult {
  bool copied;
  std::vector<Option> options;
};
static ResolveResult cpp_resolve(const InputOptions& input) {
  ResolveResult result{false,{}};
  for (const auto& item : input) {
    Option output = item.option;
    if (!item.image_reference_unchanged) {
      result.copied = true;
      output.image = item.resolved_image;
    }
    result.options.push_back(output);
  }
  return result;
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr apps(Ptr f, const std::vector<Ptr>& xs) {
  for (const auto& x : xs) f = app(f, x);
  return f;
}
static Ptr bool_term(bool b) { return sym(b ? "true" : "false"); }
static Ptr image_term(unsigned i) { return sym(i == 0 ? "imageA" : i == 1 ? "imageB" : "imageC"); }
static Ptr resolution_term(unsigned r) { return sym(r == 0 ? "resolution1x" : "resolution2x"); }
static Ptr type_term(int t) { return t < 0 ? sym("noType") : app(sym("someType"), sym(t == 0 ? "typeAvif" : "typeWebp")); }
static Ptr option_term(const Option& o) {
  return apps(sym("imageSetOption"), {image_term(o.image), resolution_term(o.resolution), type_term(o.type)});
}
static Ptr input_term(const InputOptions& input) {
  Ptr list = sym("nilInputOptions");
  for (auto it=input.rbegin(); it!=input.rend(); ++it) {
    Ptr update = it->image_reference_unchanged ? sym("sameImageReference")
                                               : app(sym("replaceImageReference"), image_term(it->resolved_image));
    list = apps(sym("consInputOption"), {apps(sym("inputOption"), {option_term(it->option), update}), list});
  }
  return list;
}
static Ptr option_list_term(const std::vector<Option>& options) {
  Ptr list = sym("nilOptions");
  for (auto it=options.rbegin(); it!=options.rend(); ++it)
    list = apps(sym("consOption"), {option_term(*it), list});
  return list;
}
static Ptr expected_term(const InputOptions& input, const ResolveResult& result) {
  if (result.copied) return app(sym("copiedCSSImageSet"), option_list_term(result.options));
  std::vector<Option> original;
  for (const auto& item : input) original.push_back(item.option);
  return app(sym("sameCSSImageSet"), option_list_term(original));
}

int main() {
  const Ptr B=sym("Bool"), I=sym("ImageId"), R=sym("Resolution");
  const Ptr T=sym("ImageSetType"), OT=sym("OptionalImageSetType");
  const Ptr O=sym("CSSImageSetOption"), L=sym("OptionList");
  const Ptr U=sym("ImageReferenceUpdate"), X=sym("InputOption"), XL=sym("InputOptionList");
  const Ptr RR=sym("ResolvedOption"), RRL=sym("ResolvedOptions"), V=sym("ResolutionResult");
  Signature signature;
  std::vector<Rule> rules;
  signature["Bool"]=type_sort(); signature["true"]=B; signature["false"]=B;
  signature["ImageId"]=type_sort(); signature["imageA"]=I; signature["imageB"]=I; signature["imageC"]=I;
  signature["Resolution"]=type_sort(); signature["resolution1x"]=R; signature["resolution2x"]=R;
  signature["ImageSetType"]=type_sort(); signature["typeAvif"]=T; signature["typeWebp"]=T;
  signature["OptionalImageSetType"]=type_sort(); signature["noType"]=OT; signature["someType"]=fn(T,OT);
  signature["CSSImageSetOption"]=type_sort(); signature["imageSetOption"]=fn(I,fn(R,fn(OT,O)));
  signature["OptionList"]=type_sort(); signature["nilOptions"]=L; signature["consOption"]=fn(O,fn(L,L));
  signature["ImageReferenceUpdate"]=type_sort(); signature["sameImageReference"]=U;
  signature["replaceImageReference"]=fn(I,U);
  signature["InputOption"]=type_sort(); signature["inputOption"]=fn(O,fn(U,X));
  signature["InputOptionList"]=type_sort(); signature["nilInputOptions"]=XL; signature["consInputOption"]=fn(X,fn(XL,XL));
  signature["ResolvedOption"]=type_sort(); signature["resolvedOption"]=fn(B,fn(O,RR));
  signature["ResolvedOptions"]=type_sort(); signature["resolvedOptions"]=fn(B,fn(L,RRL));
  signature["noCopy"]=V; signature["copyOptions"]=fn(L,V);
  signature["ResolutionResult"]=type_sort();
  signature["sameCSSImageSet"]=fn(L,V); signature["copiedCSSImageSet"]=fn(L,V);
  signature["orB"]=fn(B,fn(B,B));
  signature["resolveOne"]=fn(O,fn(U,RR));
  signature["combineResolved"]=fn(RR,fn(RRL,RRL));
  signature["resolveOptions"]=fn(XL,RRL);
  signature["originalOptions"]=fn(XL,L);
  signature["createCopyIfChanged"]=fn(RRL,V);
  signature["finishResolution"]=fn(V,fn(L,V));
  signature["resolveValuesIfNeeded"]=fn(XL,V);

  for(bool a:{false,true})for(bool b:{false,true})
    rules.push_back(rule(apps(sym("orB"),{bool_term(a),bool_term(b)}),bool_term(a||b)));
  rules.push_back(rule(apps(sym("resolveOne"),{pvar(0),sym("sameImageReference")}),
                       apps(sym("resolvedOption"),{sym("false"),pvar(0)})));
  rules.push_back(rule(apps(sym("resolveOne"),
      {apps(sym("imageSetOption"),{pvar(1),pvar(2),pvar(3)}),app(sym("replaceImageReference"),pvar(4))}),
      apps(sym("resolvedOption"),{sym("true"),apps(sym("imageSetOption"),{pvar(4),pvar(2),pvar(3)})})));
  rules.push_back(rule(app(sym("resolveOptions"),sym("nilInputOptions")),
                       apps(sym("resolvedOptions"),{sym("false"),sym("nilOptions")})));
  rules.push_back(rule(app(sym("resolveOptions"),
      apps(sym("consInputOption"),{apps(sym("inputOption"),{pvar(5),pvar(6)}),pvar(7)})),
      apps(sym("combineResolved"),{apps(sym("resolveOne"),{pvar(5),pvar(6)}),app(sym("resolveOptions"),pvar(7))})));
  rules.push_back(rule(apps(sym("combineResolved"),
      {apps(sym("resolvedOption"),{pvar(8),pvar(9)}),apps(sym("resolvedOptions"),{pvar(10),pvar(11)})}),
      apps(sym("resolvedOptions"),{apps(sym("orB"),{pvar(8),pvar(10)}),apps(sym("consOption"),{pvar(9),pvar(11)})})));
  rules.push_back(rule(app(sym("originalOptions"),sym("nilInputOptions")),sym("nilOptions")));
  rules.push_back(rule(app(sym("originalOptions"),
      apps(sym("consInputOption"),{apps(sym("inputOption"),{pvar(12),pvar(13)}),pvar(14)})),
      apps(sym("consOption"),{pvar(12),app(sym("originalOptions"),pvar(14))})));
  rules.push_back(rule(apps(sym("createCopyIfChanged"),{apps(sym("resolvedOptions"),{sym("false"),pvar(15)})}),sym("noCopy")));
  rules.push_back(rule(apps(sym("createCopyIfChanged"),{apps(sym("resolvedOptions"),{sym("true"),pvar(16)})}),app(sym("copyOptions"),pvar(16))));
  rules.push_back(rule(apps(sym("finishResolution"),{sym("noCopy"),pvar(17)}),app(sym("sameCSSImageSet"),pvar(17))));
  rules.push_back(rule(apps(sym("finishResolution"),{app(sym("copyOptions"),pvar(18)),pvar(19)}),app(sym("copiedCSSImageSet"),pvar(18))));
  rules.push_back(rule(app(sym("resolveValuesIfNeeded"),pvar(20)),
      apps(sym("finishResolution"),{app(sym("createCopyIfChanged"),app(sym("resolveOptions"),pvar(20))),app(sym("originalOptions"),pvar(20))})));

  RuleIndex indexed_rules(rules);
  const InputOptions sample={};
  auto sample_term=app(sym("resolveValuesIfNeeded"),input_term(sample));
  auto inferred=infer({},sample_term,signature,rules);
  assert(inferred&&conv(*inferred,V,indexed_rules));

  const std::vector<InputOptions> cases={
      {},
      {{{0,0,-1},true,0}},
      {{{0,0,-1},false,1}},
      {{{0,0,0},true,0},{{1,1,1},false,2}},
      {{{0,1,0},false,0},{{1,0,-1},true,0}},
      {{{0,0,-1},false,0},{{1,1,1},false,1}},
      {{{0,1,0},true,0},{{1,0,1},true,0}}
  };
  unsigned states=0;
  for(const auto& input:cases){
    const ResolveResult expected=cpp_resolve(input);
    Ptr term=app(sym("resolveValuesIfNeeded"),input_term(input));
    if(!conv(nf(term,indexed_rules),expected_term(input,expected),indexed_rules)){
      std::cerr<<"CSSImageSetValue::ResolveValuesIfNeeded mismatch\n";
      return 2;
    }
    ++states;
  }
  std::cout<<"CSSImageSetValue per-option gradient resolution and copy-or-original result cover "<<states<<" finite cases.\n";
  std::cout<<"Scope: ResolveGradients outputs are explicit same-reference/replacement observations; option order and resolution/type fields are preserved. Allocation, resolver internals, and pointer/GC identity beyond the copy-vs-original tag are omitted.\n";
}
