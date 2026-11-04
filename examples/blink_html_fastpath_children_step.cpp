#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

enum class Scan { Failed, Empty, PlainText, EscapedText };
enum class Child { Failed, Parsed };
enum class TextAction { None, AppendPlain, AppendEscaped };
enum class Result { ScanFailed, EndAfterText, ClosingTagAfterText, MaxDepthAfterText,
                    ChildFailedAfterText, ChildAppendedAfterText };
struct Input { Scan scan; bool at_end; bool next_is_slash; bool depth_limit_hit; Child child; };
static TextAction text_action(Scan s){return s==Scan::PlainText?TextAction::AppendPlain:s==Scan::EscapedText?TextAction::AppendEscaped:TextAction::None;}
static Result cpp_step(const Input& in){
  if(in.scan==Scan::Failed)return Result::ScanFailed;
  if(in.at_end)return Result::EndAfterText;
  if(in.next_is_slash)return Result::ClosingTagAfterText;
  if(in.depth_limit_hit)return Result::MaxDepthAfterText;
  return in.child==Child::Failed?Result::ChildFailedAfterText:Result::ChildAppendedAfterText;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bt(bool b){return sym(b?"true":"false");}
static Ptr scan_term(Scan s){switch(s){case Scan::Failed:return sym("scanFailed");case Scan::Empty:return sym("scanEmpty");case Scan::PlainText:return sym("scanPlainText");case Scan::EscapedText:return sym("scanEscapedText");}return sym("scanFailed");}
static Ptr child_term(Child c){return sym(c==Child::Failed?"childFailed":"childParsed");}
static Ptr text_action_term(TextAction a){switch(a){case TextAction::None:return sym("noTextAppend");case TextAction::AppendPlain:return sym("appendPlainText");case TextAction::AppendEscaped:return sym("appendEscapedText");}return sym("noTextAppend");}
static Ptr result_term(Result r){switch(r){case Result::ScanFailed:return sym("preserveScanFailure");case Result::EndAfterText:return sym("stopAtInputEnd");case Result::ClosingTagAfterText:return sym("stopAtClosingTag");case Result::MaxDepthAfterText:return sym("failAtMaximumDepth");case Result::ChildFailedAfterText:return sym("returnAfterChildFailure");case Result::ChildAppendedAfterText:return sym("appendChildAndContinue");}return sym("preserveScanFailure");}
static Ptr query(const Input& x){return apps(sym("parseChildrenStep"),{scan_term(x.scan),bt(x.at_end),bt(x.next_is_slash),bt(x.depth_limit_hit),child_term(x.child)});}
static Ptr expected(const Input& x){if(x.scan==Scan::Failed)return sym("preserveScanFailure");return app(result_term(cpp_step(x)),text_action_term(text_action(x.scan)));}

int main(){
  const Ptr B=sym("Bool"),S=sym("TextScanResult"),C=sym("ChildParseResult"),T=sym("TextAppendAction"),R=sym("ChildrenStepResult");Signature sig;std::vector<Rule> rules;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["TextScanResult"]=type_sort();for(const char* x:{"scanFailed","scanEmpty","scanPlainText","scanEscapedText"})sig[x]=S;
  sig["ChildParseResult"]=type_sort();sig["childFailed"]=C;sig["childParsed"]=C;
  sig["TextAppendAction"]=type_sort();for(const char* x:{"noTextAppend","appendPlainText","appendEscapedText"})sig[x]=T;
  sig["ChildrenStepResult"]=type_sort();sig["preserveScanFailure"]=R;for(const char* x:{"stopAtInputEnd","stopAtClosingTag","failAtMaximumDepth","returnAfterChildFailure","appendChildAndContinue"})sig[x]=fn(T,R);
  sig["textAction"]=fn(S,T);sig["afterTextScan"]=fn(T,fn(B,fn(B,fn(B,fn(C,R)))));sig["parseChildrenStep"]=fn(S,fn(B,fn(B,fn(B,fn(C,R)))));
  rules.push_back(rule(app(sym("textAction"),sym("scanFailed")),sym("noTextAppend")));
  rules.push_back(rule(app(sym("textAction"),sym("scanEmpty")),sym("noTextAppend")));
  rules.push_back(rule(app(sym("textAction"),sym("scanPlainText")),sym("appendPlainText")));
  rules.push_back(rule(app(sym("textAction"),sym("scanEscapedText")),sym("appendEscapedText")));
  rules.push_back(rule(apps(sym("afterTextScan"),{pvar(0),sym("true"),pvar(1),pvar(2),pvar(3)}),app(sym("stopAtInputEnd"),pvar(0))));
  rules.push_back(rule(apps(sym("afterTextScan"),{pvar(4),sym("false"),sym("true"),pvar(5),pvar(6)}),app(sym("stopAtClosingTag"),pvar(4))));
  rules.push_back(rule(apps(sym("afterTextScan"),{pvar(7),sym("false"),sym("false"),sym("true"),pvar(8)}),app(sym("failAtMaximumDepth"),pvar(7))));
  rules.push_back(rule(apps(sym("afterTextScan"),{pvar(9),sym("false"),sym("false"),sym("false"),sym("childFailed")}),app(sym("returnAfterChildFailure"),pvar(9))));
  rules.push_back(rule(apps(sym("afterTextScan"),{pvar(10),sym("false"),sym("false"),sym("false"),sym("childParsed")}),app(sym("appendChildAndContinue"),pvar(10))));
  rules.push_back(rule(apps(sym("parseChildrenStep"),{sym("scanFailed"),pvar(11),pvar(12),pvar(13),pvar(14)}),sym("preserveScanFailure")));
  for(const char* scan:{"scanEmpty","scanPlainText","scanEscapedText"})
    rules.push_back(rule(apps(sym("parseChildrenStep"),{sym(scan),pvar(15),pvar(16),pvar(17),pvar(18)}),apps(sym("afterTextScan"),{app(sym("textAction"),sym(scan)),pvar(15),pvar(16),pvar(17),pvar(18)})));
  RuleIndex index(rules);Input sample{Scan::PlainText,true,false,false,Child::Parsed};auto inferred=infer({},query(sample),sig,rules);assert(inferred&&conv(*inferred,R,index));
  unsigned states=0;
  for(Scan s:{Scan::Failed,Scan::Empty,Scan::PlainText,Scan::EscapedText})for(bool end:{false,true})for(bool slash:{false,true})for(bool depth:{false,true})for(Child c:{Child::Failed,Child::Parsed}){
    Input in{s,end,slash,depth,c};if(!conv(nf(query(in),index),expected(in),index)){std::cerr<<"ParseChildren step mismatch\n";return 2;}++states;
  }
  std::cout<<"HTML fast-path ParseChildren iteration covers "<<states<<" finite scan/control states.\n";
  std::cout<<"Scope: text append ordering, end/closing-tag exits, maximum-depth failure, child failure propagation, and child append/continue are modeled; text payloads, recursive parser calls, and DOM mutation are abstracted.\n";
}
