#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace lp;

enum class End { Greater, SlashGreater, Malformed };
enum class Outcome { Accepted, EventHandlerRejected, AttributesRejected };
static Outcome cpp_parse_attributes(const std::vector<std::string>& names,End end){
  for(const auto& name:names){
    if(name.size()>2&&name[0]=='o'&&name[1]=='n')return Outcome::EventHandlerRejected;
    if(name=="is")return Outcome::AttributesRejected;
  }
  if(end==End::Malformed)return Outcome::AttributesRejected;
  std::set<std::string> seen;
  for(const auto& name:names)if(!seen.insert(name).second)return Outcome::AttributesRejected;
  return Outcome::Accepted;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bool_term(bool b){return sym(b?"true":"false");}
static Ptr outcome(Outcome o){switch(o){case Outcome::Accepted:return sym("attributesAccepted");case Outcome::EventHandlerRejected:return sym("rejectEventHandlerAttribute");case Outcome::AttributesRejected:return sym("rejectAttributes");}return sym("rejectAttributes");}
static const std::vector<std::string> names={"id","class","data-x","on","onload","is"};
static Ptr name_term(const std::string& n){return sym("name_"+n);}
static Ptr list_term(const std::vector<std::string>& xs){Ptr l=sym("nilNames");for(auto i=xs.rbegin();i!=xs.rend();++i)l=apps(sym("consName"),{name_term(*i),l});return l;}
static Ptr end_term(End e){return sym(e==End::Greater?"endGreater":e==End::SlashGreater?"endSlashGreater":"endMalformed");}
static Ptr query(const std::vector<std::string>& xs,End end){return apps(sym("parseAttributes"),{list_term(xs),end_term(end)});}
static void gen_lists(std::vector<std::vector<std::string>>& out,std::vector<std::string> prefix,unsigned depth){out.push_back(prefix);if(depth==0)return;for(const auto& n:names){auto next=prefix;next.push_back(n);gen_lists(out,std::move(next),depth-1);}}

int main(){
  const Ptr B=sym("Bool"),N=sym("AttributeName"),NL=sym("AttributeNameList"),E=sym("AttributeEnd"),S=sym("AttributeScan"),O=sym("AttributeParseResult");
  Signature sig;std::vector<Rule> rules;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["AttributeName"]=type_sort();for(const auto& n:names)sig["name_"+n]=N;
  sig["AttributeNameList"]=type_sort();sig["nilNames"]=NL;sig["consName"]=fn(N,fn(NL,NL));
  sig["AttributeEnd"]=type_sort();sig["endGreater"]=E;sig["endSlashGreater"]=E;sig["endMalformed"]=E;
  sig["AttributeScan"]=type_sort();sig["scanSafe"]=S;sig["scanEventRejected"]=S;sig["scanIsRejected"]=S;
  sig["AttributeParseResult"]=type_sort();sig["attributesAccepted"]=O;sig["rejectEventHandlerAttribute"]=O;sig["rejectAttributes"]=O;
  sig["nameEqual"]=fn(N,fn(N,B));sig["containsName"]=fn(N,fn(NL,B));sig["orBool"]=fn(B,fn(B,B));
  sig["hasDuplicate"]=fn(NL,B);sig["scanSpecialNames"]=fn(NL,S);sig["finishAttributes"]=fn(S,fn(E,fn(NL,O)));sig["finishDuplicateCheck"]=fn(B,O);sig["parseAttributes"]=fn(NL,fn(E,O));
  for(const auto& a:names)for(const auto& b:names)rules.push_back(rule(apps(sym("nameEqual"),{name_term(a),name_term(b)}),bool_term(a==b)));
  for(bool a:{false,true})for(bool b:{false,true})rules.push_back(rule(apps(sym("orBool"),{bool_term(a),bool_term(b)}),bool_term(a||b)));
  rules.push_back(rule(apps(sym("containsName"),{pvar(0),sym("nilNames")}),sym("false")));
  rules.push_back(rule(apps(sym("containsName"),{pvar(0),apps(sym("consName"),{pvar(1),pvar(2)})}),apps(sym("orBool"),{apps(sym("nameEqual"),{pvar(0),pvar(1)}),apps(sym("containsName"),{pvar(0),pvar(2)})})));
  rules.push_back(rule(app(sym("hasDuplicate"),sym("nilNames")),sym("false")));
  rules.push_back(rule(app(sym("hasDuplicate"),apps(sym("consName"),{pvar(3),pvar(4)})),apps(sym("orBool"),{apps(sym("containsName"),{pvar(3),pvar(4)}),app(sym("hasDuplicate"),pvar(4))})));
  rules.push_back(rule(app(sym("scanSpecialNames"),sym("nilNames")),sym("scanSafe")));
  for(const auto& n:names){
    Ptr tail=pvar(5);
    Ptr res=(n=="onload")?sym("scanEventRejected"):n=="is"?sym("scanIsRejected"):app(sym("scanSpecialNames"),tail);
    rules.push_back(rule(app(sym("scanSpecialNames"),apps(sym("consName"),{name_term(n),tail})),res));
  }
  rules.push_back(rule(apps(sym("finishDuplicateCheck"),{sym("true")}),sym("rejectAttributes")));
  rules.push_back(rule(apps(sym("finishDuplicateCheck"),{sym("false")}),sym("attributesAccepted")));
  rules.push_back(rule(apps(sym("finishAttributes"),{sym("scanEventRejected"),pvar(6),pvar(7)}),sym("rejectEventHandlerAttribute")));
  rules.push_back(rule(apps(sym("finishAttributes"),{sym("scanIsRejected"),pvar(8),pvar(9)}),sym("rejectAttributes")));
  rules.push_back(rule(apps(sym("finishAttributes"),{sym("scanSafe"),sym("endMalformed"),pvar(10)}),sym("rejectAttributes")));
  rules.push_back(rule(apps(sym("finishAttributes"),{sym("scanSafe"),sym("endGreater"),pvar(11)}),app(sym("finishDuplicateCheck"),app(sym("hasDuplicate"),pvar(11)))));
  rules.push_back(rule(apps(sym("finishAttributes"),{sym("scanSafe"),sym("endSlashGreater"),pvar(12)}),app(sym("finishDuplicateCheck"),app(sym("hasDuplicate"),pvar(12)))));
  rules.push_back(rule(apps(sym("parseAttributes"),{pvar(13),pvar(14)}),apps(sym("finishAttributes"),{app(sym("scanSpecialNames"),pvar(13)),pvar(14),pvar(13)})));
  RuleIndex index(rules);
  auto sample=query({},End::Greater);auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,O,index));
  std::vector<std::vector<std::string>> cases;gen_lists(cases,{},3);unsigned states=0;
  for(const auto& attrs:cases)for(End end:{End::Greater,End::SlashGreater,End::Malformed}){
    if(!conv(nf(query(attrs,end),index),outcome(cpp_parse_attributes(attrs,end)),index)){
      std::cerr<<"ParseAttributes decision mismatch\n";return 2;
    }
    ++states;
  }
  std::cout<<"HTML fast-path attribute rejection/acceptance covers "<<states<<" finite name-list/terminator states.\n";
  std::cout<<"Scope: canonical attribute-name atoms, ordered on*/is rejection, duplicate-name detection, and > or /> termination are modeled; name scanning, values, whitespace, and DOM assignment are abstracted.\n";
}
