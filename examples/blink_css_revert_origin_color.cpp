#include "../src/lambda_pi.hpp"

#include <algorithm>
#include <cassert>
#include <tuple>
#include <vector>

// Compose Blink's revert-origin candidate selection with CSS-wide color
// defaulting. Priority fields outside encoded origin/importance and rule order
// are fixed; color payloads and the color-scheme initial are abstract IDs.
using namespace lp;
enum class Origin : unsigned { None=0, UA=1, User=2, Hint=3, Author=4, Animation=5, Transition=16 };
enum class Kind { Explicit, Initial, Inherit, Unset };
struct Value { Kind kind; unsigned color=0; };
struct Candidate { Origin origin; bool important; unsigned rule; Value value; };
struct Case { Origin current; bool important; unsigned rule; std::vector<Candidate> candidates; bool has_parent; unsigned parent; unsigned initial=0; };

static unsigned encoded(Origin o,bool important){unsigned x=static_cast<unsigned>(o);return important?x^0xfu:x;}
static auto key(const Candidate&c){return std::tuple(encoded(c.origin,c.important),c.rule);}
static Origin target(Origin o){switch(o){case Origin::UA:return Origin::None;case Origin::User:return Origin::UA;case Origin::Hint:case Origin::Author:case Origin::Animation:return Origin::User;case Origin::None:case Origin::Transition:assert(false);};assert(false);return Origin::None;}
static Value cpp_revert(const Case&c){auto xs=c.candidates;std::sort(xs.begin(),xs.end(),[](const auto&a,const auto&b){return key(a)>key(b);});const Candidate* picked=nullptr;for(const auto&x:xs)if(static_cast<unsigned>(target(c.current))>=static_cast<unsigned>(x.origin)){picked=&x;break;}Candidate now{c.current,c.important,c.rule,{Kind::Unset,0}};if(!picked||picked->origin==Origin::None||key(*picked)>=key(now))return{Kind::Unset,0};return picked->value;}
static unsigned cpp_color(Value v,const Case&c){switch(v.kind){case Kind::Explicit:return v.color;case Kind::Initial:return c.initial;case Kind::Inherit:case Kind::Unset:return c.has_parent?c.parent:c.initial;}return c.initial;}

static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Ptr ap(Ptr f,std::initializer_list<Ptr>xs){for(const auto&x:xs)f=app(f,x);return f;}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr x=sym("z");while(n--)x=app(sym("s"),x);return x;}
static unsigned nat_value(Ptr x){unsigned n=0;while(auto a=std::get_if<App>(&x->v)){auto h=std::get_if<Sym>(&a->fn->v);if(!h||h->name!="s")break;++n;x=a->arg;}assert(std::get_if<Sym>(&x->v)&&std::get<Sym>(x->v).name=="z");return n;}
static Ptr value_term(Value v){switch(v.kind){case Kind::Explicit:return app(sym("explicitColor"),nat(v.color));case Kind::Initial:return sym("initialColor");case Kind::Inherit:return sym("inheritColor");case Kind::Unset:return sym("unsetColor");}assert(false);return sym("unsetColor");}
static Ptr candidate_term(const Candidate&c){return ap(sym("candidate"),{nat(static_cast<unsigned>(c.origin)),sym(c.important?"yes":"no"),nat(c.rule),value_term(c.value)});}
static Ptr list_term(std::vector<Candidate> xs){std::sort(xs.begin(),xs.end(),[](const auto&a,const auto&b){return key(a)>key(b);});Ptr out=sym("nil");for(auto i=xs.rbegin();i!=xs.rend();++i)out=ap(sym("cons"),{candidate_term(*i),out});return out;}

