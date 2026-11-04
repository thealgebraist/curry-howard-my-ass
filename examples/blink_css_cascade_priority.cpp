#include "../src/lambda_pi.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

// Abstract semantic comparison for Blink's packed CascadePriority. The
// high-level key follows the actual high_bits/low_bits significance order.
using namespace lp;
enum class F16 : unsigned { Z=0, One=1, MaxMinusOne=0xfffe, Max=0xffff };
enum class Origin : unsigned { None=0, UA=1, User=2, Hint=3, Author=4, Animation=5, Transition=16 };
struct Priority {
  Origin origin; bool important; uint16_t tree_order;
  bool inline_style, try_style, try_tactics_style;
  uint16_t layer_order, rule_index, declaration_index; bool already_applied;
};
static uint32_t encode_origin(Origin o,bool important){return static_cast<uint32_t>(o)^(important?0xFu:0u);}
// independent transcription of constructor packing plus high_bits/low_bits comparison
static std::pair<uint32_t,uint64_t> cpp_pack(const Priority& p){
  uint32_t high=(p.tree_order^(p.important?0xffffu:0u))|(encode_origin(p.origin,p.important)<<16);
  uint64_t low=(uint64_t{p.rule_index}<<17)|(uint64_t{p.declaration_index}<<1)|
    (uint64_t{uint16_t(p.layer_order^(p.important?0xffffu:0u))}<<33)|
    (uint64_t{p.inline_style}<<49)|(uint64_t{p.try_style}<<50)|
    (uint64_t{p.try_tactics_style}<<51)|uint64_t{p.already_applied};
  return {high,low};
}
static int cpp_compare(const Priority& a,const Priority& b){auto x=cpp_pack(a),y=cpp_pack(b);if(x.first<y.first)return -1;if(x.first>y.first)return 1;if(x.second<y.second)return -1;if(x.second>y.second)return 1;return 0;}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr t=sym("z");while(n--)t=app(sym("s"),t);return t;}
static Ptr ap(Ptr f,std::initializer_list<Ptr> xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr ap(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr field(F16 f){switch(f){case F16::Z:return sym("f0");case F16::One:return sym("f1");case F16::MaxMinusOne:return sym("fmax1");case F16::Max:return sym("fmax");}assert(false);return sym("f0");}
static Ptr origin(Origin o){return nat(static_cast<unsigned>(o));}
static Ptr flag(bool x){return sym(x?"true":"false");}
static Ptr priority_term(const Priority& p){
  return ap(sym("keyOf"),{origin(p.origin),flag(p.important),field(static_cast<F16>(p.tree_order)),flag(p.inline_style),flag(p.try_style),flag(p.try_tactics_style),field(static_cast<F16>(p.layer_order)),field(static_cast<F16>(p.rule_index)),field(static_cast<F16>(p.declaration_index)),flag(p.already_applied)});
}
static int term_compare(Ptr t){Ptr n=nf(std::move(t),{});if(std::get_if<Sym>(&n->v)){auto s=std::get<Sym>(n->v).name;return s=="less"?-1:s=="greater"?1:0;}std::cerr<<"unreduced comparison: "<<to_string(n)<<"\n";assert(false);return 0;}

int main(){
 Signature sig;std::vector<Rule> rs;auto p=[](size_t i){return pvar(i);};
 auto N=sym("Nat"),B=sym("Bool"),F=sym("Field16"),K=sym("PriorityKey"),O=sym("Ordering");
 sig["Nat"]=type_sort();sig["z"]=N;sig["s"]=fn(N,N);
 sig["Bool"]=type_sort();sig["false"]=B;sig["true"]=B;
 sig["Field16"]=type_sort();sig["f0"]=F;sig["f1"]=F;sig["fmax1"]=F;sig["fmax"]=F;
 sig["PriorityKey"]=type_sort();
 auto key_ty=K;
 const Ptr kt[]={N,F,B,B,B,F,F,F,B};
 for(auto i=8;i>=0;--i)key_ty=fn(kt[i],key_ty);
 sig["key"]=key_ty;
 sig["keyOf"]=fn(N,fn(B,fn(F,fn(B,fn(B,fn(B,fn(F,fn(F,fn(F,fn(B,K))))))))));
 sig["flip16"]=fn(F,F);sig["encode16"]=fn(B,fn(F,F));sig["encodeOrigin"]=fn(N,fn(B,N));
 sig["cmpNat"]=fn(N,fn(N,O));sig["cmp16"]=fn(F,fn(F,O));sig["cmpBool"]=fn(B,fn(B,O));
 sig["Ordering"]=type_sort();sig["less"]=O;sig["equal"]=O;sig["greater"]=O;
 sig["thenCmp"]=fn(O,fn(O,O));sig["compareKey"]=fn(K,fn(K,O));
 for(unsigned x:{0u,1u,0xfffeu,0xffffu}){
   F16 f=static_cast<F16>(x);unsigned y=x^0xffffu;F16 out=static_cast<F16>(y);
   // The chosen domain is closed under complement: {0,1,0xfffe,0xffff}.
   rs.push_back(rule(app(sym("flip16"),field(f)),field(out)));
   rs.push_back(rule(ap(sym("encode16"),{sym("false"),field(f)}),field(f)));
   rs.push_back(rule(ap(sym("encode16"),{sym("true"),field(f)}),app(sym("flip16"),field(f))));
 }
 // Origin encoding table for all CascadeOrigin enumerators in this snapshot.
 for(unsigned x:{0u,1u,2u,3u,4u,5u,16u})for(bool important:{false,true})
   rs.push_back(rule(ap(sym("encodeOrigin"),{nat(x),flag(important)}),nat(x^(important?0xfu:0u))));
 // Total comparisons on the finite Field16 domain and generic Peano origin codes.
 rs.push_back(rule(ap(sym("cmpNat"),{sym("z"),sym("z")}),sym("equal")));
 rs.push_back(rule(ap(sym("cmpNat"),{sym("z"),app(sym("s"),p(0))}),sym("less")));
 rs.push_back(rule(ap(sym("cmpNat"),{app(sym("s"),p(0)),sym("z")}),sym("greater")));
 rs.push_back(rule(ap(sym("cmpNat"),{app(sym("s"),p(0)),app(sym("s"),p(1))}),ap(sym("cmpNat"),{p(0),p(1)})));
 const std::pair<const char*,unsigned> fs[]={{"f0",0},{"f1",1},{"fmax1",0xfffe},{"fmax",0xffff}};
 for(auto [a,av]:fs)for(auto [b,bv]:fs){const char* c=av<bv?"less":av>bv?"greater":"equal";rs.push_back(rule(ap(sym("cmp16"),{sym(a),sym(b)}),sym(c)));}
 for(const char* a:{"false","true"})for(const char* b:{"false","true"}){const char* c=std::string(a)==std::string(b)?"equal":std::string(a)=="true"?"greater":"less";rs.push_back(rule(ap(sym("cmpBool"),{sym(a),sym(b)}),sym(c)));}
 for(const char* a:{"less","equal","greater"})for(const char* b:{"less","equal","greater"}){const char* c=std::string(a)!="equal"?a:b;rs.push_back(rule(ap(sym("thenCmp"),{sym(a),sym(b)}),sym(c)));}
 // Raw constructor arguments follow the source signature. Comparison order is
 // high_bits (origin/importance, tree), then low_bits (try tactics, try,
 // inline, layer, rule, declaration, already-applied), most significant first.
 std::vector<Ptr> left,right,comps,keyA,keyB;
 for(size_t i=0;i<10;++i){left.push_back(p(i));right.push_back(p(i+10));}
 auto key_args=[](const std::vector<Ptr>& v){return std::vector<Ptr>{ap(sym("encodeOrigin"),{v[0],v[1]}),ap(sym("encode16"),{v[1],v[2]}),v[5],v[4],v[3],ap(sym("encode16"),{v[1],v[6]}),v[7],v[8],v[9]};};
 // Identity wrappers make key construction uniformly typed for booleans and fields.
 sig["identityBool"]=fn(B,B);sig["identityField"]=fn(F,F);
 for(const char* b:{"false","true"})rs.push_back(rule(app(sym("identityBool"),sym(b)),sym(b)));
 for(auto [f,v]:fs)rs.push_back(rule(app(sym("identityField"),sym(f)),sym(f)));
 auto ka=key_args(left),kb=key_args(right);
 for(size_t i=0;i<9;++i){keyA.push_back(p(20+i));keyB.push_back(p(29+i));}
 const char* cmp_names[]={"cmpNat","cmp16","cmpBool","cmpBool","cmpBool","cmp16","cmp16","cmp16","cmpBool"};
 for(size_t i=0;i<keyA.size();++i)comps.push_back(ap(sym(cmp_names[i]),{keyA[i],keyB[i]}));
 Ptr lex=comps.back();for(int i=static_cast<int>(comps.size())-2;i>=0;--i)lex=ap(sym("thenCmp"),{comps[i],lex});
 rs.push_back(rule(ap(sym("compareKey"),{ap(sym("key"),keyA),ap(sym("key"),keyB)}),lex));
 rs.push_back(rule(ap(sym("keyOf"),left),ap(sym("key"),ka)));
 rs.push_back(rule(ap(sym("keyOf"),right),ap(sym("key"),kb)));

 // The C++ and λΠ observers are compared on field precedence and boundary cases.
 const uint16_t M=0xffff, M1=0xfffe;
 Priority base{Origin::Author,false,0,false,false,false,0,0,0,false};
 std::vector<std::pair<Priority,Priority>> cases;
 // Major field/origin importance precedence, with lower fields at extremes.
 Priority low=base;low.tree_order=M;low.try_tactics_style=low.try_style=low.inline_style=true;low.layer_order=M;low.rule_index=M;low.declaration_index=M;low.already_applied=true;
 Priority high=base;high.origin=Origin::Author;high.important=true;cases.emplace_back(high,low);
 Priority authorImp=base;authorImp.important=true;authorImp.rule_index=0;
 Priority userImp=authorImp;userImp.origin=Origin::User;cases.emplace_back(userImp,authorImp);
 Priority transition=base;transition.origin=Origin::Transition;Priority uaImp=base;uaImp.origin=Origin::UA;uaImp.important=true;cases.emplace_back(transition,uaImp);
 auto pairFor=[&](auto setA,auto setB){Priority a=base,b=base;setA(a);setB(b);cases.emplace_back(a,b);};
 pairFor([](Priority&x){x.tree_order=1;},[](Priority&x){x.tree_order=0;});
 pairFor([&](Priority&x){x.tree_order=M;},[](Priority&x){x.tree_order=0;});
 pairFor([&](Priority&x){x.important=true;x.tree_order=0;},[&](Priority&x){x.important=true;x.tree_order=1;});
 pairFor([](Priority&x){x.try_tactics_style=true;},[](Priority&x){x.try_style=true;});
 pairFor([](Priority&x){x.try_style=true;},[](Priority&x){x.inline_style=true;});
 pairFor([](Priority&x){x.inline_style=true;},[](Priority&x){x.layer_order=1;});
 pairFor([](Priority&x){x.layer_order=1;},[&](Priority&x){x.rule_index=M;x.declaration_index=M;x.already_applied=true;});
 pairFor([&](Priority&x){x.important=true;x.layer_order=0;},[&](Priority&x){x.important=true;x.layer_order=1;});
 pairFor([](Priority&x){x.rule_index=1;},[](Priority&x){x.declaration_index=M;x.already_applied=true;});
 pairFor([](Priority&x){x.declaration_index=1;},[](Priority&x){x.already_applied=true;});
 pairFor([](Priority&x){x.already_applied=true;},[](Priority&){});
 pairFor([](Priority&){},[](Priority&){});
 // Within-field boundary comparisons for the 16-bit tree/layer/index fields.
 pairFor([](Priority&x){x.layer_order=M;},[](Priority&x){x.layer_order=M1;});
 pairFor([](Priority&x){x.rule_index=M;},[](Priority&x){x.rule_index=M1;});
 pairFor([](Priority&x){x.declaration_index=M;},[](Priority&x){x.declaration_index=M1;});

 unsigned checked=0;
 for(const auto& [a,b]:cases){
   Ptr term=ap(sym("compareKey"),{priority_term(a),priority_term(b)});
   auto ty=infer({},term,sig,rs);assert(ty&&conv(*ty,O,rs));
   const int model=term_compare(nf(term,rs));const int cpp=cpp_compare(a,b);
   assert(model==cpp);++checked;
 }
 std::cout<<"CascadePriority semantic key agrees with packed comparison on "<<checked<<" precedence/boundary pairs\n";
 std::cout<<"Fields: origin/importance, tree, try-tactics, try, inline, layer, rule, declaration, already-applied.\n";
}
