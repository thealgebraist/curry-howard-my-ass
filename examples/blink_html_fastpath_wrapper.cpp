#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

enum class Gate { Allowed, ShadowRoots, Tracing, ContentPolicy, FormContext };
enum class Width { EightBit, SixteenBit };
enum class Parse { Success, UnsupportedTag, UnsupportedContext, OtherFailure };
enum class OutFlag { NoPointer, PointerSetFalse, PointerSetTrue, PointerUnchanged };
struct Input { Gate gate; Width width; Parse parse; bool out_pointer; bool log_context; bool context_supported; };
struct Route { enum Kind { EarlyReject, ParsedSuccess, ParsedFailure } kind; Gate gate; Width width; Parse parse; OutFlag flag; bool context_log; };
static Route cpp_route(const Input& x){
  if(x.gate!=Gate::Allowed)return {Route::EarlyReject,x.gate,x.width,x.parse,OutFlag::PointerUnchanged,false};
  OutFlag flag=OutFlag::NoPointer;
  if(x.out_pointer)flag=x.parse==Parse::UnsupportedTag?OutFlag::PointerSetTrue:OutFlag::PointerSetFalse;
  const bool context_log=x.parse==Parse::UnsupportedContext&&x.log_context&&!x.context_supported;
  return {x.parse==Parse::Success?Route::ParsedSuccess:Route::ParsedFailure,x.gate,x.width,x.parse,flag,context_log};
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr gate_term(Gate g){switch(g){case Gate::Allowed:return sym("gateAllowed");case Gate::ShadowRoots:return sym("gateShadowRoots");case Gate::Tracing:return sym("gateTracing");case Gate::ContentPolicy:return sym("gateContentPolicy");case Gate::FormContext:return sym("gateFormContext");}return sym("gateAllowed");}
static Ptr width_term(Width w){return sym(w==Width::EightBit?"source8Bit":"source16Bit");}
static Ptr parse_term(Parse p){switch(p){case Parse::Success:return sym("parseSucceeded");case Parse::UnsupportedTag:return sym("parseUnsupportedTag");case Parse::UnsupportedContext:return sym("parseUnsupportedContext");case Parse::OtherFailure:return sym("parseOtherFailure");}return sym("parseOtherFailure");}
static Ptr out_term(OutFlag f){switch(f){case OutFlag::NoPointer:return sym("noOutputPointer");case OutFlag::PointerSetFalse:return sym("outputSetFalse");case OutFlag::PointerSetTrue:return sym("outputSetTrue");case OutFlag::PointerUnchanged:return sym("outputUnchanged");}return sym("outputUnchanged");}
static Ptr route_term(const Route& r){switch(r.kind){case Route::EarlyReject:return app(sym("gateRejected"),gate_term(r.gate));case Route::ParsedSuccess:return apps(sym("fragmentParsedSuccessfully"),{width_term(r.width),out_term(r.flag)});case Route::ParsedFailure:return apps(sym("fragmentParseAborted"),{width_term(r.width),parse_term(r.parse),out_term(r.flag),sym(r.context_log?"contextFailureLogged":"contextFailureNotLogged")});}return sym("outputUnchanged");}
static Ptr bt(bool b){return sym(b?"true":"false");}
static Ptr query(const Input& x){return apps(sym("tryParseFragment"),{gate_term(x.gate),width_term(x.width),parse_term(x.parse),bt(x.out_pointer),bt(x.log_context),bt(x.context_supported)});}

int main(){
  const Ptr G=sym("FastPathGate"),W=sym("SourceWidth"),P=sym("ParserResult"),B=sym("Bool"),O=sym("OutputPointerState"),F=sym("OutputFlagEffect"),L=sym("ContextLogDecision"),R=sym("TryFragmentResult");Signature sig;std::vector<Rule> rules;
  sig["FastPathGate"]=type_sort();for(const char* x:{"gateAllowed","gateShadowRoots","gateTracing","gateContentPolicy","gateFormContext"})sig[x]=G;
  sig["SourceWidth"]=type_sort();sig["source8Bit"]=W;sig["source16Bit"]=W;
  sig["ParserResult"]=type_sort();for(const char* x:{"parseSucceeded","parseUnsupportedTag","parseUnsupportedContext","parseOtherFailure"})sig[x]=P;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["OutputPointerState"]=type_sort();sig["hasOutputPointer"]=O;sig["noOutputPointerInput"]=O;
  sig["OutputFlagEffect"]=type_sort();for(const char* x:{"noOutputPointer","outputSetFalse","outputSetTrue","outputUnchanged"})sig[x]=F;
  sig["ContextLogDecision"]=type_sort();sig["contextFailureLogged"]=L;sig["contextFailureNotLogged"]=L;
  sig["TryFragmentResult"]=type_sort();sig["gateRejected"]=fn(G,R);sig["fragmentParsedSuccessfully"]=fn(W,fn(F,R));sig["fragmentParseAborted"]=fn(W,fn(P,fn(F,fn(L,R))));
  sig["outputEffect"]=fn(O,fn(P,F));sig["pointerState"]=fn(B,O);sig["contextLogDecision"]=fn(P,fn(B,fn(B,L)));sig["reportParserRun"]=fn(W,fn(P,fn(O,fn(B,fn(B,R)))));sig["tryParseFragment"]=fn(G,fn(W,fn(P,fn(B,fn(B,fn(B,R))))));
  rules.push_back(rule(app(sym("pointerState"),sym("true")),sym("hasOutputPointer")));
  rules.push_back(rule(app(sym("pointerState"),sym("false")),sym("noOutputPointerInput")));
  rules.push_back(rule(apps(sym("outputEffect"),{sym("noOutputPointerInput"),pvar(0)}),sym("noOutputPointer")));
  rules.push_back(rule(apps(sym("outputEffect"),{sym("hasOutputPointer"),sym("parseUnsupportedTag")}),sym("outputSetTrue")));
  for(const char* p:{"parseSucceeded","parseUnsupportedContext","parseOtherFailure"})rules.push_back(rule(apps(sym("outputEffect"),{sym("hasOutputPointer"),sym(p)}),sym("outputSetFalse")));
  rules.push_back(rule(apps(sym("contextLogDecision"),{sym("parseUnsupportedContext"),sym("true"),sym("false")}),sym("contextFailureLogged")));
  for(const char* p:{"parseSucceeded","parseUnsupportedTag","parseOtherFailure"})for(bool log:{false,true})for(bool supported:{false,true})rules.push_back(rule(apps(sym("contextLogDecision"),{sym(p),bt(log),bt(supported)}),sym("contextFailureNotLogged")));
  for(bool log:{false,true})for(bool supported:{false,true}){
    rules.push_back(rule(apps(sym("contextLogDecision"),{sym("parseUnsupportedContext"),bt(log),bt(supported)}),sym(log&&!supported?"contextFailureLogged":"contextFailureNotLogged")));
  }
  for(const char* width:{"source8Bit","source16Bit"}){
    rules.push_back(rule(apps(sym("reportParserRun"),{sym(width),sym("parseSucceeded"),pvar(1),pvar(2),pvar(3)}),apps(sym("fragmentParsedSuccessfully"),{sym(width),apps(sym("outputEffect"),{pvar(1),sym("parseSucceeded")})})));
    for(const char* parse:{"parseUnsupportedTag","parseUnsupportedContext","parseOtherFailure"})
      rules.push_back(rule(apps(sym("reportParserRun"),{sym(width),sym(parse),pvar(4),pvar(5),pvar(6)}),apps(sym("fragmentParseAborted"),{sym(width),sym(parse),apps(sym("outputEffect"),{pvar(4),sym(parse)}),apps(sym("contextLogDecision"),{sym(parse),pvar(5),pvar(6)})})));
  }
  for(const char* gate:{"gateShadowRoots","gateTracing","gateContentPolicy","gateFormContext"})rules.push_back(rule(apps(sym("tryParseFragment"),{sym(gate),pvar(7),pvar(8),pvar(9),pvar(10),pvar(11)}),app(sym("gateRejected"),sym(gate))));
  rules.push_back(rule(apps(sym("tryParseFragment"),{sym("gateAllowed"),pvar(12),pvar(13),pvar(14),pvar(15),pvar(16)}),apps(sym("reportParserRun"),{pvar(12),pvar(13),app(sym("pointerState"),pvar(14)),pvar(15),pvar(16)})));
  RuleIndex index(rules);
  Input sample{Gate::Allowed,Width::EightBit,Parse::Success,true,false,false};auto inferred=infer({},query(sample),sig,rules);assert(inferred&&conv(*inferred,R,index));
  unsigned states=0;
  for(Gate gate:{Gate::Allowed,Gate::ShadowRoots,Gate::Tracing,Gate::ContentPolicy,Gate::FormContext})
    for(Width width:{Width::EightBit,Width::SixteenBit})for(Parse parse:{Parse::Success,Parse::UnsupportedTag,Parse::UnsupportedContext,Parse::OtherFailure})
      for(bool out:{false,true})for(bool log:{false,true})for(bool supported:{false,true}){
        Input in{gate,width,parse,out,log,supported};if(!conv(nf(query(in),index),route_term(cpp_route(in)),index)){std::cerr<<"TryParsingHTMLFragment wrapper mismatch\n";return 2;}++states;
      }
  std::cout<<"HTML fragment fast-path wrapper covers "<<states<<" finite gate/parser/report states.\n";
  std::cout<<"Scope: early gate return, 8/16-bit parser dispatch, optional unsupported-tag output, success/abort report, and conditional unsupported-context logging are modeled. Run internals, metrics values, and parser side effects are abstracted.\n";
}
