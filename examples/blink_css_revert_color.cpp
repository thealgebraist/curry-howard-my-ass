#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <optional>
#include <vector>

// Compose selected CascadeMap layer lookup, ResolveRevertTo's unset fallback,
// and inherited color defaulting. Layer and full-priority keys are abstract
// ordered projections; CSS color payloads are finite IDs.
using namespace lp;
enum class Kind { Explicit, Initial, Inherit, Unset };
struct CssValue { Kind kind; unsigned color; };
struct Decl { unsigned layer_key, full_key; CssValue value; };
struct Case { std::vector<Decl> strongest_first; unsigned revert_layer, current_full; bool has_parent; unsigned parent_color; };

static CssValue cpp_find_and_resolve(const Case& c) {
  const Decl* selected=nullptr;
  for(const auto& d:c.strongest_first) if(d.layer_key<c.revert_layer){selected=&d;break;}
  // ResolveRevertTo returns unset if no entry or if it is not weaker in full
  // CascadePriority order than the declaration being reverted.
  if(!selected || selected->full_key>=c.current_full) return {Kind::Unset,0};
  return selected->value;
}
static unsigned cpp_color_default(CssValue v,bool has_parent,unsigned parent,unsigned initial=0) {
  switch(v.kind){
    case Kind::Explicit:return v.color;
    case Kind::Initial:return initial;
    case Kind::Inherit:
    case Kind::Unset:return has_parent?parent:initial;
  }
  return initial;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr t=sym("z");while(n--)t=app(sym("s"),t);return t;}
