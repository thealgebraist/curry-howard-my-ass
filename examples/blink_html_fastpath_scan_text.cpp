#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

enum class Ch { Plain8, Plain16, LessThan, Ampersand, CarriageReturn, Null };
struct ScanResult { enum Kind { Plain, EscapedDelegation, NullFailure } kind; std::vector<Ch> text, rest; bool is_8bit=true; };
static ScanResult cpp_scan(const std::vector<Ch>& input){
  std::vector<Ch> text; bool is_8bit=true;
  for(size_t i=0;i<input.size();++i){
    switch(input[i]){
      case Ch::LessThan:return {ScanResult::Plain,text,std::vector<Ch>(input.begin()+i,input.end()),is_8bit};
      case Ch::Ampersand:case Ch::CarriageReturn:return {ScanResult::EscapedDelegation,input,{},true};
      case Ch::Null:return {ScanResult::NullFailure,{},std::vector<Ch>(input.begin()+i,input.end()),is_8bit};
      case Ch::Plain8:text.push_back(Ch::Plain8);break;
      case Ch::Plain16:text.push_back(Ch::Plain16);is_8bit=false;break;
    }
  }
  return {ScanResult::Plain,text,{},is_8bit};
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bt(bool b){return sym(b?"true":"false");}
static const char* c_name(Ch c){switch(c){case Ch::Plain8:return "char8";case Ch::Plain16:return "char16";case Ch::LessThan:return "lessThan";case Ch::Ampersand:return "ampersand";case Ch::CarriageReturn:return "carriageReturn";case Ch::Null:return "nullChar";}return "char8";}
static Ptr chars(const std::vector<Ch>& xs){Ptr t=sym("nilText");for(auto i=xs.rbegin();i!=xs.rend();++i)t=apps(sym("consText"),{sym(c_name(*i)),t});return t;}
static Ptr result(const ScanResult& r){switch(r.kind){case ScanResult::Plain:return apps(sym("plainText"),{chars(r.text),chars(r.rest),bt(r.is_8bit)});case ScanResult::EscapedDelegation:return app(sym("delegateEscapedScanFromStart"),chars(r.text));case ScanResult::NullFailure:return app(sym("failAtNull"),chars(r.rest));}return sym("failAtNull");}
static Ptr scan_term(const std::vector<Ch>& input){return app(sym("scanText"),chars(input));}

int main(){
  const Ptr B=sym("Bool"),C=sym("TextCodeUnit"),L=sym("TextCodeUnits"),R=sym("TextScanResult");Signature sig;std::vector<Rule> rules;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["TextCodeUnit"]=type_sort();for(const char* c:{"char8","char16","lessThan","ampersand","carriageReturn","nullChar"})sig[c]=C;
  sig["TextCodeUnits"]=type_sort();sig["nilText"]=L;sig["consText"]=fn(C,fn(L,L));
  sig["TextScanResult"]=type_sort();sig["plainText"]=fn(L,fn(L,fn(B,R)));sig["delegateEscapedScanFromStart"]=fn(L,R);sig["failAtNull"]=fn(L,R);
  sig["scanTextLoop"]=fn(L,fn(L,fn(L,fn(B,R))));sig["scanText"]=fn(L,R);sig["snocText"]=fn(L,fn(C,L));
  rules.push_back(rule(apps(sym("snocText"),{sym("nilText"),pvar(0)}),apps(sym("consText"),{pvar(0),sym("nilText")})));
  rules.push_back(rule(apps(sym("snocText"),{apps(sym("consText"),{pvar(1),pvar(2)}),pvar(3)}),apps(sym("consText"),{pvar(1),apps(sym("snocText"),{pvar(2),pvar(3)})})));
  rules.push_back(rule(app(sym("scanText"),pvar(4)),apps(sym("scanTextLoop"),{pvar(4),sym("nilText"),pvar(4),sym("true")})));
  rules.push_back(rule(apps(sym("scanTextLoop"),{pvar(5),pvar(6),sym("nilText"),pvar(7)}),apps(sym("plainText"),{pvar(6),sym("nilText"),pvar(7)})));
  rules.push_back(rule(apps(sym("scanTextLoop"),{pvar(8),pvar(9),apps(sym("consText"),{sym("lessThan"),pvar(10)}),pvar(11)}),apps(sym("plainText"),{pvar(9),apps(sym("consText"),{sym("lessThan"),pvar(10)}),pvar(11)})));
  for(const char* special:{"ampersand","carriageReturn"})rules.push_back(rule(apps(sym("scanTextLoop"),{pvar(12),pvar(13),apps(sym("consText"),{sym(special),pvar(14)}),pvar(15)}),app(sym("delegateEscapedScanFromStart"),pvar(12))));
  rules.push_back(rule(apps(sym("scanTextLoop"),{pvar(16),pvar(17),apps(sym("consText"),{sym("nullChar"),pvar(18)}),pvar(19)}),app(sym("failAtNull"),apps(sym("consText"),{sym("nullChar"),pvar(18)}))));
  rules.push_back(rule(apps(sym("scanTextLoop"),{pvar(20),pvar(21),apps(sym("consText"),{sym("char8"),pvar(22)}),pvar(23)}),apps(sym("scanTextLoop"),{pvar(20),apps(sym("snocText"),{pvar(21),sym("char8")}),pvar(22),pvar(23)})));
  rules.push_back(rule(apps(sym("scanTextLoop"),{pvar(24),pvar(25),apps(sym("consText"),{sym("char16"),pvar(26)}),pvar(27)}),apps(sym("scanTextLoop"),{pvar(24),apps(sym("snocText"),{pvar(25),sym("char16")}),pvar(26),sym("false")})));
  RuleIndex index(rules);
  const std::vector<std::vector<Ch>> cases={{},{Ch::Plain8},{Ch::Plain8,Ch::Plain8,Ch::LessThan},{Ch::Plain8,Ch::Plain16,Ch::LessThan},{Ch::Ampersand},{Ch::Plain8,Ch::Ampersand,Ch::Plain8},{Ch::CarriageReturn,Ch::Plain8},{Ch::Plain8,Ch::Null,Ch::LessThan},{Ch::Null},{Ch::Plain16,Ch::Plain8},{Ch::Plain8,Ch::CarriageReturn}};
  auto sample=scan_term(cases[2]);auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,R,index));
  unsigned states=0;
  for(const auto& input:cases){if(!conv(nf(scan_term(input),index),result(cpp_scan(input)),index)){std::cerr<<"ScanText branch mismatch\n";return 2;}++states;}
  std::cout<<"HTML fast-path ScanText covers "<<states<<" finite code-unit sequences.\n";
  std::cout<<"Scope: plain-prefix accumulation, cursor at '<', 8-bit classification, reset/delegation from the original start at '&'/CR, and null failure are modeled. Escaped scanning, newline-whitespace canonicalization, SIMD, and source string identity are abstracted.\n";
}
