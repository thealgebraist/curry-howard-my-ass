#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

enum class Children { EarlierFailure, EndOfInput, ClosingTagReady };
enum class NameMatch { Exact, AsciiCaseEquivalent, Mismatch };
enum class Result { EarlierFailurePreserved, ContainerEndFailure,
                    UnexpectedCloseState, EndTagMismatch, Closed };
static Result cpp_close(Children children,bool enough,NameMatch name,bool greater){
  if(children==Children::EarlierFailure)return Result::EarlierFailurePreserved;
  if(children==Children::EndOfInput)return Result::ContainerEndFailure;
  if(!enough)return Result::UnexpectedCloseState;
  if(name==NameMatch::Mismatch)return Result::EndTagMismatch;
  return greater?Result::Closed:Result::UnexpectedCloseState;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr child(Children c){switch(c){case Children::EarlierFailure:return sym("childFailure");case Children::EndOfInput:return sym("childAtEnd");case Children::ClosingTagReady:return sym("closingTagReady");}return sym("childFailure");}
static Ptr match(NameMatch m){switch(m){case NameMatch::Exact:return sym("exactTagName");case NameMatch::AsciiCaseEquivalent:return sym("asciiCaseTagName");case NameMatch::Mismatch:return sym("mismatchedTagName");}return sym("mismatchedTagName");}
static Ptr result(Result r){switch(r){case Result::EarlierFailurePreserved:return sym("preserveChildFailure");case Result::ContainerEndFailure:return sym("failContainerAtEnd");case Result::UnexpectedCloseState:return sym("failUnexpectedCloseState");case Result::EndTagMismatch:return sym("failEndTagMismatch");case Result::Closed:return sym("containerClosed");}return sym("failUnexpectedCloseState");}
static Ptr bt(bool b){return sym(b?"true":"false");}
static Ptr query(Children c,bool enough,NameMatch m,bool greater){return apps(sym("closeContainer"),{child(c),bt(enough),match(m),bt(greater)});}

int main(){
  const Ptr B=sym("Bool"),C=sym("ChildParseState"),M=sym("EndTagNameMatch"),R=sym("ContainerCloseResult");
  Signature sig;std::vector<Rule> rules;
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["ChildParseState"]=type_sort();sig["childFailure"]=C;sig["childAtEnd"]=C;sig["closingTagReady"]=C;
  sig["EndTagNameMatch"]=type_sort();sig["exactTagName"]=M;sig["asciiCaseTagName"]=M;sig["mismatchedTagName"]=M;
  sig["ContainerCloseResult"]=type_sort();for(const char* x:{"preserveChildFailure","failContainerAtEnd","failUnexpectedCloseState","failEndTagMismatch","containerClosed"})sig[x]=R;
  sig["closeContainer"]=fn(C,fn(B,fn(M,fn(B,R))));
  // ParseContainerElement preserves a failure already recorded by its child parser.
  rules.push_back(rule(apps(sym("closeContainer"),{sym("childFailure"),pvar(0),pvar(1),pvar(2)}),sym("preserveChildFailure")));
  rules.push_back(rule(apps(sym("closeContainer"),{sym("childAtEnd"),pvar(3),pvar(4),pvar(5)}),sym("failContainerAtEnd")));
  rules.push_back(rule(apps(sym("closeContainer"),{sym("closingTagReady"),sym("false"),pvar(6),pvar(7)}),sym("failUnexpectedCloseState")));
  rules.push_back(rule(apps(sym("closeContainer"),{sym("closingTagReady"),sym("true"),sym("mismatchedTagName"),pvar(8)}),sym("failEndTagMismatch")));
  rules.push_back(rule(apps(sym("closeContainer"),{sym("closingTagReady"),sym("true"),sym("exactTagName"),sym("true")}),sym("containerClosed")));
  rules.push_back(rule(apps(sym("closeContainer"),{sym("closingTagReady"),sym("true"),sym("asciiCaseTagName"),sym("true")}),sym("containerClosed")));
  rules.push_back(rule(apps(sym("closeContainer"),{sym("closingTagReady"),sym("true"),sym("exactTagName"),sym("false")}),sym("failUnexpectedCloseState")));
  rules.push_back(rule(apps(sym("closeContainer"),{sym("closingTagReady"),sym("true"),sym("asciiCaseTagName"),sym("false")}),sym("failUnexpectedCloseState")));
  RuleIndex index(rules);auto sample=query(Children::ClosingTagReady,true,NameMatch::Exact,true);auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,R,index));
  unsigned states=0;
  for(Children c:{Children::EarlierFailure,Children::EndOfInput,Children::ClosingTagReady})for(bool enough:{false,true})for(NameMatch m:{NameMatch::Exact,NameMatch::AsciiCaseEquivalent,NameMatch::Mismatch})for(bool gt:{false,true}){
    if(!conv(nf(query(c,enough,m,gt),index),result(cpp_close(c,enough,m,gt)),index)){std::cerr<<"ParseContainerElement close result mismatch\n";return 2;}
    ++states;
  }
  std::cout<<"HTML fast-path container closing-tag outcomes cover "<<states<<" finite child/name/length/delimiter states.\n";
  std::cout<<"Scope: prior child failure preservation, EOF, fixed tag-name length, exact/ASCII-case matching, whitespace-skipped > validation, and successful finish are modeled; byte scanning and FinishParsingChildren effects are abstracted.\n";
}
