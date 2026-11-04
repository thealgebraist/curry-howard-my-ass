#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <vector>

using namespace lp;
enum class SK{Tag,Id,Class};
struct Atom{SK kind;unsigned name;};
struct Element{unsigned tag,id;std::vector<unsigned> classes;};
enum class VK{Explicit,Inherit,Initial,Unset};
struct CssValue{VK kind;unsigned color=0;};
struct Decl{std::vector<Atom> selector;std::array<unsigned,3> priority;CssValue value;};
struct Case{Element element;std::vector<Decl> declarations;bool has_parent;unsigned parent;};
static bool matches(const Decl&d,const Element&e){for(auto a:d.selector){bool ok=a.kind==SK::Tag?a.name==e.tag:a.kind==SK::Id?(e.id&&a.name==e.id):std::find(e.classes.begin(),e.classes.end(),a.name)!=e.classes.end();if(!ok)return false;}return true;}
static unsigned cpp_color(const Case&c){const Decl*best=nullptr;for(const auto&d:c.declarations)if(matches(d,c.element)&&(!best||d.priority>=best->priority))best=&d;CssValue v=best?best->value:CssValue{VK::Unset,0};switch(v.kind){case VK::Explicit:return v.color;case VK::Initial:return 0;case VK::Inherit:case VK::Unset:return c.has_parent?c.parent:0;}return 0;}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}static Ptr n(size_t x){Ptr t=sym("z");while(x--)t=app(sym("s"),t);return t;}
static Ptr ns(const std::vector<unsigned>&xs){Ptr t=sym("nilName");for(auto i=xs.rbegin();i!=xs.rend();++i)t=apps(sym("consName"),n(*i),t);return t;}
static Ptr atom(Atom a){return app(sym(a.kind==SK::Tag?"tag":a.kind==SK::Id?"id":"cls"),n(a.name));}
static Ptr selectors(const std::vector<Atom>&xs){Ptr t=sym("nilSel");for(auto i=xs.rbegin();i!=xs.rend();++i)t=apps(sym("consSel"),atom(*i),t);return app(sym("compound"),t);}
static Ptr element(const Element&e){return apps(sym("el"),n(e.tag),n(e.id),ns(e.classes));}
static Ptr priority(const std::array<unsigned,3>&x){return apps(sym("priority"),n(x[0]),n(x[1]),n(x[2]));}
static Ptr css_value(CssValue v){switch(v.kind){case VK::Explicit:return app(sym("explicitColor"),n(v.color));case VK::Inherit:return sym("inheritK");case VK::Initial:return sym("initialK");case VK::Unset:return sym("unsetK");}return sym("unsetK");}
static Ptr declaration(const Decl&d){return apps(sym("declaration"),selectors(d.selector),priority(d.priority),css_value(d.value));}
static Ptr declaration_list(const std::vector<Decl>&ds){Ptr t=sym("nilDecl");for(auto i=ds.rbegin();i!=ds.rend();++i)t=apps(sym("consDecl"),declaration(*i),t);return t;}

