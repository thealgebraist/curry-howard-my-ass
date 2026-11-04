#include "../src/lambda_pi.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace lp;
enum Bit : unsigned { RecalcSizeContainer=0, RecalcDescendantSizeContainers=1, RecalcStyleContainerChildren=2, RecalcStyleContainerDescendants=3, RecalcScrollStateContainer=4, RecalcDescendantScrollStateContainers=5, RecalcAnchoredContainer=6, RecalcDescendantAnchoredContainers=7, RecalcDescendantContentVisibility=8, ClearPrecomputedHover=9, Reattach=10, SuppressRecalc=11, MarkReattach=12 };
static uint16_t bit(unsigned i){return static_cast<uint16_t>(1u<<i);}
static uint16_t cpp_flags_for_children(uint16_t flags,bool has_old_style,bool can_match_size_queries){
  if(!flags)return 0;
  uint16_t result=flags&~bit(RecalcStyleContainerChildren);
  const uint16_t size_and_suppress=bit(RecalcSizeContainer)|bit(RecalcDescendantSizeContainers)|bit(SuppressRecalc);
  if((result&size_and_suppress)==bit(RecalcSizeContainer)&&has_old_style&&can_match_size_queries)result&=~bit(RecalcSizeContainer);
  if(result&bit(SuppressRecalc))result&=~bit(SuppressRecalc);else result&=~bit(MarkReattach);
  return result;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr ap(Ptr f,std::initializer_list<Ptr> xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr boolean(bool b){return sym(b?"true":"false");}
static Ptr bits_term(const auto& arg){Ptr out=sym("flags");for(unsigned i=0;i<13;++i)out=app(out,arg(i));return out;}
static Ptr flag_term(uint16_t mask){return bits_term([&](unsigned i){return boolean(mask&bit(i));});}
static Ptr flags_value(uint16_t mask){return flag_term(mask);}
int main(){
  Signature sig;std::vector<Rule> rules;auto p=[](size_t i){return pvar(i);};
  Ptr B=sym("Bool"),F=sym("Flags"),ChildChange=sym("ChildChange"),Prop=sym("Propagate");
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;sig["Flags"]=type_sort();
  Ptr flags_type=F;for(unsigned i=0;i<13;++i)flags_type=arrow(B,flags_type);sig["flags"]=flags_type;
  sig["notB"]=fn(B,B);sig["andB"]=fn(B,fn(B,B));sig["ifB"]=fn(B,fn(B,fn(B,B)));
  sig["clearSizeForChild"]=fn(F,fn(B,fn(B,B)));sig["flagsForChildren"]=fn(F,fn(B,fn(B,F)));
  sig["Propagate"]=type_sort();for(const char* x:{"pNone","pPseudos","pIndependent","pChildren","pDescendants"})sig[x]=Prop;
  sig["ChildChange"]=type_sort();sig["childChange"]=fn(Prop,fn(F,ChildChange));sig["childPropForChildren"]=fn(Prop,Prop);
  sig["forChildren"]=fn(Prop,fn(F,fn(B,fn(B,ChildChange))));
  rules.push_back(R(app(sym("notB"),sym("true")),sym("false")));rules.push_back(R(app(sym("notB"),sym("false")),sym("true")));
  for(const char* x:{"true","false"})for(const char* y:{"true","false"})rules.push_back(R(ap(sym("andB"),{sym(x),sym(y)}),boolean(std::string(x)=="true"&&std::string(y)=="true")));
  rules.push_back(R(ap(sym("ifB"),{sym("true"),p(0),p(1)}),p(0)));rules.push_back(R(ap(sym("ifB"),{sym("false"),p(0),p(1)}),p(1)));
  Ptr pattern=sym("flags");for(unsigned i=0;i<13;++i)pattern=app(pattern,p(i));
  Ptr clear_condition=ap(sym("andB"),{p(0),ap(sym("andB"),{app(sym("notB"),p(1)),ap(sym("andB"),{app(sym("notB"),p(11)),ap(sym("andB"),{p(13),p(14)})})})});
  rules.push_back(R(ap(sym("clearSizeForChild"),{pattern,p(13),p(14)}),clear_condition));
  Ptr adjusted=ap(sym("flags"),{ap(sym("ifB"),{ap(sym("clearSizeForChild"),{pattern,p(13),p(14)}),sym("false"),p(0)}),p(1),sym("false"),p(3),p(4),p(5),p(6),p(7),p(8),p(9),p(10),sym("false"),ap(sym("ifB"),{p(11),p(12),sym("false")})});
  rules.push_back(R(ap(sym("flagsForChildren"),{pattern,p(13),p(14)}),adjusted));
  rules.push_back(R(app(sym("childPropForChildren"),sym("pDescendants")),sym("pDescendants")));
  for(const char* x:{"pNone","pPseudos","pIndependent","pChildren"})rules.push_back(R(app(sym("childPropForChildren"),sym(x)),sym("pNone")));
  Ptr child_rule=ap(sym("childChange"),{app(sym("childPropForChildren"),p(0)),ap(sym("flagsForChildren"),{p(1),p(2),p(3)})});
  rules.push_back(R(ap(sym("forChildren"),{p(0),p(1),p(2),p(3)}),child_rule));
  RuleIndex ix(rules);
  Ptr typed_example=ap(sym("flagsForChildren"),{flag_term(bit(RecalcSizeContainer)),sym("true"),sym("true")});auto ty=infer({},typed_example,sig,rules);assert(ty&&conv(*ty,F,ix));
  unsigned cases=0;
  for(unsigned mask=0;mask<(1u<<13);++mask)for(bool has_style:{false,true})for(bool can_match:{false,true}){
    uint16_t input=static_cast<uint16_t>(mask);Ptr actual=ap(sym("flagsForChildren"),{flag_term(input),boolean(has_style),boolean(can_match)});
    Ptr expected=flags_value(cpp_flags_for_children(input,has_style,can_match));assert(conv(nf(actual,ix),expected,ix));++cases;
  }
  unsigned prop_cases=0;
  for(const char* pname:{"pNone","pPseudos","pIndependent","pChildren","pDescendants"}){
    const uint16_t sample=bit(RecalcSizeContainer)|bit(RecalcStyleContainerChildren)|bit(SuppressRecalc)|bit(MarkReattach);
    Ptr actual=ap(sym("forChildren"),{sym(pname),flag_term(sample),sym("true"),sym("true")});
    Ptr expected=ap(sym("childChange"),{sym(std::string(pname)=="pDescendants"?"pDescendants":"pNone"),flags_value(cpp_flags_for_children(sample,true,true))});
    assert(conv(nf(actual,ix),expected,ix));++prop_cases;
  }
  std::cout<<"StyleRecalcChange::FlagsForChildren agrees on "<<cases<<" exhaustive 13-bit flag/style states\n";
  std::cout<<"ForChildren propagation and flag transform agrees on "<<prop_cases<<" propagation levels\n";
  std::cout<<"Scope: exact flag transform; Element traversal and style recalculation scheduling are excluded.\n";
}