static unsigned nat_value(Ptr t){unsigned n=0;while(auto a=std::get_if<App>(&t->v)){auto h=std::get_if<Sym>(&a->fn->v);if(!h||h->name!="s")break;++n;t=a->arg;}assert(std::get_if<Sym>(&t->v)&&std::get<Sym>(t->v).name=="z");return n;}
static Ptr ap(Ptr f,std::initializer_list<Ptr> xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr value_term(CssValue v){switch(v.kind){case Kind::Explicit:return app(sym("explicitColor"),nat(v.color));case Kind::Initial:return sym("initialColor");case Kind::Inherit:return sym("inheritColor");case Kind::Unset:return sym("unsetColor");}assert(false);return sym("unsetColor");}
static Ptr decl_term(const Decl& d){return ap(sym("decl"),{nat(d.layer_key),nat(d.full_key),value_term(d.value)});}
static Ptr list_term(const std::vector<Decl>& ds){Ptr xs=sym("nil");for(auto i=ds.rbegin();i!=ds.rend();++i)xs=ap(sym("cons"),{decl_term(*i),xs});return xs;}

int main(){
 Signature sig;std::vector<Rule> rs;auto p=[](size_t i){return pvar(i);};
 auto N=sym("Nat"),B=sym("Bool"),O=sym("Ordering"),V=sym("CssValue"),D=sym("Decl"),L=sym("DeclList"),C=sym("Chosen");
 sig["Nat"]=type_sort();sig["z"]=N;sig["s"]=fn(N,N);
 sig["Bool"]=type_sort();sig["false"]=B;sig["true"]=B;
 sig["Ordering"]=type_sort();sig["lt"]=O;sig["eq"]=O;sig["gt"]=O;
 sig["CssValue"]=type_sort();sig["explicitColor"]=fn(N,V);sig["initialColor"]=V;sig["inheritColor"]=V;sig["unsetColor"]=V;
 sig["Decl"]=type_sort();sig["decl"]=fn(N,fn(N,fn(V,D)));
 sig["DeclList"]=type_sort();sig["nil"]=L;sig["cons"]=fn(D,fn(L,L));
 sig["Chosen"]=type_sort();sig["none"]=C;sig["some"]=fn(N,fn(V,C));
 sig["layerOf"]=fn(D,N);sig["priorityOf"]=fn(D,N);sig["valueOf"]=fn(D,V);
 sig["cmpNat"]=fn(N,fn(N,O));sig["findLayer"]=fn(L,fn(N,C));sig["findStep"]=fn(O,fn(N,fn(V,fn(L,fn(N,C)))));
 sig["resolveTo"]=fn(C,fn(N,V));sig["resolveStep"]=fn(O,fn(V,V));
 sig["parentOrInitial"]=fn(B,fn(N,fn(N,N)));
 sig["defaultColor"]=fn(V,fn(B,fn(N,fn(N,N))));
 sig["resolveColor"]=fn(L,fn(N,fn(N,fn(B,fn(N,fn(N,N))))));
 rs.push_back(rule(app(sym("layerOf"),ap(sym("decl"),{p(0),p(1),p(2)})),p(0)));
 rs.push_back(rule(app(sym("priorityOf"),ap(sym("decl"),{p(0),p(1),p(2)})),p(1)));
 rs.push_back(rule(app(sym("valueOf"),ap(sym("decl"),{p(0),p(1),p(2)})),p(2)));
 rs.push_back(rule(ap(sym("cmpNat"),{sym("z"),sym("z")}),sym("eq")));
 rs.push_back(rule(ap(sym("cmpNat"),{sym("z"),app(sym("s"),p(0))}),sym("lt")));
 rs.push_back(rule(ap(sym("cmpNat"),{app(sym("s"),p(0)),sym("z")}),sym("gt")));
 rs.push_back(rule(ap(sym("cmpNat"),{app(sym("s"),p(0)),app(sym("s"),p(1))}),ap(sym("cmpNat"),{p(0),p(1)})));
 // Strict first-eligible layer search, preserving strongest-first list order.
 rs.push_back(rule(ap(sym("findLayer"),{sym("nil"),p(0)}),sym("none")));
 rs.push_back(rule(ap(sym("findLayer"),{ap(sym("cons"),{p(0),p(1)}),p(2)}),
   ap(sym("findStep"),{ap(sym("cmpNat"),{app(sym("layerOf"),p(0)),p(2)}),app(sym("priorityOf"),p(0)),app(sym("valueOf"),p(0)),p(1),p(2)})));
 rs.push_back(rule(ap(sym("findStep"),{sym("lt"),p(0),p(1),p(2),p(3)}),ap(sym("some"),{p(0),p(1)})));
 rs.push_back(rule(ap(sym("findStep"),{sym("eq"),p(0),p(1),p(2),p(3)}),ap(sym("findLayer"),{p(2),p(3)})));
 rs.push_back(rule(ap(sym("findStep"),{sym("gt"),p(0),p(1),p(2),p(3)}),ap(sym("findLayer"),{p(2),p(3)})));
 // Missing or not-weaker selection becomes CSS unset.
 rs.push_back(rule(ap(sym("resolveTo"),{sym("none"),p(0)}),sym("unsetColor")));
 rs.push_back(rule(ap(sym("resolveTo"),{ap(sym("some"),{p(0),p(1)}),p(2)}),
   ap(sym("resolveStep"),{ap(sym("cmpNat"),{p(0),p(2)}),p(1)})));
 rs.push_back(rule(ap(sym("resolveStep"),{sym("lt"),p(0)}),p(0)));
 rs.push_back(rule(ap(sym("resolveStep"),{sym("eq"),p(0)}),sym("unsetColor")));
 rs.push_back(rule(ap(sym("resolveStep"),{sym("gt"),p(0)}),sym("unsetColor")));
 rs.push_back(rule(ap(sym("parentOrInitial"),{sym("true"),p(0),p(1)}),p(0)));
 rs.push_back(rule(ap(sym("parentOrInitial"),{sym("false"),p(0),p(1)}),p(1)));
 rs.push_back(rule(ap(sym("defaultColor"),{app(sym("explicitColor"),p(0)),p(1),p(2),p(3)}),p(0)));
 rs.push_back(rule(ap(sym("defaultColor"),{sym("initialColor"),p(0),p(1),p(2)}),p(2)));
 rs.push_back(rule(ap(sym("defaultColor"),{sym("inheritColor"),p(0),p(1),p(2)}),ap(sym("parentOrInitial"),{p(0),p(1),p(2)})));
 rs.push_back(rule(ap(sym("defaultColor"),{sym("unsetColor"),p(0),p(1),p(2)}),ap(sym("parentOrInitial"),{p(0),p(1),p(2)})));
 rs.push_back(rule(ap(sym("resolveColor"),{p(0),p(1),p(2),p(3),p(4),p(5)}),
   ap(sym("defaultColor"),{ap(sym("resolveTo"),{ap(sym("findLayer"),{p(0),p(1)}),p(2)}),p(3),p(4),p(5)})));

 const CssValue rgb2{Kind::Explicit,2},rgb3{Kind::Explicit,3},rgb4{Kind::Explicit,4};
 const std::vector<Case> cases={
   {{{7,90,rgb3},{5,50,rgb2}},6,100,true,9},           // high/same layer skipped, lower hit
   {{{6,80,rgb3},{5,50,rgb4}},6,100,true,9},           // strict equality skipped
   {{},4,100,true,8},                                  // no candidate -> unset -> parent
   {{},4,100,false,8},                                 // root miss -> initial black
   {{{5,50,{Kind::Initial,0}}},6,100,true,8},          // explicit initial
   {{{5,50,{Kind::Inherit,0}}},6,100,true,8},          // inherit parent
   {{{5,50,{Kind::Inherit,0}}},6,100,false,8},         // inherit root -> initial
   {{{5,50,{Kind::Unset,0}}},6,100,true,8},            // unset parent
   {{{5,50,{Kind::Unset,0}}},6,100,false,8},           // unset root -> initial
   {{{5,50,rgb2}},6,100,false,8},                       // explicit value at root
   {{{5,60,rgb3},{5,59,rgb4}},6,100,true,8},           // same-layer first wins
   {{{5,120,rgb4}},6,100,true,8}                       // picked but not weaker -> unset
 };
 unsigned checks=0;
 for(const auto& c:cases){
   Ptr term=ap(sym("resolveColor"),{list_term(c.strongest_first),nat(c.revert_layer),nat(c.current_full),sym(c.has_parent?"true":"false"),nat(c.parent_color),nat(0)});
   auto ty=infer({},term,sig,rs);assert(ty&&conv(*ty,N,rs));
   unsigned expected=cpp_color_default(cpp_find_and_resolve(c),c.has_parent,c.parent_color);
   assert(nat_value(nf(term,rs))==expected);++checks;
 }
 std::cout<<"Cascade revert-layer + color defaulting agrees on "<<checks<<" finite cases\n";
 std::cout<<"Scope: strict layer search, ResolveRevertTo unset fallback, inherited color keywords; values are abstract IDs.\n";
}
