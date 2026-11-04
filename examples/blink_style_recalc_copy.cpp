#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <string>
using namespace lp;
struct Guards { bool parent, old, independent; bool pseudo, dirty, animations, callbacks; bool style, style_animations, transitions, variable_ref, variable_decl, inside_link, decorations, derived_color; };
static bool cpp_copy(const Guards& g) {
  return g.parent && g.old && g.independent && !g.pseudo && !g.dirty &&
    !g.animations && !g.callbacks && g.style && !g.style_animations &&
    !g.transitions && !g.variable_ref && !g.variable_decl && !g.inside_link &&
    !g.decorations && !g.derived_color;
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a),std::move(b)); }
static Rule R(Ptr a, Ptr b) { return {std::move(a),std::move(b)}; }
static Ptr ap(Ptr f, std::initializer_list<Ptr> xs) { for (const auto& x:xs) f=app(f,x); return f; }
static Ptr bt(bool b) { return sym(b?"true":"false"); }
static Ptr normalize(Ptr t,const RuleIndex& ix) { for(int i=0;i<16;++i)t=nf(t,ix); return t; }
int main() {
  Signature sig; std::vector<Rule> rules; Ptr B=sym("Bool"); sig["Bool"]=type_sort(); sig["true"]=B; sig["false"]=B;
  sig["notB"]=fn(B,B); sig["andB"]=fn(B,fn(B,B));
  Ptr result=B; for(int i=0;i<15;++i) result=fn(B,result); sig["copyFastPath"]=result;
  rules.push_back(R(app(sym("notB"),sym("true")),sym("false"))); rules.push_back(R(app(sym("notB"),sym("false")),sym("true")));
  for(bool x:{false,true})for(bool y:{false,true})rules.push_back(R(ap(sym("andB"),{bt(x),bt(y)}),bt(x&&y)));
  auto v=[](size_t i){return pvar(i);}; Ptr body=ap(sym("andB"),{v(0),ap(sym("andB"),{v(1),ap(sym("andB"),{v(2),v(7)})})});
  for(size_t i=3;i<15;++i) if(i!=7) body=ap(sym("andB"),{body,app(sym("notB"),v(i))});
  rules.push_back(R(ap(sym("copyFastPath"),{v(0),v(1),v(2),v(3),v(4),v(5),v(6),v(7),v(8),v(9),v(10),v(11),v(12),v(13),v(14)}),body));
  RuleIndex ix(rules);
  Ptr sample=ap(sym("copyFastPath"),{sym("true"),sym("true"),sym("true"),sym("false"),sym("false"),sym("false"),sym("false"),sym("true"),sym("false"),sym("false"),sym("false"),sym("false"),sym("false"),sym("false"),sym("false")});
  auto sample_type=infer({},sample,sig,rules);
  assert(sample_type && conv(*sample_type,B,ix));
  unsigned states=0,fast=0;
  for(unsigned mask=0;mask<(1u<<15);++mask) {
    bool b[15];for(unsigned i=0;i<15;++i)b[i]=(mask>>i)&1;
    Guards g{b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],b[10],b[11],b[12],b[13],b[14]};
    Ptr term=ap(sym("copyFastPath"),{bt(b[0]),bt(b[1]),bt(b[2]),bt(b[3]),bt(b[4]),bt(b[5]),bt(b[6]),bt(b[7]),bt(b[8]),bt(b[9]),bt(b[10]),bt(b[11]),bt(b[12]),bt(b[13]),bt(b[14])});
    bool expected=cpp_copy(g); if(!conv(normalize(term,ix),bt(expected),ix)){std::cerr<<"mismatch mask="<<mask<<" got="<<to_string(normalize(term,ix))<<" expected="<<to_string(bt(expected))<<"\n";return 2;} ++states;fast+=expected;
  }
  std::cout<<"Fast-path eligibility agrees on "<<states<<" guard states ("<<fast<<" copies; "<<states-fast<<" shortcut declines).\n";
  std::cout<<"Scope: boolean branch boundary; generated CSS property copying and subsequent style diff are not represented.\n";
}
