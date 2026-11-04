#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace lp;

enum class ScanKind { Scanned, ScannedAtEnd, FailedAtEnd };
struct ScanResult { ScanKind kind; std::string canonical; };
static bool fast_char(char c){return (c>='a'&&c<='z')||c=='-';}
static bool all_fast(const std::string& s){for(char c:s)if(!fast_char(c))return false;return true;}
static std::string lower_ascii(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c=static_cast<char>(c+('a'-'A'));return s;}
static ScanResult cpp_scan_name(const std::string& raw,bool ended_at_eof){
  const bool fast=all_fast(raw);
  if(fast&&ended_at_eof)return {ScanKind::FailedAtEnd,{}};
  if(!fast&&ended_at_eof)return {ScanKind::ScannedAtEnd,lower_ascii(raw)};
  return {ScanKind::Scanned,lower_ascii(raw)};
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bool_term(bool b){return sym(b?"true":"false");}
static const std::string alphabet="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-";
static Ptr char_term(char c){return sym(std::string("ch_")+static_cast<char>(c));}
static Ptr chars_term(const std::string& s){Ptr out=sym("nilChars");for(auto i=s.rbegin();i!=s.rend();++i)out=apps(sym("consChar"),{char_term(*i),out});return out;}
static Ptr scan_term(const std::string& s,bool eof){return apps(sym("scanAttrName"),{chars_term(s),sym(eof?"atEndOfInput":"atBoundary")});}
static Ptr expected(const ScanResult& r){if(r.kind==ScanKind::FailedAtEnd)return sym("failedAtEnd");return app(sym(r.kind==ScanKind::Scanned?"scannedName":"scannedNameAtEnd"),chars_term(r.canonical));}

int main(){
  const Ptr B=sym("Bool"),C=sym("NameChar"),CL=sym("NameChars"),E=sym("NameEnding"),R=sym("NameScanResult");
  Signature sig;std::vector<Rule> rules;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["NameChar"]=type_sort();for(char c:alphabet)sig[std::string("ch_")+c]=C;
  sig["NameChars"]=type_sort();sig["nilChars"]=CL;sig["consChar"]=fn(C,fn(CL,CL));
  sig["NameEnding"]=type_sort();sig["atEndOfInput"]=E;sig["atBoundary"]=E;
  sig["NameScanResult"]=type_sort();sig["failedAtEnd"]=R;sig["scannedName"]=fn(CL,R);sig["scannedNameAtEnd"]=fn(CL,R);
  sig["isFastNameChar"]=fn(C,B);sig["asciiLower"]=fn(C,C);sig["andBool"]=fn(B,fn(B,B));
  sig["allFastNameChars"]=fn(CL,B);sig["lowerName"]=fn(CL,CL);sig["scanAttrName"]=fn(CL,fn(E,R));
  for(char c:alphabet){
    bool fast=fast_char(c);rules.push_back(rule(app(sym("isFastNameChar"),char_term(c)),bool_term(fast)));
    char lowered=(c>='A'&&c<='Z')?static_cast<char>(c+('a'-'A')):c;
    rules.push_back(rule(app(sym("asciiLower"),char_term(c)),char_term(lowered)));
  }
  for(bool a:{false,true})for(bool b:{false,true})rules.push_back(rule(apps(sym("andBool"),{bool_term(a),bool_term(b)}),bool_term(a&&b)));
  rules.push_back(rule(app(sym("allFastNameChars"),sym("nilChars")),sym("true")));
  rules.push_back(rule(app(sym("allFastNameChars"),apps(sym("consChar"),{pvar(0),pvar(1)})),apps(sym("andBool"),{app(sym("isFastNameChar"),pvar(0)),app(sym("allFastNameChars"),pvar(1))})));
  rules.push_back(rule(app(sym("lowerName"),sym("nilChars")),sym("nilChars")));
  rules.push_back(rule(app(sym("lowerName"),apps(sym("consChar"),{pvar(2),pvar(3)})),apps(sym("consChar"),{app(sym("asciiLower"),pvar(2)),app(sym("lowerName"),pvar(3))})));
  rules.push_back(rule(apps(sym("scanAttrName"),{pvar(4),sym("atEndOfInput")}),apps(sym("scanNameAtEof"),{app(sym("allFastNameChars"),pvar(4)),pvar(4)})));
  rules.push_back(rule(apps(sym("scanAttrName"),{pvar(5),sym("atBoundary")}),app(sym("scannedName"),app(sym("lowerName"),pvar(5)))));
  rules.push_back(rule(apps(sym("scanNameAtEof"),{sym("true"),pvar(6)}),sym("failedAtEnd")));
  rules.push_back(rule(apps(sym("scanNameAtEof"),{sym("false"),pvar(7)}),app(sym("scannedNameAtEnd"),app(sym("lowerName"),pvar(7)))));
  sig["scanNameAtEof"]=fn(B,fn(CL,R));
  RuleIndex index(rules);
  auto sample=scan_term("ID",false);auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,R,index));
  const std::vector<std::string> inputs={"a","id","class","data-x","onload","is","ID","Id","DATA-X","onLoad","iS","a1","A1-","-x",""};
  unsigned states=0;
  for(const auto& raw:inputs)for(bool eof:{false,true}){
    if(!conv(nf(scan_term(raw,eof),index),expected(cpp_scan_name(raw,eof)),index)){
      std::cerr<<"ScanAttrName canonicalization mismatch for ["<<raw<<"]\n";return 2;
    }
    ++states;
  }
  std::cout<<"HTML fast-path attribute-name scan covers "<<states<<" finite name/boundary states.\n";
  std::cout<<"Scope: recursive lowercase-fast-path detection and ASCII uppercase normalization over valid name characters are modeled; invalid-character and subsequent ParseAttributes handling are abstracted.\n";
}
