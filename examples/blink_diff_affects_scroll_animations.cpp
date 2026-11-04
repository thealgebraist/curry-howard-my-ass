#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace lp;

struct ScrollTimelineDiffs {
  bool scroll_timeline_name, scroll_timeline_axis;
  bool view_timeline_name, view_timeline_axis, view_timeline_inset;
  bool timeline_scope;
};
static bool cpp_diff_affects_scroll_animations(const ScrollTimelineDiffs& d) {
  return d.scroll_timeline_name || d.scroll_timeline_axis ||
         d.view_timeline_name || d.view_timeline_axis ||
         d.view_timeline_inset || d.timeline_scope;
}
enum class Difference : unsigned { NonInherited, DescendantAffecting };
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr ap(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bt(bool b){return sym(b?"true":"false");}
static Ptr diff_term(Difference d){return sym(d==Difference::NonInherited?"dNonInherited":"dDescendant");}
static Ptr repeated_type(Ptr domain,Ptr result,unsigned arity){for(unsigned i=0;i<arity;++i)result=fn(domain,result);return result;}
int main(){
  Signature sig;std::vector<Rule> rules;Ptr B=sym("Bool"),D=sym("Difference");
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["Difference"]=type_sort();sig["dNonInherited"]=D;sig["dDescendant"]=D;
  sig["scrollAffected"]=repeated_type(B,B,6);
  sig["routeScrollDiff"]=fn(B,D);
  sig["composeScrollClassifier"]=repeated_type(B,D,6);

  auto add_scroll_pattern=[&](unsigned changed_field){
    std::vector<Ptr> args;
    for(unsigned i=0;i<6;++i)args.push_back(i==changed_field?sym("true"):pvar(i));
    rules.push_back(R(ap(sym("scrollAffected"),args),sym("true")));
  };
  for(unsigned i=0;i<6;++i)add_scroll_pattern(i);
  std::vector<Ptr> no_diffs, any_diffs;
  for(unsigned i=0;i<6;++i){no_diffs.push_back(sym("false"));any_diffs.push_back(pvar(i));}
  rules.push_back(R(ap(sym("scrollAffected"),any_diffs),sym("false")));
  rules.push_back(R(app(sym("routeScrollDiff"),sym("true")),sym("dDescendant")));
  rules.push_back(R(app(sym("routeScrollDiff"),sym("false")),sym("dNonInherited")));
  std::vector<Ptr> vars;for(unsigned i=0;i<6;++i)vars.push_back(pvar(i));
  rules.push_back(R(ap(sym("composeScrollClassifier"),vars),
                     app(sym("routeScrollDiff"),ap(sym("scrollAffected"),vars))));
  RuleIndex ix(rules);
  Ptr sample=ap(sym("scrollAffected"),no_diffs);auto ty=infer({},sample,sig,rules);assert(ty&&conv(*ty,B,ix));
  std::vector<Ptr> comp_sample_args(6,sym("false"));
  auto comp_ty=infer({},ap(sym("composeScrollClassifier"),comp_sample_args),sig,rules);assert(comp_ty&&conv(*comp_ty,D,ix));

  unsigned predicate_cases=0,composition_cases=0;
  for(unsigned mask=0;mask<64;++mask){
    bool b[6];std::vector<Ptr> args;for(unsigned i=0;i<6;++i){b[i]=(mask&(1u<<i))!=0;args.push_back(bt(b[i]));}
    ScrollTimelineDiffs diffs{b[0],b[1],b[2],b[3],b[4],b[5]};
    bool expected=cpp_diff_affects_scroll_animations(diffs);
    if(!conv(nf(ap(sym("scrollAffected"),args),ix),bt(expected),ix)){std::cerr<<"scroll animation predicate mismatch at mask "<<mask<<"\n";return 2;}
    ++predicate_cases;
    Difference routed=expected?Difference::DescendantAffecting:Difference::NonInherited;
    if(!conv(nf(nf(nf(ap(sym("composeScrollClassifier"),args),ix),ix),ix),diff_term(routed),ix)){std::cerr<<"scroll classifier composition mismatch at mask "<<mask<<"\n";return 3;}
    ++composition_cases;
  }
  std::cout<<"DiffAffectsScrollAnimations agrees on "<<predicate_cases<<" timeline-field comparison states.\n";
  std::cout<<"Composed first-priority ComputeDifference route agrees on "<<composition_cases<<" baseline style states.\n";
  std::cout<<"Scope: each input represents one old/new field inequality; timeline values and equality mechanics remain opaque.\n";
}
