#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

enum class Result { Allowed, ShadowRoots, Tracing, ContentPolicy, InForm };
static Result cpp_gate(bool shadow_roots, bool tracing, bool policy_allows,
                       bool template_context, bool has_form_ancestor) {
  if (shadow_roots) return Result::ShadowRoots;
  if (tracing) return Result::Tracing;
  if (!policy_allows) return Result::ContentPolicy;
  if (!template_context && has_form_ancestor) return Result::InForm;
  return Result::Allowed;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr boolean(bool b){return sym(b?"true":"false");}
static Ptr expected(Result r){switch(r){case Result::Allowed:return sym("fastPathAllowed");case Result::ShadowRoots:return sym("rejectShadowRoots");case Result::Tracing:return sym("rejectTracing");case Result::ContentPolicy:return sym("rejectContentPolicy");case Result::InForm:return sym("rejectFormContext");}return sym("fastPathAllowed");}
static Ptr query(bool shadow,bool tracing,bool policy,bool templ,bool form){
  Ptr form_block=app(app(sym("andBool"),app(sym("notBool"),boolean(templ))),boolean(form));
  return apps(sym("canUseFastPath"),{boolean(shadow),boolean(tracing),boolean(policy),form_block});
}

int main(){
  const Ptr B=sym("Bool"),O=sym("FastPathDecision"); Signature sig; std::vector<Rule> rules;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["FastPathDecision"]=type_sort();
  for(const char* x:{"fastPathAllowed","rejectShadowRoots","rejectTracing","rejectContentPolicy","rejectFormContext"})sig[x]=O;
  sig["notBool"]=fn(B,B);sig["andBool"]=fn(B,fn(B,B));
  sig["chooseDecision"]=fn(B,fn(O,fn(O,O)));
  sig["canUseFastPath"]=fn(B,fn(B,fn(B,fn(B,O))));
  rules.push_back(rule(app(sym("notBool"),sym("true")),sym("false")));
  rules.push_back(rule(app(sym("notBool"),sym("false")),sym("true")));
  for(bool a:{false,true})for(bool b:{false,true})
    rules.push_back(rule(apps(sym("andBool"),{boolean(a),boolean(b)}),boolean(a&&b)));
  rules.push_back(rule(apps(sym("chooseDecision"),{sym("true"),pvar(0),pvar(1)}),pvar(0)));
  rules.push_back(rule(apps(sym("chooseDecision"),{sym("false"),pvar(0),pvar(1)}),pvar(1)));
  rules.push_back(rule(apps(sym("canUseFastPath"),{pvar(0),pvar(1),pvar(2),pvar(3)}),
    apps(sym("chooseDecision"),{pvar(0),sym("rejectShadowRoots"),
      apps(sym("chooseDecision"),{pvar(1),sym("rejectTracing"),
        apps(sym("chooseDecision"),{app(sym("notBool"),pvar(2)),sym("rejectContentPolicy"),
          apps(sym("chooseDecision"),{pvar(3),sym("rejectFormContext"),sym("fastPathAllowed")})})})})));
  RuleIndex index(rules);
  auto sample=query(false,false,true,false,false);
  auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,O,index));
  unsigned states=0;
  for(bool shadow:{false,true})for(bool tracing:{false,true})for(bool policy:{false,true})
    for(bool templ:{false,true})for(bool form:{false,true}){
      Ptr term=query(shadow,tracing,policy,templ,form);
      if(!conv(nf(term,index),expected(cpp_gate(shadow,tracing,policy,templ,form)),index)){
        std::cerr<<"CanUseFastPath decision mismatch\n";return 2;
      }
      ++states;
    }
  std::cout<<"HTML fragment fast-path eligibility covers "<<states<<" finite guard states.\n";
  std::cout<<"Scope: ordered shadow-root, tracing, content-policy, and form-ancestor guards with the template exception. Supported context tags, fragment parsing, and DOM effects are outside this model.\n";
}
