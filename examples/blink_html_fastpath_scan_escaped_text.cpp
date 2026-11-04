#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

enum class Code { A, B, C, Wide, Newline };
enum class Kind { Literal, CarriageReturn, CarriageReturnLineFeed, LessThan,
                  NullCharacter, ReferenceDecoded, ReferenceFailed };
struct Item { Kind kind; std::vector<Code> decoded; };
struct Result { enum Kind { Success, NullFailure, ReferenceFailure } kind; std::vector<Code> text; std::vector<Item> rest; };
static Result cpp_scan(const std::vector<Item>& input){
  std::vector<Code> output;
  for(size_t i=0;i<input.size();++i){
    const auto& x=input[i];
    switch(x.kind){
      case Kind::Literal:output.insert(output.end(),x.decoded.begin(),x.decoded.end());break;
      case Kind::CarriageReturn:case Kind::CarriageReturnLineFeed:output.push_back(Code::Newline);break;
      case Kind::LessThan:return {Result::Success,output,std::vector<Item>(input.begin()+i,input.end())};
      case Kind::NullCharacter:return {Result::NullFailure,{},std::vector<Item>(input.begin()+i,input.end())};
      case Kind::ReferenceDecoded:output.insert(output.end(),x.decoded.begin(),x.decoded.end());break;
      case Kind::ReferenceFailed:return {Result::ReferenceFailure,{},{}};
    }
  }
  return {Result::Success,output,{}};
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static const char* code_name(Code c){switch(c){case Code::A:return "unitA";case Code::B:return "unitB";case Code::C:return "unitC";case Code::Wide:return "unitWide";case Code::Newline:return "unitNewline";}return "unitA";}
static Ptr code_list(const std::vector<Code>& xs){Ptr t=sym("nilUnits");for(auto i=xs.rbegin();i!=xs.rend();++i)t=apps(sym("consUnit"),{sym(code_name(*i)),t});return t;}
static Ptr item_term(const Item& x){switch(x.kind){case Kind::Literal:return app(sym("literal"),code_list(x.decoded));case Kind::CarriageReturn:return sym("carriageReturn");case Kind::CarriageReturnLineFeed:return sym("carriageReturnLineFeed");case Kind::LessThan:return sym("lessThanItem");case Kind::NullCharacter:return sym("nullItem");case Kind::ReferenceDecoded:return app(sym("referenceDecoded"),code_list(x.decoded));case Kind::ReferenceFailed:return sym("referenceFailed");}return sym("referenceFailed");}
static Ptr item_list(const std::vector<Item>& xs){Ptr t=sym("nilItems");for(auto i=xs.rbegin();i!=xs.rend();++i)t=apps(sym("consItem"),{item_term(*i),t});return t;}
static Ptr query(const std::vector<Item>& xs){return app(sym("scanEscapedText"),item_list(xs));}
static Ptr result_term(const Result& r){if(r.kind==Result::NullFailure)return app(sym("escapedNullFailure"),item_list(r.rest));if(r.kind==Result::ReferenceFailure)return sym("escapedReferenceFailure");return apps(sym("escapedTextResult"),{code_list(r.text),item_list(r.rest)});}

int main(){
  const Ptr C=sym("OutputCodeUnit"),CL=sym("OutputText"),I=sym("EscapedItem"),IL=sym("EscapedInput"),R=sym("EscapedTextResult");Signature sig;std::vector<Rule> rules;
  sig["OutputCodeUnit"]=type_sort();for(const char* x:{"unitA","unitB","unitC","unitWide","unitNewline"})sig[x]=C;
  sig["OutputText"]=type_sort();sig["nilUnits"]=CL;sig["consUnit"]=fn(C,fn(CL,CL));
  sig["EscapedItem"]=type_sort();sig["literal"]=fn(CL,I);sig["carriageReturn"]=I;sig["carriageReturnLineFeed"]=I;sig["lessThanItem"]=I;sig["nullItem"]=I;sig["referenceDecoded"]=fn(CL,I);sig["referenceFailed"]=I;
  sig["EscapedInput"]=type_sort();sig["nilItems"]=IL;sig["consItem"]=fn(I,fn(IL,IL));
  sig["EscapedTextResult"]=type_sort();sig["escapedTextResult"]=fn(CL,fn(IL,R));sig["escapedNullFailure"]=fn(IL,R);sig["escapedReferenceFailure"]=R;
  sig["appendText"]=fn(CL,fn(CL,CL));sig["scanEscapedLoop"]=fn(CL,fn(IL,R));sig["scanEscapedText"]=fn(IL,R);
  rules.push_back(rule(apps(sym("appendText"),{sym("nilUnits"),pvar(0)}),pvar(0)));
  rules.push_back(rule(apps(sym("appendText"),{apps(sym("consUnit"),{pvar(1),pvar(2)}),pvar(3)}),apps(sym("consUnit"),{pvar(1),apps(sym("appendText"),{pvar(2),pvar(3)})})));
  rules.push_back(rule(app(sym("scanEscapedText"),pvar(4)),apps(sym("scanEscapedLoop"),{sym("nilUnits"),pvar(4)})));
  rules.push_back(rule(apps(sym("scanEscapedLoop"),{pvar(5),sym("nilItems")}),apps(sym("escapedTextResult"),{pvar(5),sym("nilItems")})));
  rules.push_back(rule(apps(sym("scanEscapedLoop"),{pvar(6),apps(sym("consItem"),{sym("lessThanItem"),pvar(7)})}),apps(sym("escapedTextResult"),{pvar(6),apps(sym("consItem"),{sym("lessThanItem"),pvar(7)})})));
  rules.push_back(rule(apps(sym("scanEscapedLoop"),{pvar(8),apps(sym("consItem"),{sym("nullItem"),pvar(9)})}),app(sym("escapedNullFailure"),apps(sym("consItem"),{sym("nullItem"),pvar(9)}))));
  rules.push_back(rule(apps(sym("scanEscapedLoop"),{pvar(10),apps(sym("consItem"),{sym("referenceFailed"),pvar(11)})}),sym("escapedReferenceFailure")));
  for(const char* kind:{"carriageReturn","carriageReturnLineFeed"})rules.push_back(rule(apps(sym("scanEscapedLoop"),{pvar(12),apps(sym("consItem"),{sym(kind),pvar(13)})}),apps(sym("scanEscapedLoop"),{apps(sym("appendText"),{pvar(12),apps(sym("consUnit"),{sym("unitNewline"),sym("nilUnits")})}),pvar(13)})));
  rules.push_back(rule(apps(sym("scanEscapedLoop"),{pvar(14),apps(sym("consItem"),{app(sym("literal"),pvar(15)),pvar(16)})}),apps(sym("scanEscapedLoop"),{apps(sym("appendText"),{pvar(14),pvar(15)}),pvar(16)})));
  rules.push_back(rule(apps(sym("scanEscapedLoop"),{pvar(17),apps(sym("consItem"),{app(sym("referenceDecoded"),pvar(18)),pvar(19)})}),apps(sym("scanEscapedLoop"),{apps(sym("appendText"),{pvar(17),pvar(18)}),pvar(19)})));
  RuleIndex index(rules);
  const std::vector<std::vector<Item>> cases={{},{ {Kind::Literal,{Code::A,Code::B}} },{{Kind::Literal,{Code::A}},{Kind::LessThan,{}},{Kind::Literal,{Code::B}}},{{Kind::CarriageReturn,{}},{Kind::Literal,{Code::A}}},{{Kind::CarriageReturnLineFeed,{}},{Kind::Literal,{Code::B}}},{{Kind::ReferenceDecoded,{Code::C}}},{{Kind::Literal,{Code::A}},{Kind::ReferenceDecoded,{Code::B,Code::C}}},{{Kind::Literal,{Code::A}},{Kind::ReferenceFailed,{}}},{{Kind::ReferenceFailed,{}}},{{Kind::Literal,{Code::Wide,Code::A}}},{{Kind::Literal,{Code::A}},{Kind::NullCharacter,{}},{Kind::Literal,{Code::B}}},{{Kind::LessThan,{}}}};
  auto sample=query(cases[2]);auto inferred=infer({},sample,sig,rules);assert(inferred&&conv(*inferred,R,index));
  unsigned states=0;
  for(const auto& input:cases){if(!conv(nf(query(input),index),result_term(cpp_scan(input)),index)){std::cerr<<"ScanEscapedText branch mismatch\n";return 2;}++states;}
  std::cout<<"HTML fast-path ScanEscapedText covers "<<states<<" finite input sequences.\n";
  std::cout<<"Scope: literal and decoder-produced text accumulation, CR/CRLF normalization, unconsumed '<', and null/reference failure are modeled. Entity parsing/consumption, concrete source cursor offsets, string canonicalization, and buffer allocation are abstracted.\n";
}
