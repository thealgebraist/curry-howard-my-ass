#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace lp;

enum class Difference : unsigned { Equal, NonInherited, PseudoElement, IndependentInherited, Inherited, DescendantAffecting };
enum class Propagate : unsigned { None, Pseudos, IndependentInherit, Children, Descendants };
enum class ApplyChanges : unsigned { No, Yes };
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
static Ptr prop_term(Propagate p){const char* n[]={"pNone","pPseudos","pIndependent","pChildren","pDescendants"};return sym(n[static_cast<unsigned>(p)]);}
static Ptr changes_term(ApplyChanges c){return sym(c==ApplyChanges::No?"applyNo":"applyYes");}
static Difference cpp_max(Difference a,Difference b){return static_cast<unsigned>(a)>static_cast<unsigned>(b)?a:b;}
static Difference cpp_compute(bool old_present,bool new_present,bool same_pointer,
                              bool cached_first_line,Difference first_line,
                              Difference regular){
  if(same_pointer)return Difference::Equal;
  if(!old_present||!new_present)return Difference::Inherited;
  return cpp_max(cached_first_line?first_line:Difference::Equal,regular);
}
static Propagate cpp_apply_style_diff(Propagate current,Difference diff){
  if(current==Propagate::Descendants||static_cast<unsigned>(diff)<static_cast<unsigned>(Difference::PseudoElement))return current;
  Propagate required=Propagate::Pseudos;
  if(diff==Difference::IndependentInherited)required=Propagate::IndependentInherit;
  if(diff==Difference::Inherited)required=Propagate::Children;
  if(diff==Difference::DescendantAffecting)required=Propagate::Descendants;
  return static_cast<unsigned>(current)>static_cast<unsigned>(required)?current:required;
}
static bool cpp_needs_reinsert(bool old_oof,bool new_oof,bool old_float,bool new_float){return old_oof!=new_oof||(!new_oof&&old_float!=new_float);}
static ApplyChanges cpp_apply_changes(bool diff_equal,bool html_root,bool anchor_evaluator,bool anchor_functions,bool first_letter_initial){
  ApplyChanges result=diff_equal?ApplyChanges::No:ApplyChanges::Yes;
  if(html_root)result=ApplyChanges::Yes;
  if(anchor_evaluator)result=ApplyChanges::No;
  else if(anchor_functions||first_letter_initial)result=ApplyChanges::Yes;
  return result;
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
  Ptr P=sym("Propagate");sig["Propagate"]=type_sort();for(const char* p:{"pNone","pPseudos","pIndependent","pChildren","pDescendants"})sig[p]=P;
  Ptr A=sym("ApplyChanges");sig["ApplyChanges"]=type_sort();sig["applyNo"]=A;sig["applyYes"]=A;
  Ptr ftype=D;for(int i=0;i<19;++i)ftype=fn(B,ftype);sig["classifyDifference"]=ftype;
  sig["computeDifference"]=fn(B,fn(B,fn(B,fn(B,fn(D,fn(D,D))))));
  sig["applyStyleDiff"]=fn(P,fn(D,P));
  sig["needsReinsert"]=fn(B,fn(B,fn(B,fn(B,B))));
  sig["layoutApplyChanges"]=fn(B,fn(B,fn(B,fn(B,fn(B,A)))));
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
    Difference output;
    if(same) output=Difference::Equal;
    else if(!oldp||!newp) output=Difference::Inherited;
    else {
      Difference first_component=cached?first:Difference::Equal;
      output=static_cast<unsigned>(first_component)>static_cast<unsigned>(regular)?first_component:regular;
    }
    std::vector<Ptr> args={bt(oldp),bt(newp),bt(same),bt(cached),diff_term(first),diff_term(regular)};
    rules.push_back(R(ap(sym("computeDifference"),args),diff_term(output)));
  }
  const Propagate propagations[]={Propagate::None,Propagate::Pseudos,Propagate::IndependentInherit,Propagate::Children,Propagate::Descendants};
  for(Propagate current:propagations)for(Difference diff:differences){
    Propagate output;
    if(current==Propagate::Descendants||static_cast<unsigned>(diff)<static_cast<unsigned>(Difference::PseudoElement)) output=current;
    else {
      Propagate required=diff==Difference::DescendantAffecting?Propagate::Descendants:
        diff==Difference::Inherited?Propagate::Children:
        diff==Difference::IndependentInherited?Propagate::IndependentInherit:Propagate::Pseudos;
      output=static_cast<unsigned>(current)>static_cast<unsigned>(required)?current:required;
    }
    rules.push_back(R(ap(sym("applyStyleDiff"),{prop_term(current),diff_term(diff)}),prop_term(output)));
  }
  for(bool old_oof:{false,true})for(bool new_oof:{false,true})for(bool old_float:{false,true})for(bool new_float:{false,true}){
    bool output=old_oof!=new_oof||(!new_oof&&old_float!=new_float);
    rules.push_back(R(ap(sym("needsReinsert"),{bt(old_oof),bt(new_oof),bt(old_float),bt(new_float)}),bt(output)));
  }
  for(bool equal:{false,true})for(bool html_root:{false,true})for(bool anchor_eval:{false,true})for(bool anchor_functions:{false,true})for(bool first_letter:{false,true}){
    ApplyChanges output=equal?ApplyChanges::No:ApplyChanges::Yes;
    if(html_root)output=ApplyChanges::Yes;
    if(anchor_eval)output=ApplyChanges::No;
    else if(anchor_functions||first_letter)output=ApplyChanges::Yes;
    rules.push_back(R(ap(sym("layoutApplyChanges"),{bt(equal),bt(html_root),bt(anchor_eval),bt(anchor_functions),bt(first_letter)}),changes_term(output)));
  }
  RuleIndex ix(rules);
  Ptr sample=ap(sym("classifyDifference"),std::vector<Ptr>(19,sym("false")));
  auto sample_type=infer({},sample,sig,rules);assert(sample_type&&conv(*sample_type,D,ix));
  Ptr outer_sample=ap(sym("computeDifference"),{sym("true"),sym("true"),sym("false"),sym("true"),diff_term(Difference::Equal),diff_term(Difference::Inherited)});
  auto outer_type=infer({},outer_sample,sig,rules);assert(outer_type&&conv(*outer_type,D,ix));
  Ptr apply_sample=ap(sym("applyStyleDiff"),{prop_term(Propagate::None),diff_term(Difference::Inherited)});
  auto apply_type=infer({},apply_sample,sig,rules);assert(apply_type&&conv(*apply_type,P,ix));
  Ptr reinsert_sample=ap(sym("needsReinsert"),{sym("false"),sym("false"),sym("false"),sym("false")});
  auto reinsert_type=infer({},reinsert_sample,sig,rules);assert(reinsert_type&&conv(*reinsert_type,B,ix));
  Ptr layout_sample=ap(sym("layoutApplyChanges"),{sym("true"),sym("false"),sym("false"),sym("false"),sym("false")});
  auto layout_type=infer({},layout_sample,sig,rules);assert(layout_type&&conv(*layout_type,A,ix));
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
  unsigned propagation_cases=0;
  for(Propagate current:propagations)for(Difference diff:differences){
    auto term=ap(sym("applyStyleDiff"),{prop_term(current),diff_term(diff)});
    if(!conv(nf(term,ix),prop_term(cpp_apply_style_diff(current,diff)),ix)){std::cerr<<"style diff propagation mismatch\n";return 4;}++propagation_cases;
  }
  std::cout<<"ComputeDifferenceIgnoringInheritedFirstLineStyle classifier agrees on "<<cases<<" abstract old/new style feature states.\n";
  std::cout<<"ComputeDifference pointer/null/cached-first-line max wrapper agrees on "<<outer_cases<<" finite states.\n";
  std::cout<<"ApplyComputedStyleDiff agrees on "<<propagation_cases<<" propagation/difference combinations.\n";
  unsigned reinsert_cases=0;
  for(bool old_oof:{false,true})for(bool new_oof:{false,true})for(bool old_float:{false,true})for(bool new_float:{false,true}){
    auto term=ap(sym("needsReinsert"),{bt(old_oof),bt(new_oof),bt(old_float),bt(new_float)});
    if(!conv(nf(term,ix),bt(cpp_needs_reinsert(old_oof,new_oof,old_float,new_float)),ix)){std::cerr<<"layout reinsert mismatch\n";return 5;}++reinsert_cases;
  }
  unsigned apply_changes_cases=0;
  for(bool equal:{false,true})for(bool html_root:{false,true})for(bool anchor_eval:{false,true})for(bool anchor_functions:{false,true})for(bool first_letter:{false,true}){
    auto term=ap(sym("layoutApplyChanges"),{bt(equal),bt(html_root),bt(anchor_eval),bt(anchor_functions),bt(first_letter)});
    if(!conv(nf(term,ix),changes_term(cpp_apply_changes(equal,html_root,anchor_eval,anchor_functions,first_letter)),ix)){std::cerr<<"layout apply-changes mismatch\n";return 6;}++apply_changes_cases;
  }
  std::cout<<"NeedsReinsertLayoutTree agrees on "<<reinsert_cases<<" position/float combinations; layout apply-change policy agrees on "<<apply_changes_cases<<" finite states.\n";
  std::cout<<"Scope: 19 classifier predicates are independent Boolean abstractions; generated field equality and cached first-line style lookup/computation remain opaque.\n";
}
