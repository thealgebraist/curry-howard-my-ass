#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace lp;

struct Option {
  unsigned image;
  unsigned resolution;
  bool type_supported;
};
using Options = std::vector<Option>;

static bool supported(const Option& option) {
  return option.type_supported && option.resolution > 0;
}
static Options cpp_filter(const Options& options) {
  Options result;
  for (const auto& option : options) if (supported(option)) result.push_back(option);
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
static int cpp_best_option(const Options& options, unsigned device_scale) {
  Options filtered = cpp_filter(options);
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
static Ptr option_term(const Option& option) {
  return apps(sym("option"), {sym(("image" + std::to_string(option.image)).c_str()),
      resolution_term(option.resolution), bool_term(option.type_supported)});
}
static Ptr options_term(const Options& options) {
  Ptr result = sym("nilOptions");
  for (auto it = options.rbegin(); it != options.rend(); ++it)
    result = apps(sym("consOption"), {option_term(*it), result});
  return result;
}
static Ptr best_term(const Options& options, unsigned scale) {
  return apps(sym("bestOption"), {options_term(options), scale_term(scale)});
}

int main() {
  const Ptr B=sym("Bool"), R=sym("Resolution"), D=sym("DeviceScale");
  const Ptr I=sym("ImageId"), O=sym("ImageSetOption"), L=sym("OptionList");
  Signature signature;
  std::vector<Rule> rules;
  signature["Bool"]=type_sort(); signature["true"]=B; signature["false"]=B;
  signature["Resolution"]=type_sort();
  for(unsigned i=0;i<=3;++i) signature["r"+std::to_string(i)]=R;
  signature["DeviceScale"]=type_sort();
  for(unsigned i=1;i<=4;++i) signature["scale"+std::to_string(i)]=D;
  signature["ImageId"]=type_sort();
  for(unsigned i=0;i<6;++i) signature["image"+std::to_string(i)]=I;
  signature["ImageSetOption"]=type_sort();
  signature["option"]=fn(I,fn(R,fn(B,O)));
  signature["OptionList"]=type_sort(); signature["nilOptions"]=L;
  signature["consOption"]=fn(O,fn(L,L));
  signature["SelectedOption"]=type_sort(); signature["noOption"]=sym("SelectedOption");
  signature["someOption"]=fn(O,sym("SelectedOption"));

  signature["andB"]=fn(B,fn(B,B)); signature["notB"]=fn(B,B);
  signature["positiveResolution"]=fn(R,B);
  signature["supportedOption"]=fn(O,B);
  signature["filterOptions"]=fn(L,L);
  signature["filterStep"]=fn(B,fn(O,fn(L,L)));
  signature["lessResolution"]=fn(R,fn(R,B));
  signature["equalResolution"]=fn(R,fn(R,B));
  signature["optionResolution"]=fn(O,R);
  signature["insertOption"]=fn(O,fn(L,L));
  signature["insertStep"]=fn(B,fn(O,fn(O,fn(L,L))));
  signature["sortOptions"]=fn(L,L);
  signature["dropResolution"]=fn(R,fn(L,L));
  signature["uniqueOptions"]=fn(L,L);
  signature["atLeastScale"]=fn(R,fn(D,B));
  signature["selectOption"]=fn(D,fn(L,fn(sym("SelectedOption"),sym("SelectedOption"))));
  signature["selectStep"]=fn(B,fn(D,fn(O,fn(L,fn(sym("SelectedOption"),sym("SelectedOption"))))));
  signature["bestOption"]=fn(L,fn(D,sym("SelectedOption")));

  for(bool a:{false,true}) for(bool b:{false,true})
    rules.push_back(rule(apps(sym("andB"),{bool_term(a),bool_term(b)}),bool_term(a&&b)));
  for(unsigned r=0;r<=3;++r)
    rules.push_back(rule(app(sym("positiveResolution"),resolution_term(r)),bool_term(r>0)));
  rules.push_back(rule(apps(sym("supportedOption"),
      {apps(sym("option"),{pvar(0),pvar(1),pvar(2)})}),
      apps(sym("andB"),{pvar(2),app(sym("positiveResolution"),pvar(1))})));

  rules.push_back(rule(app(sym("filterOptions"),sym("nilOptions")),sym("nilOptions")));
  rules.push_back(rule(app(sym("filterOptions"),
      apps(sym("consOption"),{pvar(10),pvar(11)})),
      apps(sym("filterStep"),{app(sym("supportedOption"),pvar(10)),pvar(10),
                                app(sym("filterOptions"),pvar(11))})));
  rules.push_back(rule(apps(sym("filterStep"),{sym("true"),pvar(12),pvar(13)}),
                       apps(sym("consOption"),{pvar(12),pvar(13)})));
  rules.push_back(rule(apps(sym("filterStep"),{sym("false"),pvar(14),pvar(15)}),pvar(15)));

  for(unsigned a=0;a<=3;++a) for(unsigned b=0;b<=3;++b){
    rules.push_back(rule(apps(sym("lessResolution"),{resolution_term(a),resolution_term(b)}),bool_term(a<b)));
    rules.push_back(rule(apps(sym("equalResolution"),{resolution_term(a),resolution_term(b)}),bool_term(a==b)));
  }
  rules.push_back(rule(apps(sym("insertOption"),{pvar(20),sym("nilOptions")}),
                       apps(sym("consOption"),{pvar(20),sym("nilOptions")})));
  rules.push_back(rule(apps(sym("insertOption"),
      {pvar(21),apps(sym("consOption"),{pvar(22),pvar(23)})}),
      apps(sym("insertStep"),
        {apps(sym("lessResolution"),{app(sym("optionResolution"),pvar(21)),
                                    app(sym("optionResolution"),pvar(22))}),pvar(21),pvar(22),pvar(23)})));
  rules.push_back(rule(apps(sym("optionResolution"),{apps(sym("option"),{pvar(24),pvar(25),pvar(26)})}),pvar(25)));
  rules.push_back(rule(apps(sym("insertStep"),{sym("true"),pvar(27),pvar(28),pvar(29)}),
                       apps(sym("consOption"),{pvar(27),apps(sym("consOption"),{pvar(28),pvar(29)})})));
  rules.push_back(rule(apps(sym("insertStep"),{sym("false"),pvar(30),pvar(31),pvar(32)}),
                       apps(sym("consOption"),{pvar(31),apps(sym("insertOption"),{pvar(30),pvar(32)})})));
  rules.push_back(rule(app(sym("sortOptions"),sym("nilOptions")),sym("nilOptions")));
  rules.push_back(rule(app(sym("sortOptions"),
      apps(sym("consOption"),{pvar(33),pvar(34)})),
      apps(sym("insertOption"),{pvar(33),app(sym("sortOptions"),pvar(34))})));

  rules.push_back(rule(apps(sym("dropResolution"),{pvar(40),sym("nilOptions")}),sym("nilOptions")));
  rules.push_back(rule(apps(sym("dropResolution"),
      {pvar(41),apps(sym("consOption"),{apps(sym("option"),{pvar(42),pvar(43),pvar(44)}),pvar(45)})}),
      apps(sym("dropResolutionStep"),
        {apps(sym("equalResolution"),{pvar(41),pvar(43)}),pvar(41),
         apps(sym("option"),{pvar(42),pvar(43),pvar(44)}),pvar(45)})));
  signature["dropResolutionStep"]=fn(B,fn(R,fn(O,fn(L,L))));
  rules.push_back(rule(apps(sym("dropResolutionStep"),{sym("true"),pvar(46),pvar(47),pvar(48)}),
                       apps(sym("dropResolution"),{pvar(46),pvar(48)})));
  rules.push_back(rule(apps(sym("dropResolutionStep"),{sym("false"),pvar(49),pvar(50),pvar(51)}),
                       apps(sym("consOption"),{pvar(50),apps(sym("dropResolution"),{pvar(49),pvar(51)})})));
  rules.push_back(rule(app(sym("uniqueOptions"),sym("nilOptions")),sym("nilOptions")));
  rules.push_back(rule(app(sym("uniqueOptions"),
      apps(sym("consOption"),{apps(sym("option"),{pvar(52),pvar(53),pvar(54)}),pvar(55)})),
      apps(sym("consOption"),
        {apps(sym("option"),{pvar(52),pvar(53),pvar(54)}),
         app(sym("uniqueOptions"),apps(sym("dropResolution"),{pvar(53),pvar(55)}))})));

  for(unsigned r=0;r<=3;++r) for(unsigned d=1;d<=4;++d)
    rules.push_back(rule(apps(sym("atLeastScale"),{resolution_term(r),scale_term(d)}),bool_term(r>=d)));
  rules.push_back(rule(apps(sym("selectOption"),{pvar(60),sym("nilOptions"),pvar(61)}),pvar(61)));
  rules.push_back(rule(apps(sym("selectOption"),
      {pvar(62),apps(sym("consOption"),{apps(sym("option"),{pvar(63),pvar(64),pvar(65)}),pvar(66)}),pvar(67)}),
      apps(sym("selectStep"),{apps(sym("atLeastScale"),{pvar(64),pvar(62)}),pvar(62),
        apps(sym("option"),{pvar(63),pvar(64),pvar(65)}),pvar(66),pvar(67)})));
  rules.push_back(rule(apps(sym("selectStep"),{sym("true"),pvar(68),pvar(69),pvar(70),pvar(71)}),
                       app(sym("someOption"),pvar(69))));
  rules.push_back(rule(apps(sym("selectStep"),{sym("false"),pvar(72),pvar(73),pvar(74),pvar(75)}),
                       apps(sym("selectOption"),{pvar(72),pvar(74),app(sym("someOption"),pvar(73))})));
  rules.push_back(rule(apps(sym("bestOption"),{pvar(80),pvar(81)}),
      apps(sym("selectOption"),{pvar(81),
        app(sym("uniqueOptions"),app(sym("sortOptions"),app(sym("filterOptions"),pvar(80)))),
        sym("noOption")})));

  RuleIndex indexed_rules(rules);
  const Options empty;
  auto sample=best_term(empty,1);
  auto inferred=infer({},sample,signature,rules);
  assert(inferred&&conv(*inferred,sym("SelectedOption"),indexed_rules));

  const std::vector<Options> cases={
    {},
    {{0,1,true}},
    {{1,2,true},{2,1,true}},
    {{3,1,true},{4,1,true},{5,2,true}},
    {{0,0,true},{1,2,false}},
    {{0,2,false},{1,1,true},{2,3,true}},
    {{0,3,true},{1,1,true},{2,2,true}},
    {{0,1,false},{1,2,false}}
  };
  unsigned states=0;
  for(const auto& options:cases) for(unsigned scale=1;scale<=4;++scale){
    const int expected=cpp_best_option(options,scale);
    Ptr result=best_term(options,scale);
    Ptr expected_term=expected<0?sym("noOption"):app(sym("someOption"),
        option_term(options[0]));
    if(expected>=0){
      bool found=false;
      for(const auto& option:options)if(static_cast<int>(option.image)==expected){expected_term=app(sym("someOption"),option_term(option));found=true;break;}
      assert(found);
    }
    if(!conv(nf(result,indexed_rules),expected_term,indexed_rules)){
      std::cerr<<"CSSImageSetValue::GetBestOption mismatch at device scale "<<scale<<"\n";
      return 2;
    }
    ++states;
  }
  std::cout<<"CSSImageSetValue filtering, stable resolution ordering/deduplication, and device-scale selection cover "<<states<<" finite states.\n";
  std::cout<<"Scope: resolution values are 0..3, device scales 1..4, MIME support is Boolean, and each fixture has at most three options; lazy options_ cache mutation and resolver-dependent numeric computation are omitted.\n";
}
