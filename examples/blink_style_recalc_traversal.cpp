#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
using namespace lp;
enum class Prop : unsigned { None, Pseudos, IndependentInherit, Children, Descendants };
enum class Kind { Element, Text };
enum class Dependency { None, Size, HighlightUnits, Style, ScrollState, Anchored, CachedHover };
struct Node { Kind kind; bool has_style; Dependency dependency; bool dirty; };
enum Flag : unsigned { Size=1, Style=2, Scroll=4, Anchor=8, ContentVisibility=16, Hover=32 };
static bool cpp_any_flags(unsigned f){return f!=0;}
static bool cpp_flag_dependent(unsigned f,const Node& n){
  if(!cpp_any_flags(f)||n.kind!=Kind::Element)return false;
  if(!n.has_style)return (f&(Size|Style|Scroll|Anchor))!=0;
  return ((f&Size)&&(n.dependency==Dependency::Size||n.dependency==Dependency::HighlightUnits))||
    ((f&Style)&&n.dependency==Dependency::Style)||((f&Scroll)&&n.dependency==Dependency::ScrollState)||
    ((f&Anchor)&&n.dependency==Dependency::Anchored)||((f&Hover)&&n.dependency==Dependency::CachedHover);
}
static bool cpp_should(Prop p,unsigned f,bool suppress,const Node& n){
  if(suppress)return false;if(p>=Prop::IndependentInherit)return true;if(n.dirty)return true;return cpp_flag_dependent(f,n);
}
static bool cpp_mark_reattach(bool mark,bool reattach,bool suppress){return mark&&reattach&&!suppress;}
static bool cpp_traverse_children(Prop p,unsigned f,bool child_dirty){return p>=Prop::IndependentInherit||cpp_any_flags(f)||child_dirty;}
static bool cpp_traverse_pseudo(Prop p,unsigned f,bool child_dirty){return p!=Prop::None||cpp_any_flags(f)||child_dirty;}
static bool cpp_traverse_child(bool should,bool mark,bool child_dirty,bool force_reattach,bool any_flags,bool layout_subtree){return should||mark||child_dirty||force_reattach||any_flags||layout_subtree;}
static bool cpp_should_update_pseudo(Prop p,unsigned f,bool dirty,bool child_dirty,bool layout_dirty,Dependency d){
  if(p!=Prop::None||dirty||child_dirty||layout_dirty)return true;
  return cpp_any_flags(f)&&(((f&Size)&&d==Dependency::Size)||((f&Style)&&d==Dependency::Style)||
    ((f&Scroll)&&d==Dependency::ScrollState)||((f&Anchor)&&d==Dependency::Anchored));
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr ap(Ptr f,std::initializer_list<Ptr> xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bt(bool x){return sym(x?"true":"false");}
static Ptr prop_term(Prop p){const char* n[]={"pNone","pPseudos","pIndependent","pChildren","pDescendants"};return sym(n[static_cast<unsigned>(p)]);}
static Ptr kind_term(Kind k){return sym(k==Kind::Element?"element":"textNode");}
static Ptr dep_term(Dependency d){const char* n[]={"depNone","depSize","depHighlight","depStyle","depScroll","depAnchor","depHover"};return sym(n[static_cast<unsigned>(d)]);}
static Ptr fterm(unsigned f){return sym("f"+std::to_string(f));}
int main(){
  Signature sig;std::vector<Rule> rules;auto v=[](size_t i){return pvar(i);};
  Ptr B=sym("Bool"),P=sym("Prop"),F=sym("FlagMask"),D=sym("Dependency"),K=sym("NodeKind");
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["Prop"]=type_sort();for(const char* n:{"pNone","pPseudos","pIndependent","pChildren","pDescendants"})sig[n]=P;
  sig["FlagMask"]=type_sort();for(unsigned i=0;i<64;++i)sig["f"+std::to_string(i)]=F;
  sig["Dependency"]=type_sort();for(const char* n:{"depNone","depSize","depHighlight","depStyle","depScroll","depAnchor","depHover"})sig[n]=D;
  sig["NodeKind"]=type_sort();sig["element"]=K;sig["textNode"]=K;
  sig["notB"]=fn(B,B);sig["andB"]=fn(B,fn(B,B));sig["orB"]=fn(B,fn(B,B));sig["or3"]=fn(B,fn(B,fn(B,B)));
  for(const char* n:{"flagSize","flagStyle","flagScroll","flagAnchor","flagContent","flagHover"})sig[n]=fn(F,B);
  for(const char* n:{"depSize","depHighlight","depStyle","depScroll","depAnchor","depHover"})sig[n]=fn(D,B);
  sig["anyFlags"]=fn(F,B);sig["containerFlags"]=fn(F,B);sig["depHit"]=fn(F,fn(D,B));sig["styleDependent"]=fn(B,fn(F,fn(D,B)));
  sig["isElement"]=fn(K,B);sig["flagDependent"]=fn(F,fn(K,fn(B,fn(D,B))));sig["forcesNode"]=fn(P,B);
  sig["shouldRecalc"]=fn(P,fn(F,fn(B,fn(K,fn(B,fn(D,fn(B,B)))))));
  sig["markReattach"]=fn(B,fn(B,fn(B,B)));sig["recalcChildren"]=fn(P,B);sig["updatePseudos"]=fn(P,B);
  sig["traverseChildren"]=fn(P,fn(F,fn(B,B)));sig["traversePseudo"]=fn(P,fn(F,fn(B,B)));
  sig["pseudoDependencyHit"]=fn(F,fn(D,B));sig["shouldUpdatePseudo"]=fn(P,fn(F,fn(B,fn(B,fn(B,fn(D,B))))));
  sig["traverseChild"]=fn(B,fn(B,fn(B,fn(B,fn(B,fn(B,B))))));
  rules.push_back(R(app(sym("notB"),sym("true")),sym("false")));rules.push_back(R(app(sym("notB"),sym("false")),sym("true")));
  for(const char* x:{"true","false"})for(const char* y:{"true","false"}){
    rules.push_back(R(ap(sym("andB"),{sym(x),sym(y)}),bt(std::string(x)=="true"&&std::string(y)=="true")));
    rules.push_back(R(ap(sym("orB"),{sym(x),sym(y)}),bt(std::string(x)=="true"||std::string(y)=="true")));
  }
  for(const char* x:{"true","false"})for(const char* y:{"true","false"})for(const char* z:{"true","false"})rules.push_back(R(ap(sym("or3"),{sym(x),sym(y),sym(z)}),bt(std::string(x)=="true"||std::string(y)=="true"||std::string(z)=="true")));
  for(unsigned f=0;f<64;++f){auto bitset=[&](unsigned mask){return (f&mask)!=0;};
    rules.push_back(R(app(sym("flagSize"),fterm(f)),bt(bitset(Size))));rules.push_back(R(app(sym("flagStyle"),fterm(f)),bt(bitset(Style))));rules.push_back(R(app(sym("flagScroll"),fterm(f)),bt(bitset(Scroll))));rules.push_back(R(app(sym("flagAnchor"),fterm(f)),bt(bitset(Anchor))));rules.push_back(R(app(sym("flagContent"),fterm(f)),bt(bitset(ContentVisibility))));rules.push_back(R(app(sym("flagHover"),fterm(f)),bt(bitset(Hover))));
  }
  for(Dependency d:{Dependency::None,Dependency::Size,Dependency::HighlightUnits,Dependency::Style,Dependency::ScrollState,Dependency::Anchored,Dependency::CachedHover}){
    const char* names[]={"depSize","depHighlight","depStyle","depScroll","depAnchor","depHover"};const Dependency all[]={Dependency::Size,Dependency::HighlightUnits,Dependency::Style,Dependency::ScrollState,Dependency::Anchored,Dependency::CachedHover};
    for(size_t i=0;i<6;++i)rules.push_back(R(app(sym(names[i]),dep_term(d)),bt(d==all[i])));
  }
  Ptr f0=v(0),d1=v(1);
  rules.push_back(R(app(sym("anyFlags"),f0),ap(sym("or3"),{ap(sym("or3"),{app(sym("flagSize"),f0),app(sym("flagStyle"),f0),app(sym("flagScroll"),f0)}),ap(sym("orB"),{app(sym("flagAnchor"),f0),app(sym("flagContent"),f0)}),app(sym("flagHover"),f0)})));
  rules.push_back(R(app(sym("containerFlags"),f0),ap(sym("or3"),{ap(sym("orB"),{app(sym("flagSize"),f0),app(sym("flagStyle"),f0)}),app(sym("flagScroll"),f0),app(sym("flagAnchor"),f0)})));
  Ptr sz=ap(sym("andB"),{app(sym("flagSize"),f0),ap(sym("orB"),{app(sym("depSize"),d1),app(sym("depHighlight"),d1)})});
  Ptr st=ap(sym("andB"),{app(sym("flagStyle"),f0),app(sym("depStyle"),d1)});
  Ptr sc=ap(sym("andB"),{app(sym("flagScroll"),f0),app(sym("depScroll"),d1)});
  Ptr an=ap(sym("andB"),{app(sym("flagAnchor"),f0),app(sym("depAnchor"),d1)});
  Ptr hv=ap(sym("andB"),{app(sym("flagHover"),f0),app(sym("depHover"),d1)});
  rules.push_back(R(ap(sym("depHit"),{f0,d1}),ap(sym("or3"),{ap(sym("orB"),{sz,st}),ap(sym("orB"),{sc,an}),hv})));
  rules.push_back(R(ap(sym("styleDependent"),{sym("true"),f0,d1}),ap(sym("depHit"),{f0,d1})));
  rules.push_back(R(ap(sym("styleDependent"),{sym("false"),f0,d1}),app(sym("containerFlags"),f0)));
  rules.push_back(R(app(sym("isElement"),sym("element")),sym("true")));rules.push_back(R(app(sym("isElement"),sym("textNode")),sym("false")));
  rules.push_back(R(ap(sym("flagDependent"),{v(0),v(1),v(2),v(3)}),ap(sym("andB"),{app(sym("isElement"),v(1)),ap(sym("andB"),{app(sym("anyFlags"),v(0)),ap(sym("styleDependent"),{v(2),v(0),v(3)})})})));
  for(const char* n:{"pNone","pPseudos"})rules.push_back(R(app(sym("forcesNode"),sym(n)),sym("false")));
  for(const char* n:{"pIndependent","pChildren","pDescendants"})rules.push_back(R(app(sym("forcesNode"),sym(n)),sym("true")));
  rules.push_back(R(ap(sym("shouldRecalc"),{v(0),v(1),v(2),v(3),v(4),v(5),v(6)}),ap(sym("andB"),{app(sym("notB"),v(2)),ap(sym("or3"),{app(sym("forcesNode"),v(0)),v(6),ap(sym("flagDependent"),{v(1),v(3),v(4),v(5)})})})));
  rules.push_back(R(ap(sym("markReattach"),{v(0),v(1),v(2)}),ap(sym("andB"),{v(0),ap(sym("andB"),{v(1),app(sym("notB"),v(2))})})));
  for(const char* n:{"pNone","pPseudos"})rules.push_back(R(app(sym("recalcChildren"),sym(n)),sym("false")));
  for(const char* n:{"pIndependent","pChildren","pDescendants"})rules.push_back(R(app(sym("recalcChildren"),sym(n)),sym("true")));
  rules.push_back(R(app(sym("updatePseudos"),sym("pNone")),sym("false")));
  for(const char* n:{"pPseudos","pIndependent","pChildren","pDescendants"})rules.push_back(R(app(sym("updatePseudos"),sym(n)),sym("true")));
  rules.push_back(R(ap(sym("traverseChildren"),{v(0),v(1),v(2)}),ap(sym("or3"),{app(sym("recalcChildren"),v(0)),app(sym("anyFlags"),v(1)),v(2)})));
  rules.push_back(R(ap(sym("traversePseudo"),{v(0),v(1),v(2)}),ap(sym("or3"),{app(sym("updatePseudos"),v(0)),app(sym("anyFlags"),v(1)),v(2)})));
  Ptr psize=ap(sym("andB"),{app(sym("flagSize"),f0),app(sym("depSize"),d1)});
  Ptr pstyle=ap(sym("andB"),{app(sym("flagStyle"),f0),app(sym("depStyle"),d1)});
  Ptr pscroll=ap(sym("andB"),{app(sym("flagScroll"),f0),app(sym("depScroll"),d1)});
  Ptr panchor=ap(sym("andB"),{app(sym("flagAnchor"),f0),app(sym("depAnchor"),d1)});
  rules.push_back(R(ap(sym("pseudoDependencyHit"),{f0,d1}),ap(sym("orB"),{ap(sym("orB"),{psize,pstyle}),ap(sym("orB"),{pscroll,panchor})})));
  rules.push_back(R(ap(sym("shouldUpdatePseudo"),{v(0),v(1),v(2),v(3),v(4),v(5)}),
    ap(sym("orB"),{ap(sym("or3"),{app(sym("updatePseudos"),v(0)),v(2),v(3)}),
      ap(sym("orB"),{v(4),ap(sym("andB"),{app(sym("anyFlags"),v(1)),ap(sym("pseudoDependencyHit"),{v(1),v(5)})})})})));
  rules.push_back(R(ap(sym("traverseChild"),{v(0),v(1),v(2),v(3),v(4),v(5)}),ap(sym("orB"),{ap(sym("or3"),{v(0),v(1),v(2)}),ap(sym("or3"),{v(3),v(4),v(5)})})));
  RuleIndex ix(rules);auto elem=sym("element"),txt=sym("textNode");
  Ptr type_examples[]={
    ap(sym("shouldRecalc"),{sym("pNone"),fterm(0),sym("false"),elem,sym("true"),sym("depNone"),sym("false")}),
    ap(sym("markReattach"),{sym("true"),sym("true"),sym("false")}),
    ap(sym("traverseChildren"),{sym("pNone"),fterm(0),sym("false")}),
    ap(sym("traversePseudo"),{sym("pNone"),fterm(0),sym("false")}),
    ap(sym("shouldUpdatePseudo"),{sym("pNone"),fterm(0),sym("false"),sym("false"),sym("false"),sym("depNone")}),
    ap(sym("traverseChild"),{sym("true"),sym("false"),sym("false"),sym("false"),sym("false"),sym("false")})};
  for(Ptr t:type_examples){auto ty=infer({},t,sig,rules);assert(ty&&conv(*ty,B,ix));}
  const Dependency deps[]={Dependency::None,Dependency::Size,Dependency::HighlightUnits,Dependency::Style,Dependency::ScrollState,Dependency::Anchored,Dependency::CachedHover};
  unsigned children_cases=0,pseudo_cases=0,pseudo_update_cases=0,mark_cases=0,child_formula_cases=0,child_composition_cases=0;
  for(unsigned mask=0;mask<64;++mask){bool vals[6];for(unsigned i=0;i<6;++i)vals[i]=(mask&(1u<<i))!=0;Ptr t=ap(sym("traverseChild"),{bt(vals[0]),bt(vals[1]),bt(vals[2]),bt(vals[3]),bt(vals[4]),bt(vals[5])});if(!conv(nf(t,ix),bt(cpp_traverse_child(vals[0],vals[1],vals[2],vals[3],vals[4],vals[5])),ix)){std::cerr<<"formula mismatch mask="<<mask<<" actual="<<to_string(nf(t,ix))<<" expected="<<to_string(bt(cpp_traverse_child(vals[0],vals[1],vals[2],vals[3],vals[4],vals[5])))<<"\n";return 7;}++child_formula_cases;}
  for(unsigned f=0;f<64;++f)for(Prop p:{Prop::None,Prop::Pseudos,Prop::IndependentInherit,Prop::Children,Prop::Descendants})for(bool dirty:{false,true}){
    Ptr tc=ap(sym("traverseChildren"),{prop_term(p),fterm(f),bt(dirty)});Ptr tp=ap(sym("traversePseudo"),{prop_term(p),fterm(f),bt(dirty)});
    assert(conv(nf(tc,ix),bt(cpp_traverse_children(p,f,dirty)),ix));assert(conv(nf(tp,ix),bt(cpp_traverse_pseudo(p,f,dirty)),ix));++children_cases;++pseudo_cases;
  }
  for(unsigned f=0;f<64;++f)for(Prop p:{Prop::None,Prop::Pseudos,Prop::IndependentInherit,Prop::Children,Prop::Descendants})for(bool dirty:{false,true})for(bool child_dirty:{false,true})for(bool layout_dirty:{false,true})for(Dependency d:deps){
    bool expected=cpp_should_update_pseudo(p,f,dirty,child_dirty,layout_dirty,d);
    Ptr t=ap(sym("shouldUpdatePseudo"),{prop_term(p),fterm(f),bt(dirty),bt(child_dirty),bt(layout_dirty),dep_term(d)});
    assert(conv(nf(t,ix),bt(expected),ix));++pseudo_update_cases;
  }
  for(bool mark:{false,true})for(bool reattach:{false,true})for(bool suppress:{false,true}){Ptr t=ap(sym("markReattach"),{bt(mark),bt(reattach),bt(suppress)});assert(conv(nf(t,ix),bt(cpp_mark_reattach(mark,reattach,suppress)),ix));++mark_cases;}
  for(unsigned i=0;i<320;++i){
    unsigned f=i/5;Prop p=static_cast<Prop>(i%5);bool suppress=i&1;Kind k=(i&2)?Kind::Text:Kind::Element;bool has_style=i&4;Dependency d=deps[(i/8)%7];bool dirty=i&16;
    bool mark=i&32,reattach=i&64,child_dirty=i&128,force=i&256,layout=i&8;
    Node n{k,has_style,d,dirty};bool should=cpp_should(p,f,suppress,n);bool marked=cpp_mark_reattach(mark,reattach,suppress);bool expected=cpp_traverse_child(should,marked,child_dirty,force,cpp_any_flags(f),layout);
    Ptr t=ap(sym("traverseChild"),{ap(sym("shouldRecalc"),{prop_term(p),fterm(f),bt(suppress),kind_term(k),bt(has_style),dep_term(d),bt(dirty)}),ap(sym("markReattach"),{bt(mark),bt(reattach),bt(suppress)}),bt(child_dirty),bt(force),app(sym("anyFlags"),fterm(f)),bt(layout)});
    assert(conv(nf(t,ix),bt(expected),ix));++child_composition_cases;
  }
  std::cout<<"TraverseChildren and TraversePseudoElements agree on "<<children_cases<<" TraverseChildren and "<<pseudo_cases<<" TraversePseudoElements states\n";
  std::cout<<"ShouldUpdatePseudoElement agrees on "<<pseudo_update_cases<<" finite flag/dependency/dirty states\n";
  std::cout<<"MarkReattachLayoutTree agrees on "<<mark_cases<<" flag combinations; TraverseChild OR agrees on "<<child_formula_cases<<" combinations\n";
  std::cout<<"TraverseChild composition agrees on "<<child_composition_cases<<" representative node/change states\n";
  std::cout<<"Scope: grouped query-family flags; no Element recursion, style-engine scheduling, or DOM tree traversal.\n";
}
