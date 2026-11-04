#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <tuple>
#include <set>
#include <vector>

// Compose revert-origin selection with color defaulting. C++ independently
// packs the pinned 96-bit priority representation; λΠ compares its semantic
// lexicographic field tuple in the same significance order.
using namespace lp;
enum class Origin:unsigned{None=0,UA=1,User=2,Hint=3,Author=4,Animation=5,Transition=16};
enum class Kind{Explicit,Initial,Inherit,Unset};
struct Value{Kind kind;unsigned color=0;};
struct Priority{Origin origin;bool important;uint16_t tree;bool inl,try_style,try_tactics;uint16_t layer,rule,decl;bool applied;};
struct Candidate{Priority priority;Value value;};
struct Case{Priority current;std::vector<Candidate> candidates;bool has_parent;unsigned parent,initial=0;};
static uint32_t eo(Origin o,bool i){return static_cast<uint32_t>(o)^(i?0xfu:0u);}
static uint32_t et(uint16_t x,bool i){return x^(i?0xffffu:0u);}
static uint16_t el(uint16_t x,bool i){return x^(i?0xffffu:0u);}
static std::pair<uint32_t,uint64_t> pack(const Priority&p){uint32_t hi=et(p.tree,p.important)|(eo(p.origin,p.important)<<16);uint64_t lo=(uint64_t(p.rule)<<17)|(uint64_t(p.decl)<<1)|(uint64_t(el(p.layer,p.important))<<33)|(uint64_t(p.inl)<<49)|(uint64_t(p.try_style)<<50)|(uint64_t(p.try_tactics)<<51)|uint64_t(p.applied);return{hi,lo};}
static int cmp_pack(const Priority&a,const Priority&b){auto x=pack(a),y=pack(b);if(x.first!=y.first)return x.first<y.first?-1:1;if(x.second!=y.second)return x.second<y.second?-1:1;return 0;}
static Origin target(Origin o){switch(o){case Origin::UA:return Origin::None;case Origin::User:return Origin::UA;case Origin::Hint:case Origin::Author:case Origin::Animation:return Origin::User;case Origin::None:case Origin::Transition:assert(false);};assert(false);return Origin::None;}
static Value cpp_revert(const Case&c){auto xs=c.candidates;std::sort(xs.begin(),xs.end(),[](const auto&a,const auto&b){return cmp_pack(a.priority,b.priority)>0;});const Candidate*selected=nullptr;for(const auto&x:xs)if(static_cast<unsigned>(target(c.current.origin))>=static_cast<unsigned>(x.priority.origin)){selected=&x;break;}if(!selected||selected->priority.origin==Origin::None||cmp_pack(selected->priority,c.current)>=0)return{Kind::Unset,0};return selected->value;}
static unsigned cpp_color(Value v,const Case&c){switch(v.kind){case Kind::Explicit:return v.color;case Kind::Initial:return c.initial;case Kind::Inherit:case Kind::Unset:return c.has_parent?c.parent:c.initial;}return c.initial;}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}static Ptr ap(Ptr f,std::initializer_list<Ptr>xs){for(const auto&x:xs)f=app(f,x);return f;}static Ptr ap(Ptr f,const std::vector<Ptr>&xs){for(const auto&x:xs)f=app(f,x);return f;}static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr x=sym("z");while(n--)x=app(sym("s"),x);return x;}static unsigned nval(Ptr x){unsigned n=0;while(auto a=std::get_if<App>(&x->v)){auto h=std::get_if<Sym>(&a->fn->v);if(!h||h->name!="s")break;++n;x=a->arg;}assert(std::get_if<Sym>(&x->v)&&std::get<Sym>(x->v).name=="z");return n;}
static Ptr field(unsigned x);
static Ptr pri_term(const Priority&p){return ap(sym("priority"),{nat(static_cast<unsigned>(p.origin)),sym(p.important?"yes":"no"),field(p.tree),sym(p.inl?"yes":"no"),sym(p.try_style?"yes":"no"),sym(p.try_tactics?"yes":"no"),field(p.layer),field(p.rule),field(p.decl),sym(p.applied?"yes":"no")});}
static std::array<unsigned,9> key_fields(const Priority&p){return{eo(p.origin,p.important),et(p.tree,p.important),unsigned(p.try_tactics),unsigned(p.try_style),unsigned(p.inl),el(p.layer,p.important),p.rule,p.decl,unsigned(p.applied)};}
static Ptr field(unsigned x){return sym("f"+std::to_string(x));}
static Ptr key_term(const Priority&p){Ptr out=sym("key");auto fields=key_fields(p);for(auto v:fields)out=app(out,field(v));return out;}
static Ptr val_term(Value v){switch(v.kind){case Kind::Explicit:return app(sym("explicitColor"),nat(v.color));case Kind::Initial:return sym("initialColor");case Kind::Inherit:return sym("inheritColor");case Kind::Unset:return sym("unsetColor");}return sym("unsetColor");}
static Ptr cand_term(const Candidate&c){return ap(sym("candidate"),{pri_term(c.priority),val_term(c.value)});}
static Ptr list_term(std::vector<Candidate>xs){std::sort(xs.begin(),xs.end(),[](const auto&a,const auto&b){return cmp_pack(a.priority,b.priority)>0;});Ptr out=sym("nil");for(auto i=xs.rbegin();i!=xs.rend();++i)out=ap(sym("cons"),{cand_term(*i),out});return out;}

