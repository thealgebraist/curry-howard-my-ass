#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

// Model of CascadeMap::FindRevertLayer over its already-sorted per-property
// list. Entries are ordered strongest-first by an abstract full priority key;
// layer is the projection used by the eligibility predicate. The list order
// also records rule/declaration tie order, which this slice does not encode.
using namespace lp;
struct Decl { unsigned layer; unsigned value; };

static unsigned cpp_find_revert_layer(const std::vector<Decl>& priorities,
                                      unsigned reverting_layer) {
  for (const auto& d : priorities) {
    if (d.layer < reverting_layer) return d.value;
  }
  return 0;  // No entry: StyleCascade::ResolveRevertTo maps this to unset.
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }
static Ptr nat(size_t n) { Ptr t=sym("z"); while(n--) t=app(sym("s"),t); return t; }
static size_t value(Ptr t) { size_t n=0; while(auto a=std::get_if<App>(&t->v)) { auto h=std::get_if<Sym>(&a->fn->v); if(!h||h->name!="s") break; ++n; t=a->arg; } assert(std::get_if<Sym>(&t->v)&&std::get<Sym>(t->v).name=="z"); return n; }
static Ptr decl_term(Decl d) { return apps(sym("d"),nat(d.layer),nat(d.value)); }
static Ptr list_term(const std::vector<Decl>& ds) { Ptr xs=sym("nil"); for(auto i=ds.rbegin();i!=ds.rend();++i) xs=apps(sym("cons"),decl_term(*i),xs); return xs; }
static Ptr ap(Ptr f, std::initializer_list<Ptr> xs) { for(auto& x:xs) f=app(f,x); return f; }

int main() {
  Signature sig; std::vector<Rule> rs; auto p=[](size_t i){return pvar(i);};
  auto natty=sym("Nat"), list=sym("Decls"), decl=sym("Decl");
  sig["Nat"]=type_sort(); sig["z"]=natty; sig["s"]=fn(natty,natty);
  sig["Decl"]=type_sort(); sig["d"]=fn(natty,fn(natty,decl));
  sig["Decls"]=type_sort(); sig["nil"]=list; sig["cons"]=fn(decl,fn(list,list));
  sig["Bool"]=type_sort(); sig["tt"]=sym("Bool"); sig["ff"]=sym("Bool");
  sig["lt"]=fn(natty,fn(natty,sym("Bool")));
  sig["firstEligible"]=fn(list,fn(natty,natty));
  sig["eligibleValue"]=fn(decl,fn(list,fn(natty,natty)));
  sig["resolve"]=fn(list,fn(natty,natty)); sig["valueOf"]=fn(decl,natty);
  sig["candidate"]=fn(natty,list); sig["dUnset"]=decl;
  rs.push_back(rule(ap(sym("lt"),{sym("z"),sym("z")}),sym("ff")));
  rs.push_back(rule(ap(sym("lt"),{sym("z"),app(sym("s"),p(0))}),sym("tt")));
  rs.push_back(rule(ap(sym("lt"),{app(sym("s"),p(0)),sym("z")}),sym("ff")));
  rs.push_back(rule(ap(sym("lt"),{app(sym("s"),p(0)),app(sym("s"),p(1))}),ap(sym("lt"),{p(0),p(1)})));
  rs.push_back(rule(app(sym("valueOf"),sym("dUnset")),sym("z")));
  rs.push_back(rule(app(sym("valueOf"),ap(sym("d"),{p(0),p(1)})),p(1)));
  rs.push_back(rule(ap(sym("firstEligible"),{sym("nil"),p(0)}),sym("z")));
  rs.push_back(rule(ap(sym("firstEligible"),{ap(sym("cons"),{p(0),p(1)}),p(2)}),
                      ap(sym("eligibleValue"),{p(0),p(1),p(2)})));
  rs.push_back(rule(ap(sym("eligibleValue"),{ap(sym("d"),{p(0),p(1)}),p(2),p(3)}),
                      ap(sym("eligibleValueStep"),{ap(sym("lt"),{p(0),p(3)}),p(0),p(1),p(2),p(3)})));
  sig["eligibleValueStep"]=fn(sym("Bool"),fn(natty,fn(natty,fn(list,fn(natty,natty)))));
  rs.push_back(rule(ap(sym("eligibleValueStep"),{sym("tt"),p(0),p(1),p(2),p(3)}),p(1)));
  rs.push_back(rule(ap(sym("eligibleValueStep"),{sym("ff"),p(0),p(1),p(2),p(3)}),ap(sym("firstEligible"),{p(2),p(3)})));
  rs.push_back(rule(ap(sym("resolve"),{p(0),p(1)}),ap(sym("firstEligible"),{p(0),p(1)})));

  // Every fixture is strongest-first, matching CascadePriorityList iteration.
  // Case 1 distinguishes tie behavior: first eligible in the sorted list wins;
  // a max-layer scan that replaces equal layers would incorrectly return 11.
  const std::vector<std::vector<Decl>> cases={
    {{2,3},{1,2},{0,1}},
    {{2,30},{1,10},{1,11},{0,1}},
    {{3,5},{0,4}},
    {{1,7}}
  };
  for(size_t i=0;i<cases.size();++i)
    rs.push_back(rule(app(sym("candidate"),nat(i)),list_term(cases[i])));
  std::vector<Ptr> checks;
  for(size_t i=0;i<cases.size();++i) for(unsigned layer=0;layer<=4;++layer) {
    unsigned expected=cpp_find_revert_layer(cases[i],layer);
    Ptr term=ap(sym("resolve"),{app(sym("candidate"),nat(i)),nat(layer)});
    auto ty=infer({},term,sig,rs); assert(ty&&conv(*ty,natty,rs));
    assert(value(nf(term,rs))==expected); checks.emplace_back(sym("tt"));
  }
  std::cout << "Blink revert-layer ordered lookup agrees on " << checks.size() << " finite inputs\n";
  std::cout << "Scope: sorted strongest-first lists; layer eligibility only; no prior candidate maps to unset (0).\n";
}
