#include "../src/lambda_pi.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

// Compose a fixed-origin/tree/style-flag projection of CascadePriority with
// CascadeMap's per-property sorted-list behavior. The varying packed fields
// are layer, rule index, declaration index, and already-applied.
using namespace lp;
struct Decl { uint16_t layer, rule, declaration; bool applied; unsigned value; };

static uint64_t cpp_low_bits(const Decl& d) {
  return (uint64_t{d.layer}<<33)|(uint64_t{d.rule}<<17)|
         (uint64_t{d.declaration}<<1)|uint64_t{d.applied};
}
static void cpp_add(std::vector<Decl>& list, Decl incoming) {
  // With the fixed prefix used here, ForLayerComparison order is layer order.
  // CascadeMap::Add DCHECKs that incoming layer comparison is not below top.
  assert(list.empty() || incoming.layer>=list.front().layer);
  if(list.empty() || cpp_low_bits(incoming)>cpp_low_bits(list.front())) {
    list.insert(list.begin(),incoming);  // Push
    return;
  }
  auto at=list.begin();
  while(at!=list.end() && cpp_low_bits(incoming)<cpp_low_bits(*at)) ++at;
  list.insert(at,incoming);  // >= inserts before equal; matches InsertKeepingSorted
}
static unsigned cpp_find_revert_layer(const std::vector<Decl>& list,
                                      unsigned reverting_layer) {
  for(const auto& d:list) if(d.layer<reverting_layer) return d.value;
  return 0;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr t=sym("z");while(n--)t=app(sym("s"),t);return t;}
