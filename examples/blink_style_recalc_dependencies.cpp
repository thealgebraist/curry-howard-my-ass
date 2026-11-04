#include "../src/lambda_pi.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

// Semantic projection of StyleRecalcChange's per-node query-dependent branch.
using namespace lp;
enum class Prop : unsigned { None, Pseudos, IndependentInherit, Children, Descendants };
enum class NodeKind { Element, Text };
enum class Dependency { None, Size, HighlightUnits, Style, ScrollState, Anchored, CachedHover };
struct Node { NodeKind kind; bool has_old_style; Dependency dependency; bool dirty; };

// Grouped bits correspond to the categories tested by RecalcFlagDependent.
// Child/descendant variants in Blink are ORed into the same family predicate.
enum Flag : unsigned { Size=1, Style=2, Scroll=4, Anchor=8, ContentVisibility=16, Hover=32 };
static bool has_container_flag(unsigned f){return (f&(Size|Style|Scroll|Anchor))!=0;}
static bool has_recalc_flag(unsigned f){return (f&(Size|Style|Scroll|Anchor|ContentVisibility|Hover))!=0;}
static bool cpp_flag_dependent(unsigned f,const Node& n){
  if(!has_recalc_flag(f)||n.kind!=NodeKind::Element)return false;
  if(!n.has_old_style)return has_container_flag(f);
  return ((f&Size)&&(n.dependency==Dependency::Size||n.dependency==Dependency::HighlightUnits))||
         ((f&Style)&&n.dependency==Dependency::Style)||
         ((f&Scroll)&&n.dependency==Dependency::ScrollState)||
         ((f&Anchor)&&n.dependency==Dependency::Anchored)||
         ((f&Hover)&&n.dependency==Dependency::CachedHover);
}
static bool cpp_should(Prop p,unsigned f,bool suppress,const Node& n){
  if(suppress)return false;
  if(p>=Prop::IndependentInherit)return true;
  if(n.dirty)return true;
  return cpp_flag_dependent(f,n);
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr ap(Ptr f,std::initializer_list<Ptr> xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr b(bool x){return sym(x?"true":"false");}
static Ptr pterm(Prop p){const char* names[]={"pNone","pPseudos","pIndependent","pChildren","pDescendants"};return sym(names[static_cast<unsigned>(p)]);}
static Ptr dterm(Dependency d){const char* names[]={"depNone","depSize","depHighlight","depStyle","depScroll","depAnchor","depHover"};return sym(names[static_cast<unsigned>(d)]);}
static Ptr fterm(unsigned f){return sym("f"+std::to_string(f));}

int main(){
  Signature sig;std::vector<Rule> rs;auto v=[](size_t i){return pvar(i);};
  Ptr B=sym("Bool"),P=sym("Prop"),F=sym("FlagMask"),D=sym("Dependency"),K=sym("NodeKind");
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;
  sig["Prop"]=type_sort();for(const char* x:{"pNone","pPseudos","pIndependent","pChildren","pDescendants"})sig[x]=P;
  sig["FlagMask"]=type_sort();for(unsigned i=0;i<64;++i)sig["f"+std::to_string(i)]=F;
  sig["Dependency"]=type_sort();for(const char* x:{"depNone","depSize","depHighlight","depStyle","depScroll","depAnchor","depHover"})sig[x]=D;
  sig["NodeKind"]=type_sort();sig["element"]=K;sig["textNode"]=K;
  sig["notB"]=fn(B,B);sig["andB"]=fn(B,fn(B,B));sig["orB"]=fn(B,fn(B,B));sig["or3"]=fn(B,fn(B,fn(B,B)));
  for(const char* x:{"flagSize","flagStyle","flagScroll","flagAnchor","flagContent","flagHover"})sig[x]=fn(F,B);
  for(const char* x:{"depSize","depHighlight","depStyle","depScroll","depAnchor","depHover"})sig[x]=fn(D,B);
  sig["anyContainerFlags"]=fn(F,B);sig["anyRecalcFlags"]=fn(F,B);sig["dependencyHit"]=fn(F,fn(D,B));sig["styleDependent"]=fn(B,fn(F,fn(D,B)));
  sig["isElement"]=fn(K,B);sig["flagDependent"]=fn(F,fn(K,fn(B,fn(D,B))));
  sig["forces"]=fn(P,B);sig["shouldRecalc"]=fn(P,fn(F,fn(B,fn(K,fn(B,fn(D,fn(B,B)))))));
  rs.push_back(R(app(sym("notB"),sym("true")),sym("false")));rs.push_back(R(app(sym("notB"),sym("false")),sym("true")));
  for(const char* x:{"true","false"})for(const char* y:{"true","false"}){
    rs.push_back(R(ap(sym("andB"),{sym(x),sym(y)}),b(std::string(x)=="true"&&std::string(y)=="true")));
    rs.push_back(R(ap(sym("orB"),{sym(x),sym(y)}),b(std::string(x)=="true"||std::string(y)=="true")));
  }
  for(const char* x:{"true","false"})for(const char* y:{"true","false"})for(const char* z:{"true","false"})
    rs.push_back(R(ap(sym("or3"),{sym(x),sym(y),sym(z)}),b(std::string(x)=="true"||std::string(y)=="true"||std::string(z)=="true")));
  for(unsigned f=0;f<64;++f){
    auto bit=[&](unsigned mask){return (f&mask)!=0;};
    rs.push_back(R(app(sym("flagSize"),fterm(f)),b(bit(Size))));rs.push_back(R(app(sym("flagStyle"),fterm(f)),b(bit(Style))));
    rs.push_back(R(app(sym("flagScroll"),fterm(f)),b(bit(Scroll))));rs.push_back(R(app(sym("flagAnchor"),fterm(f)),b(bit(Anchor))));
    rs.push_back(R(app(sym("flagContent"),fterm(f)),b(bit(ContentVisibility))));rs.push_back(R(app(sym("flagHover"),fterm(f)),b(bit(Hover))));
  }
  for(Dependency d:{Dependency::None,Dependency::Size,Dependency::HighlightUnits,Dependency::Style,Dependency::ScrollState,Dependency::Anchored,Dependency::CachedHover}){
    const char* key=d==Dependency::Size?"depSize":d==Dependency::HighlightUnits?"depHighlight":d==Dependency::Style?"depStyle":d==Dependency::ScrollState?"depScroll":d==Dependency::Anchored?"depAnchor":d==Dependency::CachedHover?"depHover":nullptr;
    for(const char* name:{"depSize","depHighlight","depStyle","depScroll","depAnchor","depHover"})rs.push_back(R(app(sym(name),dterm(d)),b(key&&std::string(key)==name)));
  }
  rs.push_back(R(app(sym("anyContainerFlags"),v(0)),ap(sym("or3"),{ap(sym("orB"),{app(sym("flagSize"),v(0)),app(sym("flagStyle"),v(0))}),app(sym("flagScroll"),v(0)),app(sym("flagAnchor"),v(0))})));
  rs.push_back(R(app(sym("anyRecalcFlags"),v(0)),ap(sym("or3"),{ap(sym("or3"),{app(sym("flagSize"),v(0)),app(sym("flagStyle"),v(0)),app(sym("flagScroll"),v(0))}),ap(sym("orB"),{app(sym("flagAnchor"),v(0)),app(sym("flagContent"),v(0))}),app(sym("flagHover"),v(0))})));
  Ptr size_hit=ap(sym("andB"),{app(sym("flagSize"),v(0)),ap(sym("orB"),{app(sym("depSize"),v(1)),app(sym("depHighlight"),v(1))})});
  Ptr style_hit=ap(sym("andB"),{app(sym("flagStyle"),v(0)),app(sym("depStyle"),v(1))});
  Ptr scroll_hit=ap(sym("andB"),{app(sym("flagScroll"),v(0)),app(sym("depScroll"),v(1))});
  Ptr anchor_hit=ap(sym("andB"),{app(sym("flagAnchor"),v(0)),app(sym("depAnchor"),v(1))});
  Ptr hover_hit=ap(sym("andB"),{app(sym("flagHover"),v(0)),app(sym("depHover"),v(1))});
  rs.push_back(R(ap(sym("dependencyHit"),{v(0),v(1)}),ap(sym("or3"),{ap(sym("orB"),{size_hit,style_hit}),ap(sym("orB"),{scroll_hit,anchor_hit}),hover_hit})));
  rs.push_back(R(ap(sym("styleDependent"),{sym("true"),v(0),v(1)}),ap(sym("dependencyHit"),{v(0),v(1)})));
  rs.push_back(R(ap(sym("styleDependent"),{sym("false"),v(0),v(1)}),app(sym("anyContainerFlags"),v(0))));
  rs.push_back(R(app(sym("isElement"),sym("element")),sym("true")));rs.push_back(R(app(sym("isElement"),sym("textNode")),sym("false")));
  rs.push_back(R(ap(sym("flagDependent"),{v(0),v(1),v(2),v(3)}),ap(sym("andB"),{app(sym("isElement"),v(1)),ap(sym("andB"),{app(sym("anyRecalcFlags"),v(0)),ap(sym("styleDependent"),{v(2),v(0),v(3)})})})));
  for(const char* p:{"pNone","pPseudos"})rs.push_back(R(app(sym("forces"),sym(p)),sym("false")));
  for(const char* p:{"pIndependent","pChildren","pDescendants"})rs.push_back(R(app(sym("forces"),sym(p)),sym("true")));
  rs.push_back(R(ap(sym("shouldRecalc"),{v(0),v(1),v(2),v(3),v(4),v(5),v(6)}),ap(sym("andB"),{app(sym("notB"),v(2)),ap(sym("or3"),{app(sym("forces"),v(0)),v(6),ap(sym("flagDependent"),{v(1),v(3),v(4),v(5)})})})));
  RuleIndex ix(rs);
  auto flags_term=[](unsigned f){return fterm(f);};
  auto elem=sym("element"),txt=sym("textNode");
  unsigned dependent_cases=0,gate_cases=0;
  const Dependency deps[]={Dependency::None,Dependency::Size,Dependency::HighlightUnits,Dependency::Style,Dependency::ScrollState,Dependency::Anchored,Dependency::CachedHover};
  // Exhaust every grouped flag mask against every old-style dependency state.
  for(unsigned f=0;f<64;++f)for(NodeKind k:{NodeKind::Element,NodeKind::Text})for(bool has_style:{false,true})for(Dependency dep:deps){
    Node node{k,has_style,dep,false};
    Ptr t=ap(sym("flagDependent"),{flags_term(f),k==NodeKind::Element?elem:txt,b(has_style),dterm(dep)});
    assert(conv(nf(t,ix),b(cpp_flag_dependent(f,node)),ix));++dependent_cases;
  }
  // Exhaust the complete decision product over propagation, suppression, dirty state,
  // grouped flag masks, node kind, old-style presence, and dependency category.
  for(unsigned f=0;f<64;++f)for(Prop prop:{Prop::None,Prop::Pseudos,Prop::IndependentInherit,Prop::Children,Prop::Descendants})for(bool suppress:{false,true})for(NodeKind k:{NodeKind::Element,NodeKind::Text})for(bool has_style:{false,true})for(Dependency dep:deps)for(bool dirty:{false,true}){
    Node node{k,has_style,dep,dirty};const bool expected=cpp_should(prop,f,suppress,node);
    Ptr t=ap(sym("shouldRecalc"),{pterm(prop),flags_term(f),b(suppress),k==NodeKind::Element?elem:txt,b(has_style),dterm(dep),b(dirty)});
    assert(conv(nf(t,ix),b(expected),ix));++gate_cases;
  }
  std::cout<<"StyleRecalcChange query dependency predicate agrees on "<<dependent_cases<<" finite flag/style/dependency states\n";
  std::cout<<"ShouldRecalcStyleFor agrees on "<<gate_cases<<" grouped propagation/suppression/dependency states\n";
  std::cout<<"Scope: child/descendant bits are grouped by query family; no full Element traversal, pseudo-element predicate, FlagsForChildren, or layout reattachment behavior.\n";
}
