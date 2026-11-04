#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>

// Finite projection of StyleRecalcChange::ShouldRecalcStyleFor and
// ForChildren. It includes suppression, forced child/descendant propagation,
// node dirtiness, and one style-container-query-dependent element category.
using namespace lp;
enum class Prop { None, Pseudos, IndependentInherit, Children, Descendants };
enum class NodeKind { Element, Text };
enum class FlagSet { None, Suppress, StyleChildren, SuppressStyleChildren };
struct Node { NodeKind kind; bool needs_style; bool depends_style_query; };

static bool cpp_should(Prop prop, FlagSet flags, Node node) {
  if (flags==FlagSet::Suppress || flags==FlagSet::SuppressStyleChildren) return false;
  if (prop==Prop::Children || prop==Prop::Descendants) return true;
  if (node.needs_style) return true;
  const bool style_query_flag=flags==FlagSet::StyleChildren;
  return style_query_flag && node.kind==NodeKind::Element && node.depends_style_query;
}
struct ChildChange { Prop prop; FlagSet flags; };
static ChildChange cpp_for_children(Prop prop, FlagSet) {
  // ForChildren preserves descendant propagation; other levels become kNo.
  // In this modeled flag subset, FlagsForChildren removes both the
  // child-only style-container flag and root-only suppression.
  return {prop==Prop::Descendants?Prop::Descendants:Prop::None,FlagSet::None};
}