int main(){
  constexpr uint16_t M=0xffff,M1=0xfffe;
  auto make=[](Origin o=Origin::Author,bool imp=false,uint16_t tree=0,bool inl=false,bool tr=false,bool tt=false,uint16_t layer=0,uint16_t rule=0,uint16_t decl=0,bool applied=false){return Priority{o,imp,tree,inl,tr,tt,layer,rule,decl,applied};};
  Value red{Kind::Explicit,3},blue{Kind::Explicit,4};
  std::vector<Case> cases={
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::Author,false,0,false,false,false,0,9),red},{make(Origin::User,false,0,false,false,false,0,5),blue}},true,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,false,false,false,0,5),{Kind::Initial}}},true,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,false,false,false,0,5),{Kind::Inherit}}},false,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{},true,7},
   {make(Origin::Author,false,0,false,false,false,0,8),{},false,7},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,false,false,false,0,5),{Kind::Unset}}},true,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,true,0,false,false,false,0,1),red}},true,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,false,false,false,0,8),red}},true,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,false,false,false,0,9),red}},true,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,1,false,false,false,0,5),red}},true,8}, // tree order decides
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,true,false,false,0,1),blue}},true,8}, // inline flag
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,false,true,false,0,1),blue}},true,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,false,false,true,0,1),blue}},true,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,false,false,false,1,1),blue}},true,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,false,false,false,0,7,1),blue}},true,8},
   {make(Origin::Author,false,0,false,false,false,0,8),{{make(Origin::User,false,0,false,false,false,0,7,0,true),blue}},true,8},
   {make(Origin::Author,true,0,false,false,false,0,5),{{make(Origin::User,true,0,false,false,false,0,1),{Kind::Inherit}}},true,8},
   {make(Origin::UA,false,0,false,false,false,0,2),{{make(Origin::UA,false,0,false,false,false,0,1),red}},true,8},
  };
  std::vector<std::pair<Priority,Priority>> cmp_cases;
  auto pairFor=[&](auto a,auto b){Priority x=make(),y=make();a(x);b(y);cmp_cases.emplace_back(x,y);};
  pairFor([](Priority&x){x.important=true;},[](Priority&){});
  pairFor([](Priority&x){x.tree=1;},[](Priority&x){x.tree=0;});
  pairFor([](Priority&x){x.important=true;x.tree=0;},[](Priority&x){x.important=true;x.tree=1;});
  pairFor([](Priority&x){x.try_tactics=true;},[](Priority&x){x.try_style=true;});
  pairFor([](Priority&x){x.try_style=true;},[](Priority&x){x.inl=true;});
  pairFor([](Priority&x){x.inl=true;},[](Priority&x){x.layer=1;});
  pairFor([&](Priority&x){x.layer=M;},[&](Priority&x){x.layer=M1;});
  pairFor([&](Priority&x){x.important=true;x.layer=0;},[&](Priority&x){x.important=true;x.layer=1;});
  pairFor([&](Priority&x){x.rule=M;},[&](Priority&x){x.rule=M1;});
  pairFor([&](Priority&x){x.decl=M;},[&](Priority&x){x.decl=M1;});
  pairFor([](Priority&x){x.applied=true;},[](Priority&){});
  pairFor([](Priority&){},[](Priority&){});
  Signature sig;std::vector<Rule>rs;auto p=[](size_t i){return pvar(i);};Ptr N=sym("Nat"),B=sym("Bool"),O=sym("Ordering"),K=sym("PriorityKey"),P=sym("Priority"),C=sym("Candidate"),L=sym("CandidateList"),V=sym("CssValue");
  sig["Nat"]=type_sort();sig["z"]=N;sig["s"]=fn(N,N);sig["Bool"]=type_sort();sig["yes"]=B;sig["no"]=B;sig["Ordering"]=type_sort();for(auto n:{"less","equal","greater"})sig[n]=O;
  Ptr F=sym("Field");sig["Field"]=type_sort();sig["PriorityKey"]=type_sort();sig["key"]=K;for(int i=0;i<9;++i)sig["key"]=fn(F,sig["key"]);sig["priority"]=fn(N,fn(B,fn(F,fn(B,fn(B,fn(B,fn(F,fn(F,fn(F,fn(B,P))))))))));sig["keyOf"]=fn(P,K);sig["fieldRank"]=fn(F,N);sig["cmpNat"]=fn(N,fn(N,O));sig["cmpField"]=fn(F,fn(F,O));sig["cmpKey"]=fn(K,fn(K,O));sig["thenCmp"]=fn(O,fn(O,O));sig["originOfPriority"]=fn(P,N);
  sig["CssValue"]=type_sort();sig["explicitColor"]=fn(N,V);for(auto n:{"initialColor","inheritColor","unsetColor"})sig[n]=V;sig["Candidate"]=type_sort();sig["candidate"]=fn(P,fn(V,C));sig["CandidateList"]=type_sort();sig["nil"]=L;sig["cons"]=fn(C,fn(L,L));sig["originOf"]=fn(C,N);sig["priorityOf"]=fn(C,P);sig["valueOf"]=fn(C,V);sig["findOrigin"]=fn(L,fn(N,C));sig["findStep"]=fn(O,fn(C,fn(L,fn(N,C))));sig["noneCandidate"]=C;sig["resolveTo"]=fn(C,fn(P,V));sig["resolveStep"]=fn(O,fn(V,V));sig["targetOrigin"]=fn(N,N);
  sig["parentOrInitial"]=fn(B,fn(N,fn(N,N)));sig["defaultColor"]=fn(V,fn(B,fn(N,fn(N,N))));sig["resolveColor"]=fn(L,fn(P,fn(B,fn(N,fn(N,N)))));
  auto yes=sym("yes"),no=sym("no");
  // These rules are generated for each finite source priority fixture. The
  // tuple values are the source-normalized fields, not a packed-bit result.
  std::vector<Priority> ps;std::set<unsigned> field_domain{0,1,65534,65535};for(const auto&c:cases){ps.push_back(c.current);for(const auto&x:c.candidates)ps.push_back(x.priority);}for(const auto&x:cmp_cases){ps.push_back(x.first);ps.push_back(x.second);}for(const auto&pr:ps){for(auto v:key_fields(pr))field_domain.insert(v);field_domain.insert(pr.tree);field_domain.insert(pr.layer);field_domain.insert(pr.rule);field_domain.insert(pr.decl);auto lhs=pri_term(pr);rs.push_back(R(app(sym("keyOf"),lhs),key_term(pr)));}
  std::vector<unsigned> fv(field_domain.begin(),field_domain.end());for(auto v:fv)sig["f"+std::to_string(v)]=F;for(size_t i=0;i<fv.size();++i)rs.push_back(R(app(sym("fieldRank"),field(fv[i])),nat(static_cast<unsigned>(i))));rs.push_back(R(ap(sym("cmpField"),{p(0),p(1)}),ap(sym("cmpNat"),{app(sym("fieldRank"),p(0)),app(sym("fieldRank"),p(1))})));
  for(const auto&pr:ps)rs.push_back(R(app(sym("originOfPriority"),pri_term(pr)),nat(static_cast<unsigned>(pr.origin))));
  rs.push_back(R(ap(sym("cmpNat"),{sym("z"),app(sym("s"),p(0))}),sym("less")));rs.push_back(R(ap(sym("cmpNat"),{app(sym("s"),p(0)),sym("z")}),sym("greater")));rs.push_back(R(ap(sym("cmpNat"),{app(sym("s"),p(0)),app(sym("s"),p(1))}),ap(sym("cmpNat"),{p(0),p(1)})));
  // Correct zero/zero equality (the constructor above intentionally replaced below).
  rs.push_back(R(ap(sym("cmpNat"),{sym("z"),sym("z")}),sym("equal")));
  auto args=[&](size_t base){std::vector<Ptr>x;for(size_t i=0;i<9;++i)x.push_back(p(base+i));return x;};auto ka=args(0),kb=args(9);std::vector<Ptr> comps;for(size_t i=0;i<9;++i)comps.push_back(ap(sym("cmpField"),{ka[i],kb[i]}));Ptr lex=comps.back();for(int i=7;i>=0;--i)lex=ap(sym("thenCmp"),{comps[i],lex});rs.push_back(R(ap(sym("cmpKey"),{ap(sym("key"),ka),ap(sym("key"),kb)}),lex));
  sig["comparePriorities"]=fn(P,fn(P,O));rs.push_back(R(ap(sym("comparePriorities"),{p(0),p(1)}),ap(sym("cmpKey"),{app(sym("keyOf"),p(0)),app(sym("keyOf"),p(1))})));
  for(auto a:{"less","equal","greater"})for(auto b:{"less","equal","greater"})rs.push_back(R(ap(sym("thenCmp"),{sym(a),sym(b)}),sym(std::string(a)=="equal"?b:a)));
  rs.push_back(R(app(sym("originOf"),ap(sym("candidate"),{p(0),p(1)})),app(sym("originOfPriority"),p(0))));rs.push_back(R(app(sym("priorityOf"),ap(sym("candidate"),{p(0),p(1)})),p(0)));rs.push_back(R(app(sym("valueOf"),ap(sym("candidate"),{p(0),p(1)})),p(1)));
  rs.push_back(R(ap(sym("findOrigin"),{sym("nil"),p(0)}),sym("noneCandidate")));rs.push_back(R(ap(sym("findOrigin"),{ap(sym("cons"),{p(0),p(1)}),p(2)}),ap(sym("findStep"),{ap(sym("cmpNat"),{p(2),app(sym("originOf"),p(0))}),p(0),p(1),p(2)})));rs.push_back(R(ap(sym("findStep"),{sym("less"),p(0),p(1),p(2)}),ap(sym("findOrigin"),{p(1),p(2)})));rs.push_back(R(ap(sym("findStep"),{sym("equal"),p(0),p(1),p(2)}),p(0)));rs.push_back(R(ap(sym("findStep"),{sym("greater"),p(0),p(1),p(2)}),p(0)));
  rs.push_back(R(ap(sym("targetOrigin"),{nat(1)}),nat(0)));rs.push_back(R(ap(sym("targetOrigin"),{nat(2)}),nat(1)));for(unsigned x:{3u,4u,5u})rs.push_back(R(ap(sym("targetOrigin"),{nat(x)}),nat(2)));
  rs.push_back(R(ap(sym("resolveTo"),{sym("noneCandidate"),p(0)}),sym("unsetColor")));rs.push_back(R(ap(sym("resolveTo"),{ap(sym("candidate"),{pri_term(make(Origin::None)),p(0)}),p(1)}),sym("unsetColor")));rs.push_back(R(ap(sym("resolveTo"),{ap(sym("candidate"),{p(0),p(1)}),p(2)}),ap(sym("resolveStep"),{ap(sym("cmpKey"),{app(sym("keyOf"),p(0)),app(sym("keyOf"),p(2))}),p(1)})));rs.push_back(R(ap(sym("resolveStep"),{sym("less"),p(0)}),p(0)));for(auto o:{"equal","greater"})rs.push_back(R(ap(sym("resolveStep"),{sym(o),p(0)}),sym("unsetColor")));
  rs.push_back(R(ap(sym("parentOrInitial"),{yes,p(0),p(1)}),p(0)));rs.push_back(R(ap(sym("parentOrInitial"),{no,p(0),p(1)}),p(1)));rs.push_back(R(ap(sym("defaultColor"),{ap(sym("explicitColor"),{p(0)}),p(1),p(2),p(3)}),p(0)));rs.push_back(R(ap(sym("defaultColor"),{sym("initialColor"),p(0),p(1),p(2)}),p(2)));for(auto v:{"inheritColor","unsetColor"})rs.push_back(R(ap(sym("defaultColor"),{sym(v),p(0),p(1),p(2)}),ap(sym("parentOrInitial"),{p(0),p(1),p(2)})));
  rs.push_back(R(ap(sym("resolveColor"),{p(0),p(1),p(2),p(3),p(4)}),ap(sym("defaultColor"),{ap(sym("resolveTo"),{ap(sym("findOrigin"),{p(0),app(sym("targetOrigin"),app(sym("originOfPriority"),p(1)))}),p(1)}),p(2),p(3),p(4)})));
  RuleIndex indexed(rs);unsigned count=0;for(size_t i=0;i<cases.size();++i){const auto&c=cases[i];Ptr term=ap(sym("resolveColor"),{list_term(c.candidates),pri_term(c.current),sym(c.has_parent?"yes":"no"),nat(c.parent),nat(c.initial)});auto ty=infer({},term,sig,indexed);assert(ty&&conv(*ty,N,indexed));auto actual=nval(nf(term,indexed)),expected=cpp_color(cpp_revert(c),c);if(actual!=expected){std::println("case {} mismatch actual {} expected {}",i,actual,expected);return 2;}++count;}
  unsigned compared=0;for(const auto&[a,b]:cmp_cases){auto term=ap(sym("comparePriorities"),{pri_term(a),pri_term(b)});auto ty=infer({},term,sig,indexed);assert(ty&&conv(*ty,O,indexed));auto got=nf(term,indexed);auto expected=cmp_pack(a,b);const char* label=expected<0?"less":expected>0?"greater":"equal";if(!std::get_if<Sym>(&got->v)||std::get<Sym>(got->v).name!=label){std::println("priority pair mismatch expected {} got {}",label,to_string(got));return 3;}++compared;}
  std::println("full CascadePriority tuple comparison composed with revert-origin color defaulting agrees on {} finite cases",count);
  std::println("λΠ semantic key and C++ packed comparator agree on {} field precedence/boundary pairs",compared);
  std::println("Scope: semantic tuple follows pinned high/low field order and finite 16-bit encodings; C++ side uses packed high_bits/low_bits transcription. No full 96-bit λΠ packing, arbitrary uint16 domain proof, declaration collection, color-scheme/currentColor/visited/animated behavior, or style lifecycle.");
}