static unsigned nat_value(Ptr t){unsigned n=0;while(auto a=std::get_if<App>(&t->v)){auto h=std::get_if<Sym>(&a->fn->v);if(!h||h->name!="s")break;++n;t=a->arg;}assert(std::get_if<Sym>(&t->v)&&std::get<Sym>(t->v).name=="z");return n;}
static Ptr ap(Ptr f,std::initializer_list<Ptr> xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr dterm(Decl d){return ap(sym("d"),{nat(d.layer),nat(d.rule),nat(d.declaration),sym(d.applied?"true":"false"),nat(d.value)});}
static Ptr list_term(const std::vector<Decl>& ds){Ptr out=sym("nil");for(auto it=ds.rbegin();it!=ds.rend();++it)out=ap(sym("cons"),{dterm(*it),out});return out;}

int main(){
 Signature sig;std::vector<Rule> rs;auto p=[](size_t i){return pvar(i);};
 auto N=sym("Nat"),B=sym("Bool"),O=sym("Ordering"),D=sym("Decl"),L=sym("DeclList"),P=sym("Priority");
 sig["Nat"]=type_sort();sig["z"]=N;sig["s"]=fn(N,N);
 sig["Bool"]=type_sort();sig["false"]=B;sig["true"]=B;
 sig["Ordering"]=type_sort();sig["lt"]=O;sig["eq"]=O;sig["gt"]=O;
 sig["Decl"]=type_sort();sig["d"]=fn(N,fn(N,fn(N,fn(B,fn(N,D)))));
 sig["DeclList"]=type_sort();sig["nil"]=L;sig["cons"]=fn(D,fn(L,L));
 sig["Priority"]=type_sort();sig["priority"]=fn(N,fn(N,fn(N,fn(B,P))));
 sig["priorityOf"]=fn(D,P);sig["layerOf"]=fn(D,N);sig["valueOf"]=fn(D,N);
 sig["cmpNat"]=fn(N,fn(N,O));sig["cmpBool"]=fn(B,fn(B,O));sig["thenCmp"]=fn(O,fn(O,O));
 sig["comparePriority"]=fn(P,fn(P,O));
 sig["insert"]=fn(L,fn(D,L));sig["insertStep"]=fn(O,fn(L,fn(D,L)));
 sig["insertSorted"]=fn(L,fn(D,L));sig["insertSortedStep"]=fn(O,fn(D,fn(L,fn(D,L))));
 sig["build"]=fn(L,fn(L,L));
 sig["findRevertLayer"]=fn(L,fn(N,N));sig["findStep"]=fn(O,fn(N,fn(L,fn(N,N))));

 rs.push_back(rule(app(sym("priorityOf"),ap(sym("d"),{p(0),p(1),p(2),p(3),p(4)})),ap(sym("priority"),{p(0),p(1),p(2),p(3)})));
 rs.push_back(rule(app(sym("layerOf"),ap(sym("d"),{p(0),p(1),p(2),p(3),p(4)})),p(0)));
 rs.push_back(rule(app(sym("valueOf"),ap(sym("d"),{p(0),p(1),p(2),p(3),p(4)})),p(4)));
 rs.push_back(rule(ap(sym("cmpNat"),{sym("z"),sym("z")}),sym("eq")));
 rs.push_back(rule(ap(sym("cmpNat"),{sym("z"),app(sym("s"),p(0))}),sym("lt")));
 rs.push_back(rule(ap(sym("cmpNat"),{app(sym("s"),p(0)),sym("z")}),sym("gt")));
 rs.push_back(rule(ap(sym("cmpNat"),{app(sym("s"),p(0)),app(sym("s"),p(1))}),ap(sym("cmpNat"),{p(0),p(1)})));
 for(const char* a:{"false","true"})for(const char* b:{"false","true"}){
   const char* order=std::string(a)==std::string(b)?"eq":std::string(a)=="true"?"gt":"lt";
   rs.push_back(rule(ap(sym("cmpBool"),{sym(a),sym(b)}),sym(order)));
 }
 rs.push_back(rule(ap(sym("thenCmp"),{sym("lt"),p(0)}),sym("lt")));
 rs.push_back(rule(ap(sym("thenCmp"),{sym("gt"),p(0)}),sym("gt")));
 rs.push_back(rule(ap(sym("thenCmp"),{sym("eq"),p(0)}),p(0)));
 // Source priority order under the fixed prefix: layer, rule, declaration,
 // already-applied. This is the low-word order from CascadePriority.
 std::vector<Ptr> ka, kb, cmps;
 for(size_t i=0;i<8;++i){ka.push_back(p(i));kb.push_back(p(i+8));}
 auto mkprio=[](const std::vector<Ptr>& v){return ap(sym("priority"),{v[0],v[1],v[2],v[3]});};
 cmps={ap(sym("cmpNat"),{ka[0],kb[0]}),ap(sym("cmpNat"),{ka[1],kb[1]}),ap(sym("cmpNat"),{ka[2],kb[2]}),ap(sym("cmpBool"),{ka[3],kb[3]})};
 Ptr lex=cmps.back();for(int i=static_cast<int>(cmps.size())-2;i>=0;--i)lex=ap(sym("thenCmp"),{cmps[i],lex});
 rs.push_back(rule(ap(sym("comparePriority"),{mkprio(ka),mkprio(kb)}),lex));

 rs.push_back(rule(ap(sym("insert"),{sym("nil"),p(0)}),ap(sym("cons"),{p(0),sym("nil")})));
 rs.push_back(rule(ap(sym("insert"),{ap(sym("cons"),{p(0),p(1)}),p(2)}),
   ap(sym("insertStep"),{ap(sym("comparePriority"),{app(sym("priorityOf"),p(2)),app(sym("priorityOf"),p(0))}),ap(sym("cons"),{p(0),p(1)}),p(2)})));
 rs.push_back(rule(ap(sym("insertStep"),{sym("gt"),p(0),p(1)}),ap(sym("cons"),{p(1),p(0)})));
 rs.push_back(rule(ap(sym("insertStep"),{sym("eq"),p(0),p(1)}),ap(sym("insertSorted"),{p(0),p(1)})));
 rs.push_back(rule(ap(sym("insertStep"),{sym("lt"),p(0),p(1)}),ap(sym("insertSorted"),{p(0),p(1)})));
 rs.push_back(rule(ap(sym("insertSorted"),{sym("nil"),p(0)}),ap(sym("cons"),{p(0),sym("nil")})));
 rs.push_back(rule(ap(sym("insertSorted"),{ap(sym("cons"),{p(0),p(1)}),p(2)}),
   ap(sym("insertSortedStep"),{ap(sym("comparePriority"),{app(sym("priorityOf"),p(2)),app(sym("priorityOf"),p(0))}),p(0),p(1),p(2)})));
 rs.push_back(rule(ap(sym("insertSortedStep"),{sym("lt"),p(0),p(1),p(2)}),ap(sym("cons"),{p(0),ap(sym("insertSorted"),{p(1),p(2)})})));
 rs.push_back(rule(ap(sym("insertSortedStep"),{sym("eq"),p(0),p(1),p(2)}),ap(sym("cons"),{p(2),ap(sym("cons"),{p(0),p(1)})})));
 rs.push_back(rule(ap(sym("insertSortedStep"),{sym("gt"),p(0),p(1),p(2)}),ap(sym("cons"),{p(2),ap(sym("cons"),{p(0),p(1)})})));
 rs.push_back(rule(ap(sym("build"),{sym("nil"),p(0)}),p(0)));
 rs.push_back(rule(ap(sym("build"),{ap(sym("cons"),{p(0),p(1)}),p(2)}),ap(sym("build"),{p(1),ap(sym("insert"),{p(2),p(0)})})));

 // ForLayerComparison in this fixed-origin/tree/style-flag slice is monotone
 // in the layer field. FindRevertLayer uses a strict comparison and returns
 // the first eligible entry in the strongest-first list.
 rs.push_back(rule(ap(sym("findRevertLayer"),{sym("nil"),p(0)}),sym("z")));
 rs.push_back(rule(ap(sym("findRevertLayer"),{ap(sym("cons"),{p(0),p(1)}),p(2)}),
   ap(sym("findStep"),{ap(sym("cmpNat"),{app(sym("layerOf"),p(0)),p(2)}),app(sym("valueOf"),p(0)),p(1),p(2)})));
 rs.push_back(rule(ap(sym("findStep"),{sym("lt"),p(0),p(1),p(2)}),p(0)));
 rs.push_back(rule(ap(sym("findStep"),{sym("eq"),p(0),p(1),p(2)}),ap(sym("findRevertLayer"),{p(1),p(2)})));
 rs.push_back(rule(ap(sym("findStep"),{sym("gt"),p(0),p(1),p(2)}),ap(sym("findRevertLayer"),{p(1),p(2)})));

 const std::vector<std::vector<Decl>> inputs={
   {{5,6,0,false,10},{5,4,0,false,20},{5,4,0,false,21},{6,1,0,false,30},{7,0,0,false,40}},
   {{2,3,0,false,1},{2,3,0,false,2},{4,6,0,false,4},{4,2,0,false,5}}
 };
 unsigned insertion_checks=0,revert_checks=0;
 for(size_t seq=0;seq<inputs.size();++seq){
   std::vector<Decl> cpp_sorted;
   for(size_t prefix=0;prefix<=inputs[seq].size();++prefix){
     if(prefix>0)cpp_add(cpp_sorted,inputs[seq][prefix-1]);
     std::vector<Decl> prefix_input(inputs[seq].begin(),inputs[seq].begin()+prefix);
     Ptr built=ap(sym("build"),{list_term(prefix_input),sym("nil")});
     auto ty=infer({},built,sig,rs);assert(ty&&conv(*ty,L,rs));
     assert(conv(nf(built,rs),list_term(cpp_sorted),rs));++insertion_checks;
     for(unsigned bound=0;bound<=8;++bound){
       Ptr find=ap(sym("findRevertLayer"),{built,nat(bound)});
       auto find_ty=infer({},find,sig,rs);assert(find_ty&&conv(*find_ty,N,rs));
       assert(nat_value(nf(find,rs))==cpp_find_revert_layer(cpp_sorted,bound));++revert_checks;
     }
   }
 }
 std::cout<<"CascadeMap packed-key insertion agrees on "<<insertion_checks<<" prefix configurations\n";
 std::cout<<"FindRevertLayer first-eligible selection agrees on "<<revert_checks<<" finite (prefix,bound) cases\n";
 std::cout<<"Scope: origin/tree/style flags fixed; layer/rule/declaration/applied packed fields modeled; property collection and CSS application omitted.\n";
}