static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr ap(Ptr f,std::initializer_list<Ptr> xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr prop_term(Prop p){const char* n[]={"pNone","pPseudos","pIndependent","pChildren","pDescendants"};return sym(n[static_cast<unsigned>(p)]);}
static Ptr node_term(NodeKind n){return sym(n==NodeKind::Element?"element":"textNode");}
static Ptr flag_term(FlagSet f){const char* n[]={"noFlags","suppress","styleChildren","suppressStyleChildren"};return sym(n[static_cast<unsigned>(f)]);}
static Ptr bool_term(bool b){return sym(b?"true":"false");}

int main(){
  Signature sig;std::vector<Rule> rules;auto p=[](size_t i){return pvar(i);};
  auto boolean=sym("Bool"),prop=sym("Prop"),node=sym("NodeKind"),flags=sym("Flags"),child=sym("ChildChange");
  sig["Bool"]=type_sort();sig["true"]=boolean;sig["false"]=boolean;
  sig["Prop"]=type_sort();sig["pNone"]=prop;sig["pPseudos"]=prop;sig["pIndependent"]=prop;sig["pChildren"]=prop;sig["pDescendants"]=prop;
  sig["NodeKind"]=type_sort();sig["element"]=node;sig["textNode"]=node;
  sig["Flags"]=type_sort();sig["noFlags"]=flags;sig["suppress"]=flags;sig["styleChildren"]=flags;sig["suppressStyleChildren"]=flags;
  sig["notB"]=fn(boolean,boolean);sig["andB"]=fn(boolean,fn(boolean,boolean));sig["orB"]=fn(boolean,fn(boolean,boolean));sig["or3"]=fn(boolean,fn(boolean,fn(boolean,boolean)));
  sig["isSuppressed"]=fn(flags,boolean);sig["forces"]=fn(prop,boolean);
  sig["flagDependent"]=fn(flags,fn(node,fn(boolean,boolean)));
  sig["shouldRecalc"]=fn(prop,fn(flags,fn(node,fn(boolean,fn(boolean,boolean)))));
  sig["childProp"]=fn(prop,prop);sig["childFlags"]=fn(flags,flags);
  sig["ChildChange"]=type_sort();sig["childChange"]=fn(prop,fn(flags,child));
  sig["forChildren"]=fn(prop,fn(flags,child));

  rules.push_back(rule(app(sym("notB"),sym("true")),sym("false")));
  rules.push_back(rule(app(sym("notB"),sym("false")),sym("true")));
  for(const char* a:{"true","false"})for(const char* b:{"true","false"}){
    bool result=std::string(a)=="true"&&std::string(b)=="true";
    rules.push_back(rule(ap(sym("andB"),{sym(a),sym(b)}),bool_term(result)));
  }
  for(const char* a:{"true","false"})for(const char* b:{"true","false"}){
    bool result=std::string(a)=="true"||std::string(b)=="true";
    rules.push_back(rule(ap(sym("orB"),{sym(a),sym(b)}),bool_term(result)));
  }
  for(const char* a:{"true","false"})for(const char* b:{"true","false"})for(const char* c:{"true","false"}){
    bool result=std::string(a)=="true"||std::string(b)=="true"||std::string(c)=="true";
    rules.push_back(rule(ap(sym("or3"),{sym(a),sym(b),sym(c)}),bool_term(result)));
  }
  rules.push_back(rule(app(sym("isSuppressed"),sym("noFlags")),sym("false")));
  rules.push_back(rule(app(sym("isSuppressed"),sym("styleChildren")),sym("false")));
  rules.push_back(rule(app(sym("isSuppressed"),sym("suppress")),sym("true")));
  rules.push_back(rule(app(sym("isSuppressed"),sym("suppressStyleChildren")),sym("true")));
  for(const char* q:{"pNone","pPseudos","pIndependent"})rules.push_back(rule(app(sym("forces"),sym(q)),sym("false")));
  rules.push_back(rule(app(sym("forces"),sym("pChildren")),sym("true")));
  rules.push_back(rule(app(sym("forces"),sym("pDescendants")),sym("true")));
  for(const char* f:{"noFlags","suppress"})for(const char* n:{"element","textNode"})for(const char* d:{"true","false"})
    rules.push_back(rule(ap(sym("flagDependent"),{sym(f),sym(n),sym(d)}),sym("false")));
  for(const char* f:{"styleChildren","suppressStyleChildren"}){
    for(const char* d:{"true","false"})rules.push_back(rule(ap(sym("flagDependent"),{sym(f),sym("element"),sym(d)}),sym(d)));
    for(const char* d:{"true","false"})rules.push_back(rule(ap(sym("flagDependent"),{sym(f),sym("textNode"),sym(d)}),sym("false")));
  }
  rules.push_back(rule(ap(sym("shouldRecalc"),{p(0),p(1),p(2),p(3),p(4)}),
    ap(sym("andB"),{app(sym("notB"),app(sym("isSuppressed"),p(1))),
      ap(sym("or3"),{app(sym("forces"),p(0)),p(3),ap(sym("flagDependent"),{p(1),p(2),p(4)})})})));
  for(const char* q:{"pNone","pPseudos","pIndependent","pChildren"})rules.push_back(rule(app(sym("childProp"),sym(q)),sym("pNone")));
  rules.push_back(rule(app(sym("childProp"),sym("pDescendants")),sym("pDescendants")));
  for(const char* f:{"noFlags","suppress","styleChildren","suppressStyleChildren"})rules.push_back(rule(app(sym("childFlags"),sym(f)),sym("noFlags")));
  rules.push_back(rule(ap(sym("forChildren"),{p(0),p(1)}),ap(sym("childChange"),{app(sym("childProp"),p(0)),app(sym("childFlags"),p(1))})));

  unsigned gate_cases=0,child_cases=0;
  const Prop props[]={Prop::None,Prop::Pseudos,Prop::IndependentInherit,Prop::Children,Prop::Descendants};
  const FlagSet flag_sets[]={FlagSet::None,FlagSet::Suppress,FlagSet::StyleChildren,FlagSet::SuppressStyleChildren};
  for(Prop pr:props)for(FlagSet fl:flag_sets)for(NodeKind nk:{NodeKind::Element,NodeKind::Text})for(bool dirty:{false,true})for(bool dependent:{false,true}){
    Node nd{nk,dirty,dependent};
    Ptr term=ap(sym("shouldRecalc"),{prop_term(pr),flag_term(fl),node_term(nk),bool_term(dirty),bool_term(dependent)});
    auto ty=infer({},term,sig,rules);assert(ty&&conv(*ty,boolean,rules));
    assert(conv(nf(term,rules),bool_term(cpp_should(pr,fl,nd)),rules));++gate_cases;
  }
  for(Prop pr:props)for(FlagSet fl:flag_sets){
    ChildChange expected=cpp_for_children(pr,fl);
    Ptr term=ap(sym("forChildren"),{prop_term(pr),flag_term(fl)});
    auto ty=infer({},term,sig,rules);assert(ty&&conv(*ty,child,rules));
    Ptr expected_term=ap(sym("childChange"),{prop_term(expected.prop),flag_term(expected.flags)});
    assert(conv(nf(term,rules),expected_term,rules));++child_cases;
  }
  std::cout<<"StyleRecalcChange gate agrees on "<<gate_cases<<" finite node/change states\n";
  std::cout<<"ForChildren propagation/flag projection agrees on "<<child_cases<<" finite states\n";
  std::cout<<"Scope: suppression, propagation level, NeedsStyleRecalc, and one style-query-dependent Element path.\n";
}
