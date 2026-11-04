#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace lp;

enum class Namespace { Html, Foreign };
enum class Outcome { BodyKeep, BodyStrip, ParseContainer, Reject };
struct TagInfo { const char* name; bool is_void; };
static const std::vector<TagInfo> supported={
  {"a",false},{"b",false},{"br",true},{"button",false},{"div",false},
  {"footer",false},{"i",false},{"input",true},{"li",false},{"label",false},
  {"option",false},{"ol",false},{"p",false},{"select",false},
  {"span",false},{"strong",false},{"ul",false}
};
static Outcome cpp_context(const std::string& tag,Namespace ns,bool strip){
  if(ns!=Namespace::Html)return Outcome::Reject;
  if(tag=="body")return strip?Outcome::BodyStrip:Outcome::BodyKeep;
  for(const auto& info:supported)if(tag==info.name)return info.is_void?Outcome::Reject:Outcome::ParseContainer;
  return Outcome::Reject;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr result(Outcome o){switch(o){case Outcome::BodyKeep:return sym("bodyContextKeepWhitespace");case Outcome::BodyStrip:return sym("bodyContextStripWhitespace");case Outcome::ParseContainer:return sym("parseContainerContext");case Outcome::Reject:return sym("rejectUnsupportedContext");}return sym("rejectUnsupportedContext");}
static Ptr tag_term(const std::string& tag){return sym(tag=="body"?"tag_body":(std::find_if(supported.begin(),supported.end(),[&](const TagInfo&i){return tag==i.name;})!=supported.end()?"tag_"+tag:"otherTag"));}
static Ptr query(const std::string& tag,Namespace ns,bool strip){return apps(sym("runContext"),{tag_term(tag),sym(ns==Namespace::Html?"htmlNamespace":"foreignNamespace"),sym(strip?"stripWhitespace":"keepWhitespace")});}

int main(){
  const Ptr T=sym("ContextTag"),N=sym("Namespace"),W=sym("BodyWhitespaceBehavior"),O=sym("ContextDispatchResult");
  Signature sig;std::vector<Rule> rules;
  sig["ContextTag"]=type_sort();sig["Namespace"]=type_sort();sig["BodyWhitespaceBehavior"]=type_sort();sig["ContextDispatchResult"]=type_sort();
  sig["htmlNamespace"]=N;sig["foreignNamespace"]=N;sig["stripWhitespace"]=W;sig["keepWhitespace"]=W;
  sig["bodyContextKeepWhitespace"]=O;sig["bodyContextStripWhitespace"]=O;sig["parseContainerContext"]=O;sig["rejectUnsupportedContext"]=O;
  sig["runContext"]=fn(T,fn(N,fn(W,O)));
  sig["tag_body"]=T;rules.push_back(rule(apps(sym("runContext"),{sym("tag_body"),sym("htmlNamespace"),sym("stripWhitespace")}),sym("bodyContextStripWhitespace")));
  rules.push_back(rule(apps(sym("runContext"),{sym("tag_body"),sym("htmlNamespace"),sym("keepWhitespace")}),sym("bodyContextKeepWhitespace")));
  std::vector<std::string> known={"body"};
  for(const auto& info:supported){
    const std::string term="tag_"+std::string(info.name);known.push_back(info.name);sig[term]=T;
    Ptr rhs=info.is_void?sym("rejectUnsupportedContext"):sym("parseContainerContext");
    rules.push_back(rule(apps(sym("runContext"),{sym(term),sym("htmlNamespace"),pvar(0)}),rhs));
  }
  sig["otherTag"]=T;
  rules.push_back(rule(apps(sym("runContext"),{pvar(1),sym("foreignNamespace"),pvar(2)}),sym("rejectUnsupportedContext")));
  rules.push_back(rule(apps(sym("runContext"),{sym("otherTag"),sym("htmlNamespace"),pvar(3)}),sym("rejectUnsupportedContext")));
  RuleIndex index(rules);
  auto sample=query("body",Namespace::Html,false);
  auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,O,index));
  unsigned states=0;
  for(const std::string& tag:known)for(Namespace ns:{Namespace::Html,Namespace::Foreign})for(bool strip:{false,true}){
    if(!conv(nf(query(tag,ns,strip),index),result(cpp_context(tag,ns,strip)),index)){
      std::cerr<<"HTML fast-path context dispatch mismatch for "<<tag<<"\n";return 2;
    }
    ++states;
  }
  for(Namespace ns:{Namespace::Html,Namespace::Foreign})for(bool strip:{false,true}){
    if(!conv(nf(query("custom-unknown",ns,strip),index),result(cpp_context("custom-unknown",ns,strip)),index)){
      std::cerr<<"HTML fast-path unsupported context mismatch\n";return 3;
    }
    ++states;
  }
  std::cout<<"HTML fragment fast-path context dispatch covers "<<states<<" finite tag/namespace/behavior states (17 supported tags plus body).\n";
  std::cout<<"Scope: exact supported tag classes, HTML versus foreign namespace, body whitespace behavior, and unsupported-context rejection are modeled; hashing, element creation, child parsing, and DOM effects are abstracted.\n";
}