int main(){
  const Element e{1,7,{2,3}};
  const std::vector<Case> cases={
    {e,{{{{SK::Id,7}},{3,0,0},{VK::Explicit,44}},{{{SK::Tag,1}},{1,0,0},{VK::Explicit,33}}},true,9},
    {e,{{{{SK::Id,8}},{9,0,0},{VK::Explicit,88}},{{{SK::Tag,1}},{1,0,0},{VK::Explicit,21}}},true,9},
    {e,{{{{SK::Class,2}},{2,0,0},{VK::Explicit,22}},{{{SK::Class,3}},{2,0,0},{VK::Explicit,23}}},true,9},
    {e,{{{{SK::Tag,1}},{1,0,0},{VK::Initial,0}}},true,9},
    {e,{{{{SK::Tag,1}},{1,0,0},{VK::Initial,0}}},false,9},
    {e,{{{{SK::Tag,1}},{1,0,0},{VK::Inherit,0}}},true,17},
    {e,{{{{SK::Tag,1}},{1,0,0},{VK::Inherit,0}}},false,17},
    {e,{{{{SK::Tag,1}},{1,0,0},{VK::Unset,0}}},true,18},
    {e,{{{{SK::Tag,1}},{1,0,0},{VK::Unset,0}}},false,18},
    {e,{{{{SK::Tag,1}},{1,0,0},{VK::Explicit,42}}},false,0},
    {e,{},true,27},
    {e,{},false,27},
  };
  Signature sig;std::vector<Rule>rs;auto p=[](size_t i){return pvar(i);};
  Ptr N=sym("Nat"),B=sym("Bool"),A=sym("Atom"),E=sym("Element"),SL=sym("SelectorList"),S=sym("Selector"),P=sym("Priority"),D=sym("Declaration"),DL=sym("DeclList"),V=sym("CssValue");
  sig["Nat"]=type_sort();sig["z"]=N;sig["s"]=fn(N,N);sig["Bool"]=type_sort();sig["yes"]=B;sig["no"]=B;
  sig["NameList"]=type_sort();sig["nilName"]=sym("NameList");sig["consName"]=fn(N,fn(sym("NameList"),sym("NameList")));
  sig["Element"]=type_sort();sig["el"]=fn(N,fn(N,fn(sym("NameList"),E)));
  sig["Atom"]=type_sort();sig["tag"]=fn(N,A);sig["id"]=fn(N,A);sig["cls"]=fn(N,A);sig["SelectorList"]=type_sort();sig["nilSel"]=SL;sig["consSel"]=fn(A,fn(SL,SL));
  sig["Selector"]=type_sort();sig["compound"]=fn(SL,S);sig["Priority"]=type_sort();sig["priority"]=fn(N,fn(N,fn(N,P)));
  sig["CssValue"]=type_sort();sig["explicitColor"]=fn(N,V);for(const char*x:{"inheritK","initialK","unsetK","missingK"})sig[x]=V;
  sig["Declaration"]=type_sort();sig["declaration"]=fn(S,fn(P,fn(V,D)));sig["DeclList"]=type_sort();sig["nilDecl"]=DL;sig["consDecl"]=fn(D,fn(DL,DL));
  sig["Ordering"]=type_sort();for(const char*x:{"less","same","more"})sig[x]=sym("Ordering");
  sig["Result"]=type_sort();
  sig["eq"]=fn(N,fn(N,B));sig["lt"]=fn(N,fn(N,B));sig["andB"]=fn(B,fn(B,B));sig["orB"]=fn(B,fn(B,B));sig["member"]=fn(sym("NameList"),fn(N,B));sig["matchAtom"]=fn(A,fn(E,B));sig["matchAll"]=fn(SL,fn(E,B));sig["matchSelector"]=fn(S,fn(E,B));sig["matchDecl"]=fn(D,fn(E,B));sig["collect"]=fn(DL,fn(E,DL));sig["collectStep"]=fn(D,fn(DL,fn(E,DL)));sig["collectIf"]=fn(B,fn(D,fn(DL,DL)));
  sig["cmpNat"]=fn(N,fn(N,sym("Ordering")));sig["thenCmp"]=fn(sym("Ordering"),fn(sym("Ordering"),sym("Ordering")));sig["cmpPriority"]=fn(P,fn(P,sym("Ordering")));sig["chooseBy"]=fn(sym("Ordering"),fn(D,fn(D,D)));sig["choose"]=fn(D,fn(D,D));sig["best"]=fn(DL,D);sig["valueOf"]=fn(D,V);
  sig["parentInitial"]=fn(B,fn(N,fn(N,N)));sig["defaultColor"]=fn(V,fn(B,fn(N,fn(N,N))));sig["computedColor"]=fn(DL,fn(E,fn(B,fn(N,fn(N,N)))));
  sig["rulesAt"]=fn(N,DL);sig["elementAt"]=fn(N,E);sig["parentAt"]=fn(N,B);sig["parentColorAt"]=fn(N,N);sig["observe"]=fn(N,N);
  auto yes=sym("yes"),no=sym("no");
  rs.push_back(R(apps(sym("eq"),sym("z"),sym("z")),yes));rs.push_back(R(apps(sym("eq"),sym("z"),app(sym("s"),p(0))),no));rs.push_back(R(apps(sym("eq"),app(sym("s"),p(0)),sym("z")),no));rs.push_back(R(apps(sym("eq"),app(sym("s"),p(0)),app(sym("s"),p(1))),apps(sym("eq"),p(0),p(1))));
  rs.push_back(R(apps(sym("lt"),sym("z"),sym("z")),no));rs.push_back(R(apps(sym("lt"),sym("z"),app(sym("s"),p(0))),yes));rs.push_back(R(apps(sym("lt"),app(sym("s"),p(0)),sym("z")),no));rs.push_back(R(apps(sym("lt"),app(sym("s"),p(0)),app(sym("s"),p(1))),apps(sym("lt"),p(0),p(1))));
  rs.push_back(R(apps(sym("andB"),yes,p(0)),p(0)));rs.push_back(R(apps(sym("andB"),no,p(0)),no));rs.push_back(R(apps(sym("orB"),yes,p(0)),yes));rs.push_back(R(apps(sym("orB"),no,p(0)),p(0)));
  rs.push_back(R(apps(sym("member"),sym("nilName"),p(0)),no));rs.push_back(R(apps(sym("member"),apps(sym("consName"),p(0),p(1)),p(2)),apps(sym("orB"),apps(sym("eq"),p(0),p(2)),apps(sym("member"),p(1),p(2)))));
  rs.push_back(R(apps(sym("matchAtom"),app(sym("tag"),p(0)),apps(sym("el"),p(1),p(2),p(3))),apps(sym("eq"),p(0),p(1))));
  rs.push_back(R(apps(sym("matchAtom"),app(sym("id"),p(0)),apps(sym("el"),p(1),sym("z"),p(2))),no));rs.push_back(R(apps(sym("matchAtom"),app(sym("id"),p(0)),apps(sym("el"),p(1),p(2),p(3))),apps(sym("eq"),p(0),p(2))));
  rs.push_back(R(apps(sym("matchAtom"),app(sym("cls"),p(0)),apps(sym("el"),p(1),p(2),p(3))),apps(sym("member"),p(3),p(0))));rs.push_back(R(apps(sym("matchAll"),sym("nilSel"),p(0)),yes));rs.push_back(R(apps(sym("matchAll"),apps(sym("consSel"),p(0),p(1)),p(2)),apps(sym("andB"),apps(sym("matchAtom"),p(0),p(2)),apps(sym("matchAll"),p(1),p(2)))));
  rs.push_back(R(apps(sym("matchSelector"),app(sym("compound"),p(0)),p(1)),apps(sym("matchAll"),p(0),p(1))));rs.push_back(R(apps(sym("matchDecl"),apps(sym("declaration"),p(0),p(1),p(2)),p(3)),apps(sym("matchSelector"),p(0),p(3))));
  rs.push_back(R(apps(sym("collect"),sym("nilDecl"),p(0)),sym("nilDecl")));rs.push_back(R(apps(sym("collect"),apps(sym("consDecl"),p(0),p(1)),p(2)),apps(sym("collectStep"),p(0),apps(sym("collect"),p(1),p(2)),p(2))));rs.push_back(R(apps(sym("collectStep"),p(0),p(1),p(2)),apps(sym("collectIf"),apps(sym("matchDecl"),p(0),p(2)),p(0),p(1))));rs.push_back(R(apps(sym("collectIf"),yes,p(0),p(1)),apps(sym("consDecl"),p(0),p(1))));rs.push_back(R(apps(sym("collectIf"),no,p(0),p(1)),p(1)));
  rs.push_back(R(apps(sym("cmpNat"),sym("z"),sym("z")),sym("same")));rs.push_back(R(apps(sym("cmpNat"),sym("z"),app(sym("s"),p(0))),sym("less")));rs.push_back(R(apps(sym("cmpNat"),app(sym("s"),p(0)),sym("z")),sym("more")));rs.push_back(R(apps(sym("cmpNat"),app(sym("s"),p(0)),app(sym("s"),p(1))),apps(sym("cmpNat"),p(0),p(1))));rs.push_back(R(apps(sym("thenCmp"),sym("less"),p(0)),sym("less")));rs.push_back(R(apps(sym("thenCmp"),sym("more"),p(0)),sym("more")));rs.push_back(R(apps(sym("thenCmp"),sym("same"),p(0)),p(0)));
  std::vector<Ptr>a,b,c;for(size_t i=0;i<3;++i){a.push_back(p(i));b.push_back(p(i+3));c.push_back(apps(sym("cmpNat"),p(i),p(i+3)));}Ptr lex=c[2];for(int i=1;i>=0;--i)lex=apps(sym("thenCmp"),c[i],lex);rs.push_back(R(apps(sym("cmpPriority"),roll(sym("priority"),a),roll(sym("priority"),b)),lex));
  rs.push_back(R(apps(sym("chooseBy"),sym("more"),p(0),p(1)),p(0)));rs.push_back(R(apps(sym("chooseBy"),sym("same"),p(0),p(1)),p(1)));rs.push_back(R(apps(sym("chooseBy"),sym("less"),p(0),p(1)),p(1)));rs.push_back(R(apps(sym("choose"),p(0),sym("noDecl")),p(0)));rs.push_back(R(apps(sym("choose"),apps(sym("declaration"),p(0),p(1),p(2)),apps(sym("declaration"),p(3),p(4),p(5))),apps(sym("chooseBy"),apps(sym("cmpPriority"),p(1),p(4)),apps(sym("declaration"),p(0),p(1),p(2)),apps(sym("declaration"),p(3),p(4),p(5)))));sig["noDecl"]=D;rs.push_back(R(app(sym("best"),sym("nilDecl")),sym("noDecl")));rs.push_back(R(app(sym("best"),apps(sym("consDecl"),p(0),p(1))),apps(sym("choose"),p(0),app(sym("best"),p(1)))));
  rs.push_back(R(app(sym("valueOf"),sym("noDecl")),sym("missingK")));rs.push_back(R(app(sym("valueOf"),apps(sym("declaration"),p(0),p(1),p(2))),p(2)));
  rs.push_back(R(apps(sym("parentInitial"),sym("yes"),p(0),p(1)),p(0)));rs.push_back(R(apps(sym("parentInitial"),sym("no"),p(0),p(1)),p(1)));
  rs.push_back(R(apps(sym("defaultColor"),app(sym("explicitColor"),p(0)),p(1),p(2),p(3)),p(0)));rs.push_back(R(apps(sym("defaultColor"),sym("initialK"),p(0),p(1),p(2)),p(2)));for(const char*k:{"inheritK","unsetK","missingK"})rs.push_back(R(apps(sym("defaultColor"),sym(k),p(0),p(1),p(2)),apps(sym("parentInitial"),p(0),p(1),p(2))));
  rs.push_back(R(apps(sym("computedColor"),p(0),p(1),p(2),p(3),p(4)),apps(sym("defaultColor"),app(sym("valueOf"),app(sym("best"),apps(sym("collect"),p(0),p(1)))),p(2),p(3),p(4))));
  rs.push_back(R(app(sym("observe"),p(0)),apps(sym("computedColor"),app(sym("rulesAt"),p(0)),app(sym("elementAt"),p(0)),app(sym("parentAt"),p(0)),app(sym("parentColorAt"),p(0)),sym("z"))));
  for(size_t i=0;i<cases.size();++i){rs.push_back(R(app(sym("rulesAt"),n(i)),declaration_list(cases[i].declarations)));rs.push_back(R(app(sym("elementAt"),n(i)),element(cases[i].element)));rs.push_back(R(app(sym("parentAt"),n(i)),sym(cases[i].has_parent?"yes":"no")));rs.push_back(R(app(sym("parentColorAt"),n(i)),n(cases[i].parent)));}
  auto type=infer({},app(sym("observe"),n(0)),sig,rs);assert(type&&conv(*type,N,rs));for(size_t i=0;i<cases.size();++i){auto got=nf(app(sym("observe"),n(i)),rs);if(!conv(got,n(cpp_color(cases[i])),rs)){std::println("case {} expected {}, got {}",i,cpp_color(cases[i]),to_string(got));return 2;}}
  std::println("CSS selector collection, winner selection, and inherited color defaulting agree on {} finite cases",cases.size());
  std::println("Scope: one element, tag/id/class conjunctions, one abstract priority projection, inherited color only; values/parent colors are IDs and initial black is 0.");
}
