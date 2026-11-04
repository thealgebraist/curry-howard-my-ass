#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace lp;

enum class End { Greater, Slash, SpaceGreater, SpaceSlash, SpaceEnd, SpaceOther, Invalid, Eof };
enum class Kind { FastName, MappedName, Failed };
struct Result { Kind kind; std::string name; End end; };
static bool lower_ascii(char c){return c>='a'&&c<='z';}
static bool ascii_letter(char c){return lower_ascii(c)||(c>='A'&&c<='Z');}
static std::string lowercase(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c=static_cast<char>(c+('a'-'A'));return s;}
static bool accepted(End e){return e==End::Greater||e==End::Slash||e==End::SpaceGreater||e==End::SpaceSlash||e==End::SpaceEnd||e==End::SpaceOther;}
static Result cpp_scan(const std::string& raw,End e){
  if(!accepted(e)||!std::all_of(raw.begin(),raw.end(),ascii_letter))return {Kind::Failed,{},e};
  const bool fast=std::all_of(raw.begin(),raw.end(),lower_ascii);
  return {fast?Kind::FastName:Kind::MappedName,fast?raw:lowercase(raw),e};
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bt(bool b){return sym(b?"true":"false");}
static Ptr char_term(char c){return sym(std::string("tag_")+c);}
static Ptr chars(const std::string& s){Ptr out=sym("nilTagChars");for(auto i=s.rbegin();i!=s.rend();++i)out=apps(sym("consTagChar"),{char_term(*i),out});return out;}
static Ptr end_term(End e){switch(e){case End::Greater:return sym("endGreater");case End::Slash:return sym("endSlash");case End::SpaceGreater:return sym("endSpaceThenGreater");case End::SpaceSlash:return sym("endSpaceThenSlash");case End::SpaceEnd:return sym("endSpaceThenEof");case End::SpaceOther:return sym("endSpaceThenOther");case End::Invalid:return sym("endInvalid");case End::Eof:return sym("endEof");}return sym("endEof");}
static Ptr result_term(const Result& r){if(r.kind==Kind::Failed)return sym("tagNameFailure");return apps(sym(r.kind==Kind::FastName?"fastPathTagName":"mappedTagName"),{chars(r.name),end_term(r.end)});}
static Ptr query(const std::string& raw,End e){return apps(sym("scanTagname"),{chars(raw),end_term(e)});}

int main(){
  const Ptr B=sym("Bool"),C=sym("AsciiTagChar"),L=sym("AsciiTagName"),E=sym("TagNameEnding"),R=sym("TagNameScanResult");Signature sig;std::vector<Rule> rules;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["AsciiTagChar"]=type_sort();for(char c='a';c<='z';++c)sig[std::string("tag_")+c]=C;for(char c='A';c<='Z';++c)sig[std::string("tag_")+c]=C;
  sig["AsciiTagName"]=type_sort();sig["nilTagChars"]=L;sig["consTagChar"]=fn(C,fn(L,L));
  sig["TagNameEnding"]=type_sort();
  const std::vector<std::pair<End,const char*>> ends={{End::Greater,"endGreater"},{End::Slash,"endSlash"},{End::SpaceGreater,"endSpaceThenGreater"},{End::SpaceSlash,"endSpaceThenSlash"},{End::SpaceEnd,"endSpaceThenEof"},{End::SpaceOther,"endSpaceThenOther"},{End::Invalid,"endInvalid"},{End::Eof,"endEof"}};
  for(const auto& [e,n]:ends)sig[n]=E;
  sig["TagNameScanResult"]=type_sort();sig["tagNameFailure"]=R;sig["fastPathTagName"]=fn(L,fn(E,R));sig["mappedTagName"]=fn(L,fn(E,R));
  sig["isLowerTagChar"]=fn(C,B);sig["boolAnd"]=fn(B,fn(B,B));sig["allLowerTagName"]=fn(L,B);sig["lowerTagChar"]=fn(C,C);sig["lowerTagName"]=fn(L,L);sig["chooseTagNamePath"]=fn(B,fn(L,fn(E,R)));sig["scanTagname"]=fn(L,fn(E,R));
  for(char c='a';c<='z';++c){rules.push_back(rule(app(sym("isLowerTagChar"),char_term(c)),sym("true")));rules.push_back(rule(app(sym("lowerTagChar"),char_term(c)),char_term(c)));}
  for(char c='A';c<='Z';++c){rules.push_back(rule(app(sym("isLowerTagChar"),char_term(c)),sym("false")));rules.push_back(rule(app(sym("lowerTagChar"),char_term(c)),char_term(static_cast<char>(c+('a'-'A')))));}
  for(bool a:{false,true})for(bool b:{false,true})rules.push_back(rule(apps(sym("boolAnd"),{bt(a),bt(b)}),bt(a&&b)));
  rules.push_back(rule(app(sym("allLowerTagName"),sym("nilTagChars")),sym("true")));
  rules.push_back(rule(app(sym("allLowerTagName"),apps(sym("consTagChar"),{pvar(0),pvar(1)})),apps(sym("boolAnd"),{app(sym("isLowerTagChar"),pvar(0)),app(sym("allLowerTagName"),pvar(1))})));
  rules.push_back(rule(app(sym("lowerTagName"),sym("nilTagChars")),sym("nilTagChars")));
  rules.push_back(rule(app(sym("lowerTagName"),apps(sym("consTagChar"),{pvar(2),pvar(3)})),apps(sym("consTagChar"),{app(sym("lowerTagChar"),pvar(2)),app(sym("lowerTagName"),pvar(3))})));
  rules.push_back(rule(apps(sym("chooseTagNamePath"),{sym("true"),pvar(4),pvar(5)}),apps(sym("fastPathTagName"),{pvar(4),pvar(5)})));
  rules.push_back(rule(apps(sym("chooseTagNamePath"),{sym("false"),pvar(6),pvar(7)}),apps(sym("mappedTagName"),{app(sym("lowerTagName"),pvar(6)),pvar(7)})));
  for(const auto& [e,n]:ends){
    if(accepted(e))rules.push_back(rule(apps(sym("scanTagname"),{pvar(8),sym(n)}),apps(sym("chooseTagNamePath"),{app(sym("allLowerTagName"),pvar(8)),pvar(8),sym(n)})));
    else rules.push_back(rule(apps(sym("scanTagname"),{pvar(9),sym(n)}),sym("tagNameFailure")));
  }
  RuleIndex index(rules);const std::vector<std::pair<std::string,End>> cases={{"div",End::Greater},{"DIV",End::Greater},{"DiV",End::SpaceSlash},{"a",End::Slash},{"",End::Greater},{"p",End::SpaceEnd},{"ul",End::SpaceOther},{"button",End::SpaceGreater},{"span",End::Invalid},{"div",End::Eof},{"p",End::Invalid},{"A",End::Invalid}};
  auto sample=query("div",End::Greater);auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,R,index));
  unsigned states=0;for(const auto& [raw,e]:cases){if(!conv(nf(query(raw,e),index),result_term(cpp_scan(raw,e)),index)){std::cerr<<"ScanTagname mismatch for "<<raw<<"\n";return 2;}++states;}
  std::cout<<"HTML fast-path tag-name scanning covers "<<states<<" finite name/ending observations.\n";
  std::cout<<"Scope: lowercase fast path, ASCII-uppercase mapping, delimiter validation, whitespace skip result, and non-letter rejection are modeled; source cursor offsets and subsequent tag dispatch are abstracted.\n";
}
