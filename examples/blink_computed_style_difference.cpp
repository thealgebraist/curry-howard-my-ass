#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace lp;

enum class Difference : unsigned { Equal, NonInherited, PseudoElement, IndependentInherited, Inherited, DescendantAffecting };
struct Features {
  bool scroll_animation_diff, display_changed, display_affects_descendants;
  bool marker_group_none_changed, old_marker_group_none, scroll_container_changed;
  bool non_independent_inherited_equal, justify_items_equal, decorations_equal;
  bool non_inherited_equal, child_has_explicit_inheritance, variables_independent;
  bool inherited_variables_equal, independent_inherited_equal;
  bool effective_overscroll_changed, has_any_pseudo, pseudo_styles_equal;
  bool marker_group_position_changed, display_list_item_transition;
};
static Difference cpp_difference(const Features& f) {
  if (f.scroll_animation_diff || (f.display_changed && f.display_affects_descendants) ||
      f.marker_group_none_changed || (!f.old_marker_group_none && f.scroll_container_changed))
    return Difference::DescendantAffecting;
  if (!f.non_independent_inherited_equal || !f.justify_items_equal || !f.decorations_equal)
    return Difference::Inherited;
  if (!f.non_inherited_equal && f.child_has_explicit_inheritance) return Difference::Inherited;
  if (!f.inherited_variables_equal && !f.variables_independent) return Difference::Inherited;
  if (!f.independent_inherited_equal || !f.inherited_variables_equal)
    return Difference::IndependentInherited;
  if (f.non_inherited_equal)
    return f.pseudo_styles_equal ? Difference::Equal : Difference::PseudoElement;
  if (f.effective_overscroll_changed) return Difference::DescendantAffecting;
  if (f.has_any_pseudo || f.marker_group_position_changed ||
      (f.display_changed && f.display_list_item_transition))
    return Difference::PseudoElement;
  return Difference::NonInherited;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr ap(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bt(bool b){return sym(b?"true":"false");}
static Ptr diff_term(Difference d){const char* n[]={"dEqual","dNonInherited","dPseudo","dIndependent","dInherited","dDescendant"};return sym(n[static_cast<unsigned>(d)]);}
static Difference cpp_max(Difference a,Difference b){return static_cast<unsigned>(a)>static_cast<unsigned>(b)?a:b;}
static Difference cpp_compute(bool old_present,bool new_present,bool same_pointer,
                              bool cached_first_line,Difference first_line,
                              Difference regular){
  if(same_pointer)return Difference::Equal;
  if(!old_present||!new_present)return Difference::Inherited;
  return cpp_max(cached_first_line?first_line:Difference::Equal,regular);
}
using Pattern=std::vector<int>; // -1 is wildcard; 0/1 are false/true.
static void add_pattern(std::vector<Rule>& rules,const Pattern& p,Ptr output){
  std::vector<Ptr> args;for(size_t i=0;i<p.size();++i)args.push_back(p[i]<0?pvar(i):sym(p[i]?"true":"false"));
  rules.push_back(R(ap(sym("classifyDifference"),args),std::move(output)));
}
static Pattern wild(){return Pattern(19,-1);}
int main(){
  Signature sig;std::vector<Rule> rules;Ptr B=sym("Bool"),D=sym("Difference");
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;sig["Difference"]=type_sort();
  for(const char* d:{"dEqual","dNonInherited","dPseudo","dIndependent","dInherited","dDescendant"})sig[d]=D;
  Ptr ftype=D;for(int i=0;i<19;++i)ftype=fn(B,ftype);sig["classifyDifference"]=ftype;
  sig["computeDifference"]=fn(B,fn(B,fn(B,fn(B,fn(D,fn(D,D))))));
  auto branch=[&](std::initializer_list<std::pair<size_t,bool>> constraints,Difference out){auto p=wild();for(auto [i,b]:constraints)p[i]=b?1:0;add_pattern(rules,p,diff_term(out));};
  // First-match rule order follows ComputeDifferenceIgnoringInheritedFirstLineStyle.
  branch({{0,true}},Difference::DescendantAffecting);
  branch({{1,true},{2,true}},Difference::DescendantAffecting);
  branch({{3,true}},Difference::DescendantAffecting);
  branch({{4,false},{5,true}},Difference::DescendantAffecting);
  branch({{6,false}},Difference::Inherited);branch({{7,false}},Difference::Inherited);branch({{8,false}},Difference::Inherited);
  branch({{9,false},{10,true}},Difference::Inherited);
  branch({{11,false},{12,false}},Difference::Inherited);
  branch({{13,false}},Difference::IndependentInherited);branch({{12,false}},Difference::IndependentInherited);
  branch({{9,true},{16,true}},Difference::Equal);branch({{9,true},{16,false}},Difference::PseudoElement);
  branch({{9,false},{10,false},{13,true},{14,true}},Difference::DescendantAffecting);
  branch({{9,false},{10,false},{13,true},{14,false},{15,true}},Difference::PseudoElement);
  branch({{9,false},{10,false},{13,true},{14,false},{15,false},{17,true}},Difference::PseudoElement);
  branch({{9,false},{10,false},{13,true},{14,false},{15,false},{17,false},{1,true},{18,true}},Difference::PseudoElement);
  add_pattern(rules,wild(),diff_term(Difference::NonInherited));
  const Difference differences[]={Difference::Equal,Difference::NonInherited,Difference::PseudoElement,Difference::IndependentInherited,Difference::Inherited,Difference::DescendantAffecting};
  // Exact wrapper branches: pointer identity, null styles, then max of cached
  // inherited first-line and ordinary style differences.
  for(bool oldp:{false,true})for(bool newp:{false,true})for(bool same:{false,true})for(bool cached:{false,true})for(Difference first:differences)for(Difference regular:differences){
    Difference output=cpp_compute(oldp,newp,same,cached,first,regular);
    std::vector<Ptr> args={bt(oldp),bt(newp),bt(same),bt(cached),diff_term(first),diff_term(regular)};
    rules.push_back(R(ap(sym("computeDifference"),args),diff_term(output)));
  }
  RuleIndex ix(rules);
  Ptr sample=ap(sym("classifyDifference"),std::vector<Ptr>(19,sym("false")));
  auto sample_type=infer({},sample,sig,rules);assert(sample_type&&conv(*sample_type,D,ix));
  Ptr outer_sample=ap(sym("computeDifference"),{sym("true"),sym("true"),sym("false"),sym("true"),diff_term(Difference::Equal),diff_term(Difference::Inherited)});
  auto outer_type=infer({},outer_sample,sig,rules);assert(outer_type&&conv(*outer_type,D,ix));
  unsigned cases=0;const unsigned limit=1u<<19;
  for(unsigned mask=0;mask<limit;++mask){
    bool b[19];std::vector<Ptr> args;for(unsigned i=0;i<19;++i){b[i]=(mask>>i)&1;args.push_back(bt(b[i]));}
    Features f{b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],b[10],b[11],b[12],b[13],b[14],b[15],b[16],b[17],b[18]};
    auto term=ap(sym("classifyDifference"),args);auto got=nf(term,ix);auto expected=diff_term(cpp_difference(f));
    if(!conv(got,expected,ix)){std::cerr<<"difference classifier mismatch at mask "<<mask<<"\n";return 2;}++cases;
  }
  unsigned outer_cases=0;
  for(bool oldp:{false,true})for(bool newp:{false,true})for(bool same:{false,true})for(bool cached:{false,true})for(Difference first:differences)for(Difference regular:differences){
    auto term=ap(sym("computeDifference"),{bt(oldp),bt(newp),bt(same),bt(cached),diff_term(first),diff_term(regular)});
    if(!conv(nf(nf(term,ix),ix),diff_term(cpp_compute(oldp,newp,same,cached,first,regular)),ix)){std::cerr<<"outer difference mismatch\n";return 3;}++outer_cases;
  }
  std::cout<<"ComputeDifferenceIgnoringInheritedFirstLineStyle classifier agrees on "<<cases<<" abstract old/new style feature states.\n";
  std::cout<<"ComputeDifference pointer/null/cached-first-line max wrapper agrees on "<<outer_cases<<" finite states.\n";
  std::cout<<"Scope: 19 classifier predicates are independent Boolean abstractions; generated field equality and cached first-line style lookup/computation remain opaque.\n";
}