int main(){
  const auto UA=Origin::UA,U=Origin::User,H=Origin::Hint,A=Origin::Author,Anim=Origin::Animation;
  const Value red{Kind::Explicit,3},blue{Kind::Explicit,4};
  const std::vector<Case> cases={
    {A,false,8,{{A,false,9,red},{U,false,5,blue}},true,8}, // author skipped; user value wins
    {A,false,8,{{U,false,5,{Kind::Initial,0}}},true,8},
    {A,false,8,{{U,false,5,{Kind::Inherit,0}}},true,8},
    {A,false,8,{{U,false,5,{Kind::Inherit,0}}},false,8},
    {A,false,8,{{U,false,5,{Kind::Unset,0}}},true,8},
    {A,false,8,{{U,false,5,{Kind::Unset,0}}},false,8},
    {A,false,8,{},true,7}, // no candidate => unset => inherited parent color
    {A,false,8,{},false,7},
    {A,false,5,{{U,true,1,red},{U,false,7,blue}},true,8}, // importance projection makes pick not weaker
    {A,false,8,{{U,false,8,red}},true,8}, // equal full priority => unset
    {U,false,8,{{U,false,9,red},{UA,false,3,blue}},true,8}, // user skipped, UA selected
    {H,false,5,{{Anim,false,7,red},{U,false,3,blue}},true,8}, // target user
  };
  Signature sig;std::vector<Rule>rs;auto p=[](size_t i){return pvar(i);};
  Ptr N=sym("Nat"),B=sym("Bool"),O=sym("Ordering"),P=sym("Priority"),C=sym("Candidate"),L=sym("CandidateList"),V=sym("CssValue");
  sig["Nat"]=type_sort();sig["z"]=N;sig["s"]=fn(N,N);sig["Bool"]=type_sort();sig["yes"]=B;sig["no"]=B;sig["Ordering"]=type_sort();for(auto n:{"less","equal","greater"})sig[n]=O;
  sig["Priority"]=type_sort();sig["priorityKey"]=fn(N,fn(B,fn(N,P)));sig["cmpNat"]=fn(N,fn(N,O));sig["cmpBool"]=fn(B,fn(B,O));sig["thenCmp"]=fn(O,fn(O,O));sig["cmpPriority"]=fn(P,fn(P,O));
  sig["CssValue"]=type_sort();sig["explicitColor"]=fn(N,V);for(auto n:{"initialColor","inheritColor","unsetColor"})sig[n]=V;
  sig["Candidate"]=type_sort();sig["candidate"]=fn(N,fn(B,fn(N,fn(V,C))));sig["CandidateList"]=type_sort();sig["nil"]=L;sig["cons"]=fn(C,fn(L,L));sig["originOf"]=fn(C,N);sig["priorityOf"]=fn(C,P);sig["valueOf"]=fn(C,V);
  sig["targetOrigin"]=fn(N,N);sig["findOrigin"]=fn(L,fn(N,C));sig["findStep"]=fn(O,fn(C,fn(L,fn(N,C))));sig["resolveTo"]=fn(C,fn(P,V));sig["resolveStep"]=fn(O,fn(V,V));
  sig["parentOrInitial"]=fn(B,fn(N,fn(N,N)));sig["defaultColor"]=fn(V,fn(B,fn(N,fn(N,N))));sig["resolveColor"]=fn(L,fn(N,fn(B,fn(N,fn(B,fn(N,fn(N,N)))))));
  auto yes=sym("yes"),no=sym("no");
  for(unsigned origin:{0u,1u,2u,3u,4u,5u,16u}){rs.push_back(rule(ap(sym("priorityKey"),{nat(origin),no,nat(0)}),ap(sym("pair"),{nat(origin),nat(0)})));rs.push_back(rule(ap(sym("priorityKey"),{nat(origin),yes,nat(0)}),ap(sym("pair"),{nat(origin^0xfu),nat(0)})));}
  sig["pair"]=fn(N,fn(N,P));
  // General priority projection keeps rule order as the lower tie-breaker.
  for(unsigned origin:{0u,1u,2u,3u,4u,5u,16u})for(bool imp:{false,true})for(unsigned r=0;r<12;++r)rs.push_back(rule(ap(sym("priorityKey"),{nat(origin),imp?yes:no,nat(r)}),ap(sym("pair"),{nat(encoded(static_cast<Origin>(origin),imp)),nat(r)})));
  rs.push_back(rule(ap(sym("targetOrigin"),{nat(1)}),nat(0)));rs.push_back(rule(ap(sym("targetOrigin"),{nat(2)}),nat(1)));for(unsigned o:{3u,4u,5u})rs.push_back(rule(ap(sym("targetOrigin"),{nat(o)}),nat(2)));
  rs.push_back(rule(ap(sym("cmpNat"),{sym("z"),sym("z")}),sym("equal")));rs.push_back(rule(ap(sym("cmpNat"),{sym("z"),app(sym("s"),p(0))}),sym("less")));rs.push_back(rule(ap(sym("cmpNat"),{app(sym("s"),p(0)),sym("z")}),sym("greater")));rs.push_back(rule(ap(sym("cmpNat"),{app(sym("s"),p(0)),app(sym("s"),p(1))}),ap(sym("cmpNat"),{p(0),p(1)})));
  for(auto a:{"less","equal","greater"})for(auto b:{"less","equal","greater"})rs.push_back(rule(ap(sym("thenCmp"),{sym(a),sym(b)}),sym(std::string(a)=="equal"?b:a)));
  rs.push_back(rule(ap(sym("cmpBool"),{no,no}),sym("equal")));rs.push_back(rule(ap(sym("cmpBool"),{yes,yes}),sym("equal")));rs.push_back(rule(ap(sym("cmpBool"),{no,yes}),sym("less")));rs.push_back(rule(ap(sym("cmpBool"),{yes,no}),sym("greater")));
  rs.push_back(rule(ap(sym("cmpPriority"),{ap(sym("pair"),{p(0),p(1)}),ap(sym("pair"),{p(2),p(3)})}),ap(sym("thenCmp"),{ap(sym("cmpNat"),{p(0),p(2)}),ap(sym("cmpNat"),{p(1),p(3)})})));
  rs.push_back(rule(app(sym("originOf"),ap(sym("candidate"),{p(0),p(1),p(2),p(3)})),p(0)));rs.push_back(rule(app(sym("priorityOf"),ap(sym("candidate"),{p(0),p(1),p(2),p(3)})),ap(sym("priorityKey"),{p(0),p(1),p(2)})));rs.push_back(rule(app(sym("valueOf"),ap(sym("candidate"),{p(0),p(1),p(2),p(3)})),p(3)));
  sig["noneCandidate"]=C;rs.push_back(rule(ap(sym("findOrigin"),{sym("nil"),p(0)}),sym("noneCandidate")));rs.push_back(rule(ap(sym("findOrigin"),{ap(sym("cons"),{p(0),p(1)}),p(2)}),ap(sym("findStep"),{ap(sym("cmpNat"),{p(2),app(sym("originOf"),p(0))}),p(0),p(1),p(2)})));
  rs.push_back(rule(ap(sym("findStep"),{sym("less"),p(0),p(1),p(2)}),ap(sym("findOrigin"),{p(1),p(2)})));rs.push_back(rule(ap(sym("findStep"),{sym("equal"),p(0),p(1),p(2)}),p(0)));rs.push_back(rule(ap(sym("findStep"),{sym("greater"),p(0),p(1),p(2)}),p(0)));
  rs.push_back(rule(ap(sym("resolveTo"),{sym("noneCandidate"),p(0)}),sym("unsetColor")));rs.push_back(rule(ap(sym("resolveTo"),{ap(sym("candidate"),{nat(0),p(0),p(1),p(2)}),p(3)}),sym("unsetColor")));
  rs.push_back(rule(ap(sym("resolveTo"),{ap(sym("candidate"),{p(0),p(1),p(2),p(3)}),p(4)}),ap(sym("resolveStep"),{ap(sym("cmpPriority"),{ap(sym("priorityKey"),{p(0),p(1),p(2)}),p(4)}),p(3)})));
  rs.push_back(rule(ap(sym("resolveStep"),{sym("less"),p(0)}),p(0)));for(auto o:{"equal","greater"})rs.push_back(rule(ap(sym("resolveStep"),{sym(o),p(0)}),sym("unsetColor")));
  rs.push_back(rule(ap(sym("parentOrInitial"),{yes,p(0),p(1)}),p(0)));rs.push_back(rule(ap(sym("parentOrInitial"),{no,p(0),p(1)}),p(1)));
  rs.push_back(rule(ap(sym("defaultColor"),{ap(sym("explicitColor"),{p(0)}),p(1),p(2),p(3)}),p(0)));rs.push_back(rule(ap(sym("defaultColor"),{sym("initialColor"),p(0),p(1),p(2)}),p(2)));for(auto v:{"inheritColor","unsetColor"})rs.push_back(rule(ap(sym("defaultColor"),{sym(v),p(0),p(1),p(2)}),ap(sym("parentOrInitial"),{p(0),p(1),p(2)})));
  rs.push_back(rule(ap(sym("resolveColor"),{p(0),p(1),p(2),p(3),p(4),p(5),p(6)}),ap(sym("defaultColor"),{ap(sym("resolveTo"),{ap(sym("findOrigin"),{p(0),app(sym("targetOrigin"),p(1))}),ap(sym("priorityKey"),{p(1),p(2),p(3)})}),p(4),p(5),p(6)})));
  unsigned checks=0;for(const auto&c:cases){auto input=ap(sym("resolveColor"),{list_term(c.candidates),nat(static_cast<unsigned>(c.current)),sym(c.important?"yes":"no"),nat(c.rule),sym(c.has_parent?"yes":"no"),nat(c.parent),nat(c.initial)});auto ty=infer({},input,sig,rs);assert(ty&&conv(*ty,N,rs));auto actual=nat_value(nf(input,rs)),expected=cpp_color(cpp_revert(c),c);assert(actual==expected);++checks;}
  std::println("revert-origin selection composed with inherited color defaulting agrees on {} finite cases",checks);
  std::println("Scope: normalized origin plus rule-index priority projection, sorted per-property candidates, unset fallback, inherited color longhand; color scheme, currentColor/visited/animated paths, full CascadePriority fields and style-tree effects are abstract or omitted.");
}
