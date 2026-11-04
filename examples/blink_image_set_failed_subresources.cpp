#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

enum class CachedState { NoStyleImage, NoResourceContent, Loaded, FailedOrCanceled };
static bool cpp_failed(bool fetcher_present, bool identity_check,
                       unsigned fetcher, CachedState shared,
                       CachedState keyed_a, CachedState keyed_b) {
  const bool use_keyed = fetcher_present && identity_check;
  CachedState selected = use_keyed
      ? (fetcher == 0 ? keyed_a : keyed_b) : shared;
  if (selected == CachedState::NoStyleImage) return false;
  return selected != CachedState::Loaded;
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs),std::move(rhs)}; }
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bool_term(bool b){return sym(b?"true":"false");}
static Ptr state_term(CachedState s){switch(s){case CachedState::NoStyleImage:return sym("noStyleImage");case CachedState::NoResourceContent:return sym("noResourceContent");case CachedState::Loaded:return sym("loadedContent");case CachedState::FailedOrCanceled:return sym("failedContent");}return sym("noStyleImage");}
static Ptr query_term(bool present,bool gate,unsigned fetcher,
                      CachedState shared,CachedState a,CachedState b){
  const bool use_keyed=present&&gate;
  return apps(sym("hasFailedSubresources"),{sym(use_keyed?"fetcherCache":"sharedCache"),bool_term(present),bool_term(gate),sym(fetcher==0?"fetcherA":"fetcherB"),state_term(shared),state_term(a),state_term(b)});
}

int main(){
  const Ptr B=sym("Bool"),M=sym("CacheMode"),F=sym("Fetcher"),S=sym("CachedStyleImageState"),R=sym("FailureResult");
  Signature sig;std::vector<Rule> rules;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["CacheMode"]=type_sort();sig["sharedCache"]=M;sig["fetcherCache"]=M;
  sig["Fetcher"]=type_sort();sig["fetcherA"]=F;sig["fetcherB"]=F;
  sig["CachedStyleImageState"]=type_sort();for(const char* x:{"noStyleImage","noResourceContent","loadedContent","failedContent"})sig[x]=S;
  sig["FailureResult"]=type_sort();sig["hasFailedSubresources"]=fn(M,fn(B,fn(B,fn(F,fn(S,fn(S,fn(S,R)))))));
  sig["selectCachedState"]=fn(M,fn(B,fn(B,fn(F,fn(S,fn(S,fn(S,S)))))));
  sig["isFailedContentState"]=fn(S,B);
  for(bool p:{false,true})for(bool g:{false,true}){
    // A null fetcher or disabled feature selects the shared cache.
    const bool keyed=p&&g;
    for(unsigned f=0;f<2;++f){
      Ptr lhs=apps(sym("selectCachedState"),{sym("fetcherCache"),bool_term(p),bool_term(g),sym(f==0?"fetcherA":"fetcherB"),pvar(0),pvar(1),pvar(2)});
      rules.push_back(rule(lhs,keyed?pvar(f==0?1:2):pvar(0)));
    }
  }
  rules.push_back(rule(apps(sym("selectCachedState"),{sym("sharedCache"),pvar(3),pvar(4),pvar(5),pvar(6),pvar(7),pvar(8)}),pvar(6)));
  rules.push_back(rule(app(sym("isFailedContentState"),sym("noStyleImage")),sym("false")));
  rules.push_back(rule(app(sym("isFailedContentState"),sym("noResourceContent")),sym("true")));
  rules.push_back(rule(app(sym("isFailedContentState"),sym("loadedContent")),sym("false")));
  rules.push_back(rule(app(sym("isFailedContentState"),sym("failedContent")),sym("true")));
  rules.push_back(rule(apps(sym("hasFailedSubresources"),{pvar(9),pvar(10),pvar(11),pvar(12),pvar(13),pvar(14),pvar(15)}),
      app(sym("isFailedContentState"),apps(sym("selectCachedState"),{pvar(9),pvar(10),pvar(11),pvar(12),pvar(13),pvar(14),pvar(15)}))));
  RuleIndex index(rules);
  auto sample=query_term(false,false,0,CachedState::NoStyleImage,CachedState::Loaded,CachedState::Loaded);
  auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,R,index));
  unsigned states=0;
  for(bool present:{false,true})for(bool gate:{false,true})for(unsigned fetcher=0;fetcher<2;++fetcher)
      for(unsigned x=0;x<4;++x)for(unsigned a=0;a<4;++a)for(unsigned b=0;b<4;++b){
        auto shared=static_cast<CachedState>(x),ka=static_cast<CachedState>(a),kb=static_cast<CachedState>(b);
        auto term=query_term(present,gate,fetcher,shared,ka,kb);
        if(!conv(nf(term,index),bool_term(cpp_failed(present,gate,fetcher,shared,ka,kb)),index)){
          std::cerr<<"HasFailedOrCanceledSubresources mismatch\n";return 2;
        }
        ++states;
      }
  std::cout<<"CSSImageSetValue::HasFailedOrCanceledSubresources covers "<<states<<" finite cache/status states.\n";
  std::cout<<"Scope: cache mode, fetcher presence/identity feature, selected cached style-image state, and resource-content status are modeled; fetcher keys, DSF, resource loading, and StyleImage internals are abstracted.\n";
}
