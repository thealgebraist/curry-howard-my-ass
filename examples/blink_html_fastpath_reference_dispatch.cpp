#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

enum class Scan { Terminated, TerminatedAtLimit, EndBeforeSemicolon, NullBeforeSemicolon, TooLong };
enum class Ref { TooShort, Numeric, Amp, Lt, Gt, Nbsp, Uncommon };
enum class Parse { Invalid, ValidA, ValidB };
enum class Fallback { Failed, SuccessNoRemainder, SuccessWithRemainder };
enum class Result { Failed, DecodedA, DecodedB, DecodedAmp, DecodedLt,
                    DecodedGt, DecodedNbsp, FallbackNoRemainder,
                    FallbackAdjustedCursor };
struct Input { Scan scan; Ref ref; Parse numeric; Fallback fallback; };
static Result cpp_dispatch(const Input& x){
  if(x.scan!=Scan::Terminated&&x.scan!=Scan::TerminatedAtLimit)return Result::Failed;
  if(x.ref==Ref::TooShort)return Result::Failed;
  if(x.ref==Ref::Numeric)return x.numeric==Parse::Invalid?Result::Failed:x.numeric==Parse::ValidA?Result::DecodedA:Result::DecodedB;
  switch(x.ref){case Ref::Amp:return Result::DecodedAmp;case Ref::Lt:return Result::DecodedLt;case Ref::Gt:return Result::DecodedGt;case Ref::Nbsp:return Result::DecodedNbsp;case Ref::Uncommon:if(x.fallback==Fallback::Failed)return Result::Failed;return x.fallback==Fallback::SuccessNoRemainder?Result::FallbackNoRemainder:Result::FallbackAdjustedCursor;default:return Result::Failed;}
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr scan_term(Scan s){switch(s){case Scan::Terminated:return sym("semicolonFound");case Scan::TerminatedAtLimit:return sym("semicolonFoundAtLimit");case Scan::EndBeforeSemicolon:return sym("referenceEndedEarly");case Scan::NullBeforeSemicolon:return sym("referenceHitNull");case Scan::TooLong:return sym("referenceExceededLookahead");}return sym("referenceEndedEarly");}
static Ptr ref_term(Ref r){switch(r){case Ref::TooShort:return sym("referenceTooShort");case Ref::Numeric:return sym("numericReference");case Ref::Amp:return sym("nameAmp");case Ref::Lt:return sym("nameLt");case Ref::Gt:return sym("nameGt");case Ref::Nbsp:return sym("nameNbsp");case Ref::Uncommon:return sym("uncommonName");}return sym("referenceTooShort");}
static Ptr parse_term(Parse p){return sym(p==Parse::Invalid?"numericInvalid":p==Parse::ValidA?"numericOutputA":"numericOutputB");}
static Ptr fallback_term(Fallback f){switch(f){case Fallback::Failed:return sym("fallbackFailed");case Fallback::SuccessNoRemainder:return sym("fallbackConsumedAll");case Fallback::SuccessWithRemainder:return sym("fallbackHasRemainder");}return sym("fallbackFailed");}
static Ptr result_term(Result r){switch(r){case Result::Failed:return sym("referenceFailure");case Result::DecodedA:return sym("decodedA");case Result::DecodedB:return sym("decodedB");case Result::DecodedAmp:return sym("decodedAmp");case Result::DecodedLt:return sym("decodedLt");case Result::DecodedGt:return sym("decodedGt");case Result::DecodedNbsp:return sym("decodedNbsp");case Result::FallbackNoRemainder:return sym("decodedFallback");case Result::FallbackAdjustedCursor:return sym("decodedFallbackAndRewindRemainder");}return sym("referenceFailure");}
static Ptr query(const Input& x){return apps(sym("scanCharacterReference"),{scan_term(x.scan),ref_term(x.ref),parse_term(x.numeric),fallback_term(x.fallback)});}

int main(){
  const Ptr S=sym("ReferenceScan"),N=sym("ReferenceKind"),P=sym("NumericObservation"),F=sym("FallbackObservation"),R=sym("ReferenceDispatchResult");Signature sig;std::vector<Rule> rules;
  sig["ReferenceScan"]=type_sort();for(const char* x:{"semicolonFound","semicolonFoundAtLimit","referenceEndedEarly","referenceHitNull","referenceExceededLookahead"})sig[x]=S;
  sig["ReferenceKind"]=type_sort();for(const char* x:{"referenceTooShort","numericReference","nameAmp","nameLt","nameGt","nameNbsp","uncommonName"})sig[x]=N;
  sig["NumericObservation"]=type_sort();for(const char* x:{"numericInvalid","numericOutputA","numericOutputB"})sig[x]=P;
  sig["FallbackObservation"]=type_sort();for(const char* x:{"fallbackFailed","fallbackConsumedAll","fallbackHasRemainder"})sig[x]=F;
  sig["ReferenceDispatchResult"]=type_sort();for(const char* x:{"referenceFailure","decodedA","decodedB","decodedAmp","decodedLt","decodedGt","decodedNbsp","decodedFallback","decodedFallbackAndRewindRemainder"})sig[x]=R;
  sig["scanCharacterReference"]=fn(S,fn(N,fn(P,fn(F,R))));
  for(const char* scan:{"referenceEndedEarly","referenceHitNull","referenceExceededLookahead"})rules.push_back(rule(apps(sym("scanCharacterReference"),{sym(scan),pvar(0),pvar(1),pvar(2)}),sym("referenceFailure")));
  for(const char* terminator:{"semicolonFound","semicolonFoundAtLimit"}){
    rules.push_back(rule(apps(sym("scanCharacterReference"),{sym(terminator),sym("referenceTooShort"),pvar(3),pvar(4)}),sym("referenceFailure")));
    rules.push_back(rule(apps(sym("scanCharacterReference"),{sym(terminator),sym("numericReference"),sym("numericInvalid"),pvar(5)}),sym("referenceFailure")));
    rules.push_back(rule(apps(sym("scanCharacterReference"),{sym(terminator),sym("numericReference"),sym("numericOutputA"),pvar(6)}),sym("decodedA")));
    rules.push_back(rule(apps(sym("scanCharacterReference"),{sym(terminator),sym("numericReference"),sym("numericOutputB"),pvar(7)}),sym("decodedB")));
    for(const auto& pair:std::vector<std::pair<const char*,const char*>>{{"nameAmp","decodedAmp"},{"nameLt","decodedLt"},{"nameGt","decodedGt"},{"nameNbsp","decodedNbsp"}})
      rules.push_back(rule(apps(sym("scanCharacterReference"),{sym(terminator),sym(pair.first),pvar(8),pvar(9)}),sym(pair.second)));
    rules.push_back(rule(apps(sym("scanCharacterReference"),{sym(terminator),sym("uncommonName"),pvar(10),sym("fallbackFailed")}),sym("referenceFailure")));
    rules.push_back(rule(apps(sym("scanCharacterReference"),{sym(terminator),sym("uncommonName"),pvar(11),sym("fallbackConsumedAll")}),sym("decodedFallback")));
    rules.push_back(rule(apps(sym("scanCharacterReference"),{sym(terminator),sym("uncommonName"),pvar(12),sym("fallbackHasRemainder")}),sym("decodedFallbackAndRewindRemainder")));
  }
  RuleIndex index(rules);Input sample{Scan::Terminated,Ref::Amp,Parse::Invalid,Fallback::Failed};auto inferred=infer({},query(sample),sig,rules);assert(inferred&&conv(*inferred,R,index));
  const std::vector<Input> cases={
    {Scan::EndBeforeSemicolon,Ref::Numeric,Parse::ValidA,Fallback::SuccessNoRemainder},
    {Scan::NullBeforeSemicolon,Ref::Amp,Parse::ValidA,Fallback::SuccessNoRemainder},
    {Scan::TooLong,Ref::Uncommon,Parse::ValidA,Fallback::SuccessWithRemainder},
    {Scan::TerminatedAtLimit,Ref::Numeric,Parse::ValidA,Fallback::Failed},
    {Scan::Terminated,Ref::TooShort,Parse::Invalid,Fallback::Failed},
    {Scan::Terminated,Ref::Numeric,Parse::Invalid,Fallback::Failed},
    {Scan::Terminated,Ref::Numeric,Parse::ValidA,Fallback::Failed},
    {Scan::Terminated,Ref::Numeric,Parse::ValidB,Fallback::Failed},
    {Scan::Terminated,Ref::Amp,Parse::Invalid,Fallback::Failed},
    {Scan::Terminated,Ref::Lt,Parse::Invalid,Fallback::Failed},
    {Scan::Terminated,Ref::Gt,Parse::Invalid,Fallback::Failed},
    {Scan::Terminated,Ref::Nbsp,Parse::Invalid,Fallback::Failed},
    {Scan::Terminated,Ref::Uncommon,Parse::Invalid,Fallback::Failed},
    {Scan::Terminated,Ref::Uncommon,Parse::Invalid,Fallback::SuccessNoRemainder},
    {Scan::Terminated,Ref::Uncommon,Parse::Invalid,Fallback::SuccessWithRemainder}
  };
  unsigned states=0;for(const auto& x:cases){if(!conv(nf(query(x),index),result_term(cpp_dispatch(x)),index)){std::cerr<<"ScanHTMLCharacterReference dispatch mismatch\n";return 2;}++states;}
  std::cout<<"HTML fast-path character-reference gate/dispatch covers "<<states<<" finite observations.\n";
  std::cout<<"Scope: pre-semicolon failure classes, minimum reference length, numeric/common-name/fallback branch routing, decoder success/failure, and fallback remainder rewind are modeled. Character scanning, numeric conversion, entity tables, and actual cursor offsets are abstracted.\n";
}
