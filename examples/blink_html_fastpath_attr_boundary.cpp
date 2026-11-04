#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace lp;

enum class Next { End, Delimiter, Invalid, UpperOrDigit };
enum class Kind { EndFailure, NameAtDelimiter, NameBeforeInvalid, MappedName };
struct Result { Kind kind; std::string name; };
static bool fast_char(char c){return (c>='a'&&c<='z')||c=='-';}
static bool all_fast(const std::string& s){for(char c:s)if(!fast_char(c))return false;return true;}
static std::string lower_ascii(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c=static_cast<char>(c+('a'-'A'));return s;}
static Result cpp_scan(const std::string& prefix,Next next,const std::string& full_name){
  switch(next){case Next::End:return all_fast(prefix)?Result{Kind::EndFailure,{}}:Result{Kind::MappedName,lower_ascii(full_name)};case Next::Delimiter:return {Kind::NameAtDelimiter,prefix};case Next::Invalid:return {Kind::NameBeforeInvalid,prefix};case Next::UpperOrDigit:return {Kind::MappedName,lower_ascii(full_name)};}
  return {Kind::EndFailure,{}};
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bool_term(bool b){return sym(b?"true":"false");}
static Ptr char_term(char c){return sym(std::string("ch_")+c);}
static Ptr chars(const std::string& s){Ptr t=sym("nilChars");for(auto i=s.rbegin();i!=s.rend();++i)t=apps(sym("consChar"),{char_term(*i),t});return t;}
static Ptr next_term(Next n){switch(n){case Next::End:return sym("atEnd");case Next::Delimiter:return sym("atDelimiter");case Next::Invalid:return sym("atInvalidChar");case Next::UpperOrDigit:return sym("atValidSlowPathChar");}return sym("atEnd");}
static Ptr query(const std::string& prefix,Next next,const std::string& full){return apps(sym("scanNameFrontier"),{chars(prefix),next_term(next),chars(full)});}
static Ptr expected(const Result& r){switch(r.kind){case Kind::EndFailure:return sym("nameScanEndFailure");case Kind::NameAtDelimiter:return app(sym("nameStopsAtDelimiter"),chars(r.name));case Kind::NameBeforeInvalid:return app(sym("nameStopsBeforeInvalid"),chars(r.name));case Kind::MappedName:return app(sym("nameMappedByFullScan"),chars(r.name));}return sym("nameScanEndFailure");}

int main(){
  const Ptr B=sym("Bool"),C=sym("NameChar"),CL=sym("NameChars"),N=sym("NextNameClass"),R=sym("NameFrontierResult");Signature sig;std::vector<Rule> rules;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["NameChar"]=type_sort();for(char c='a';c<='z';++c)sig[std::string("ch_")+c]=C;for(char c='A';c<='Z';++c)sig[std::string("ch_")+c]=C;for(char c='0';c<='9';++c)sig[std::string("ch_")+c]=C;sig["ch_-"]=C;
  sig["NameChars"]=type_sort();sig["nilChars"]=CL;sig["consChar"]=fn(C,fn(CL,CL));
  sig["NextNameClass"]=type_sort();sig["atEnd"]=N;sig["atDelimiter"]=N;sig["atInvalidChar"]=N;sig["atValidSlowPathChar"]=N;
  sig["NameFrontierResult"]=type_sort();sig["nameScanEndFailure"]=R;sig["nameStopsAtDelimiter"]=fn(CL,R);sig["nameStopsBeforeInvalid"]=fn(CL,R);sig["nameMappedByFullScan"]=fn(CL,R);
  sig["isFast"]=fn(C,B);sig["boolAnd"]=fn(B,fn(B,B));sig["allFast"]=fn(CL,B);sig["scanAtEnd"]=fn(B,fn(CL,fn(CL,R)));
  sig["lowerChar"]=fn(C,C);sig["lowerName"]=fn(CL,CL);sig["scanNameFrontier"]=fn(CL,fn(N,fn(CL,R)));
  for(char c='a';c<='z';++c)rules.push_back(rule(app(sym("isFast"),char_term(c)),sym("true")));
  for(char c='A';c<='Z';++c)rules.push_back(rule(app(sym("isFast"),char_term(c)),sym("false")));
  for(char c='0';c<='9';++c)rules.push_back(rule(app(sym("isFast"),char_term(c)),sym("false")));
  rules.push_back(rule(app(sym("isFast"),char_term('-')),sym("true")));
  for(bool a:{false,true})for(bool b:{false,true})rules.push_back(rule(apps(sym("boolAnd"),{bool_term(a),bool_term(b)}),bool_term(a&&b)));
  rules.push_back(rule(app(sym("allFast"),sym("nilChars")),sym("true")));
  rules.push_back(rule(app(sym("allFast"),apps(sym("consChar"),{pvar(10),pvar(11)})),apps(sym("boolAnd"),{app(sym("isFast"),pvar(10)),app(sym("allFast"),pvar(11))})));
  for(char c='a';c<='z';++c)rules.push_back(rule(app(sym("lowerChar"),char_term(c)),char_term(c)));
  for(char c='A';c<='Z';++c)rules.push_back(rule(app(sym("lowerChar"),char_term(c)),char_term(static_cast<char>(c+('a'-'A')))));
  for(char c='0';c<='9';++c)rules.push_back(rule(app(sym("lowerChar"),char_term(c)),char_term(c)));
  rules.push_back(rule(app(sym("lowerChar"),char_term('-')),char_term('-')));
  rules.push_back(rule(app(sym("lowerName"),sym("nilChars")),sym("nilChars")));
  Ptr lower_lhs=app(sym("lowerName"),apps(sym("consChar"),{pvar(0),pvar(1)}));
  Ptr lower_rhs=apps(sym("consChar"),{app(sym("lowerChar"),pvar(0)),app(sym("lowerName"),pvar(1))});
  rules.push_back(rule(lower_lhs,lower_rhs));
  rules.push_back(rule(apps(sym("scanAtEnd"),{sym("true"),pvar(12),pvar(13)}),sym("nameScanEndFailure")));
  rules.push_back(rule(apps(sym("scanAtEnd"),{sym("false"),pvar(14),pvar(15)}),app(sym("nameMappedByFullScan"),app(sym("lowerName"),pvar(15)))));
  rules.push_back(rule(apps(sym("scanNameFrontier"),{pvar(2),sym("atEnd"),pvar(3)}),apps(sym("scanAtEnd"),{app(sym("allFast"),pvar(2)),pvar(2),pvar(3)})));
  rules.push_back(rule(apps(sym("scanNameFrontier"),{pvar(4),sym("atDelimiter"),pvar(5)}),app(sym("nameStopsAtDelimiter"),pvar(4))));
  rules.push_back(rule(apps(sym("scanNameFrontier"),{pvar(6),sym("atInvalidChar"),pvar(7)}),app(sym("nameStopsBeforeInvalid"),pvar(6))));
  rules.push_back(rule(apps(sym("scanNameFrontier"),{pvar(8),sym("atValidSlowPathChar"),pvar(9)}),app(sym("nameMappedByFullScan"),app(sym("lowerName"),pvar(9)))));
  RuleIndex index(rules);auto sample=query("id",Next::Delimiter,"id");auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,R,index));
  struct Case{std::string prefix;Next next;std::string full;};
  const std::vector<Case> cases={{"",Next::End,""},{"id",Next::Delimiter,"id"},{"",Next::Delimiter,""},{"data-x",Next::Delimiter,"data-x"},{"on",Next::End,"on"},{"ID",Next::End,"ID"},{"data1",Next::End,"data1"},{"na",Next::UpperOrDigit,"naMe"},{"id",Next::UpperOrDigit,"id2"},{"",Next::UpperOrDigit,"A1"},{"id",Next::Invalid,"id_"},{"",Next::Invalid,"_id"},{"cl",Next::UpperOrDigit,"clAss"},{"-d",Next::UpperOrDigit,"-d1"}};
  unsigned states=0;
  for(const auto& c:cases){if(!conv(nf(query(c.prefix,c.next,c.full),index),expected(cpp_scan(c.prefix,c.next,c.full)),index)){std::cerr<<"ScanAttrName frontier mismatch\n";return 2;}++states;}
  std::cout<<"HTML fast-path attribute scanner frontier covers "<<states<<" finite prefix/next-character states.\n";
  std::cout<<"Scope: end-of-input failure, delimiter stop, invalid-character stop without consuming the character, and full lowercase mapping path are modeled; ParseAttributes decides the subsequent outcome.\n";
}
