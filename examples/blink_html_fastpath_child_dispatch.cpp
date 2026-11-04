#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace lp;

enum class Parent { Flow, Phrasing, List, Select, Option };
enum class Result { Void, FlowContainer, PhrasingContainer, ListContainer,
                    SelectContainer, OptionContainer, Unsupported,
                    NestedAnchor, NestedListItem, OptionChild, SpecificChild };
static const std::vector<std::string> tags={"a","b","br","button","div","footer","i","input","li","label","option","ol","p","select","span","strong","ul","other"};
static Result general(const std::string& tag,bool nonphrasing,bool in_a,bool in_li){
  if(tag=="a")return in_a?Result::NestedAnchor:(nonphrasing?Result::FlowContainer:Result::PhrasingContainer);
  if(tag=="li")return in_li?Result::NestedListItem:(nonphrasing?Result::FlowContainer:Result::Unsupported);
  if(tag=="br"||tag=="input")return Result::Void;
  if(tag=="b"||tag=="button"||tag=="i"||tag=="label"||tag=="span"||tag=="strong")return Result::PhrasingContainer;
  if(tag=="select")return Result::SelectContainer;
  if(tag=="p")return nonphrasing?Result::PhrasingContainer:Result::Unsupported;
  if(tag=="div"||tag=="footer")return nonphrasing?Result::FlowContainer:Result::Unsupported;
  if(tag=="ol"||tag=="ul")return nonphrasing?Result::ListContainer:Result::Unsupported;
  return Result::Unsupported;
}
static Result cpp_dispatch(Parent parent,const std::string& tag,bool in_a,bool in_li){
  switch(parent){
    case Parent::Flow:return general(tag,true,in_a,in_li);
    case Parent::Phrasing:return general(tag,false,in_a,in_li);
    case Parent::List:return tag=="li"?Result::FlowContainer:Result::SpecificChild;
    case Parent::Select:return tag=="option"?Result::OptionContainer:Result::SpecificChild;
    case Parent::Option:return Result::OptionChild;
  }
  return Result::Unsupported;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bt(bool b){return sym(b?"true":"false");}
static Ptr parent_term(Parent p){switch(p){case Parent::Flow:return sym("flowParent");case Parent::Phrasing:return sym("phrasingParent");case Parent::List:return sym("listParent");case Parent::Select:return sym("selectParent");case Parent::Option:return sym("optionParent");}return sym("flowParent");}
static Ptr tag_term(const std::string& t){return sym("tag_"+t);}
static Ptr result_term(Result r){switch(r){case Result::Void:return sym("parseVoidElement");case Result::FlowContainer:return sym("parseFlowContainer");case Result::PhrasingContainer:return sym("parsePhrasingContainer");case Result::ListContainer:return sym("parseListContainer");case Result::SelectContainer:return sym("parseSelectContainer");case Result::OptionContainer:return sym("parseOptionContainer");case Result::Unsupported:return sym("rejectUnsupportedTag");case Result::NestedAnchor:return sym("rejectNestedAnchor");case Result::NestedListItem:return sym("rejectNestedListItem");case Result::OptionChild:return sym("rejectOptionElementChild");case Result::SpecificChild:return sym("rejectUnexpectedSpecificChild");}return sym("rejectUnsupportedTag");}
static Ptr query(Parent p,const std::string& tag,bool in_a,bool in_li){return apps(sym("dispatchChild"),{parent_term(p),tag_term(tag),bt(in_a),bt(in_li)});}

int main(){
  const Ptr B=sym("Bool"),T=sym("HtmlFastPathTag"),O=sym("ChildDispatchResult");Signature sig;std::vector<Rule> rules;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["HtmlFastPathTag"]=type_sort();for(const auto& t:tags)sig["tag_"+t]=T;
  sig["ParentTagKind"]=type_sort();for(const char* p:{"flowParent","phrasingParent","listParent","selectParent","optionParent"})sig[p]=sym("ParentTagKind");
  sig["ChildDispatchResult"]=type_sort();for(const char* o:{"parseVoidElement","parseFlowContainer","parsePhrasingContainer","parseListContainer","parseSelectContainer","parseOptionContainer","rejectUnsupportedTag","rejectNestedAnchor","rejectNestedListItem","rejectOptionElementChild","rejectUnexpectedSpecificChild"})sig[o]=O;
  sig["chooseResult"]=fn(B,fn(O,fn(O,O)));sig["dispatchGeneral"]=fn(B,fn(T,fn(B,fn(B,O))));sig["dispatchChild"]=fn(sym("ParentTagKind"),fn(T,fn(B,fn(B,O))));
  rules.push_back(rule(apps(sym("chooseResult"),{sym("true"),pvar(0),pvar(1)}),pvar(0)));
  rules.push_back(rule(apps(sym("chooseResult"),{sym("false"),pvar(0),pvar(1)}),pvar(1)));
  // <a> is valid in both child modes, but nesting it would trigger HTML auto-close.
  rules.push_back(rule(apps(sym("dispatchGeneral"),{pvar(2),tag_term("a"),sym("true"),pvar(3)}),sym("rejectNestedAnchor")));
  rules.push_back(rule(apps(sym("dispatchGeneral"),{sym("true"),tag_term("a"),sym("false"),pvar(4)}),sym("parseFlowContainer")));
  rules.push_back(rule(apps(sym("dispatchGeneral"),{sym("false"),tag_term("a"),sym("false"),pvar(5)}),sym("parsePhrasingContainer")));
  // <li> is flow-only and cannot nest through the generic ParseElement branch.
  rules.push_back(rule(apps(sym("dispatchGeneral"),{pvar(6),tag_term("li"),pvar(7),sym("true")}),sym("rejectNestedListItem")));
  rules.push_back(rule(apps(sym("dispatchGeneral"),{sym("true"),tag_term("li"),pvar(8),sym("false")}),sym("parseFlowContainer")));
  rules.push_back(rule(apps(sym("dispatchGeneral"),{sym("false"),tag_term("li"),pvar(9),sym("false")}),sym("rejectUnsupportedTag")));
  for(const char* t:{"br","input"})rules.push_back(rule(apps(sym("dispatchGeneral"),{pvar(10),tag_term(t),pvar(11),pvar(12)}),sym("parseVoidElement")));
  for(const char* t:{"b","button","i","label","span","strong"})rules.push_back(rule(apps(sym("dispatchGeneral"),{pvar(13),tag_term(t),pvar(14),pvar(15)}),sym("parsePhrasingContainer")));
  rules.push_back(rule(apps(sym("dispatchGeneral"),{pvar(16),tag_term("select"),pvar(17),pvar(18)}),sym("parseSelectContainer")));
  rules.push_back(rule(apps(sym("dispatchGeneral"),{sym("true"),tag_term("p"),pvar(19),pvar(20)}),sym("parsePhrasingContainer")));
  rules.push_back(rule(apps(sym("dispatchGeneral"),{sym("false"),tag_term("p"),pvar(21),pvar(22)}),sym("rejectUnsupportedTag")));
  for(const char* t:{"div","footer"}){
    rules.push_back(rule(apps(sym("dispatchGeneral"),{sym("true"),tag_term(t),pvar(23),pvar(24)}),sym("parseFlowContainer")));
    rules.push_back(rule(apps(sym("dispatchGeneral"),{sym("false"),tag_term(t),pvar(25),pvar(26)}),sym("rejectUnsupportedTag")));
  }
  for(const char* t:{"ol","ul"}){
    rules.push_back(rule(apps(sym("dispatchGeneral"),{sym("true"),tag_term(t),pvar(27),pvar(28)}),sym("parseListContainer")));
    rules.push_back(rule(apps(sym("dispatchGeneral"),{sym("false"),tag_term(t),pvar(29),pvar(30)}),sym("rejectUnsupportedTag")));
  }
  rules.push_back(rule(apps(sym("dispatchGeneral"),{pvar(31),tag_term("option"),pvar(32),pvar(33)}),sym("rejectUnsupportedTag")));
  rules.push_back(rule(apps(sym("dispatchGeneral"),{pvar(34),tag_term("other"),pvar(35),pvar(36)}),sym("rejectUnsupportedTag")));
  rules.push_back(rule(apps(sym("dispatchChild"),{sym("flowParent"),pvar(37),pvar(38),pvar(39)}),apps(sym("dispatchGeneral"),{sym("true"),pvar(37),pvar(38),pvar(39)})));
  rules.push_back(rule(apps(sym("dispatchChild"),{sym("phrasingParent"),pvar(40),pvar(41),pvar(42)}),apps(sym("dispatchGeneral"),{sym("false"),pvar(40),pvar(41),pvar(42)})));
  rules.push_back(rule(apps(sym("dispatchChild"),{sym("listParent"),tag_term("li"),pvar(43),pvar(44)}),sym("parseFlowContainer")));
  for(const auto& t:tags)if(t!="li")rules.push_back(rule(apps(sym("dispatchChild"),{sym("listParent"),tag_term(t),pvar(45),pvar(46)}),sym("rejectUnexpectedSpecificChild")));
  rules.push_back(rule(apps(sym("dispatchChild"),{sym("selectParent"),tag_term("option"),pvar(47),pvar(48)}),sym("parseOptionContainer")));
  for(const auto& t:tags)if(t!="option")rules.push_back(rule(apps(sym("dispatchChild"),{sym("selectParent"),tag_term(t),pvar(49),pvar(50)}),sym("rejectUnexpectedSpecificChild")));
  rules.push_back(rule(apps(sym("dispatchChild"),{sym("optionParent"),pvar(51),pvar(52),pvar(53)}),sym("rejectOptionElementChild")));
  RuleIndex index(rules);auto sample=query(Parent::Flow,"div",false,false);auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,O,index));
  unsigned states=0;
  for(Parent p:{Parent::Flow,Parent::Phrasing,Parent::List,Parent::Select,Parent::Option})for(const auto& t:tags)for(bool a:{false,true})for(bool li:{false,true}){
    if(!conv(nf(query(p,t,a,li),index),result_term(cpp_dispatch(p,t,a,li)),index)){
      std::cerr<<"HTML fast-path child dispatch mismatch: parent="<<static_cast<int>(p)<<" tag="<<t<<"\n";return 2;
    }
    ++states;
  }
  std::cout<<"HTML fast-path child grammar covers "<<states<<" finite parent/tag/nesting states.\n";
  std::cout<<"Scope: generic flow/phrasing dispatch, list/select specialized children, option text-only behavior, supported child categories, and nested a/li guards are modeled. Scanning, attributes, recursive child content, and DOM construction are abstracted.\n";
}
