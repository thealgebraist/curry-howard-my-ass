#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <vector>

using namespace lp;

// Finite transcription of a restricted path: simple selector matching first,
// then strongest declaration wins. Priority order is lexicographic; equal
// priorities replace the prior winner (later input wins).
enum class K { Tag, Id, Class };
struct Atom { K kind; unsigned name; };
struct Element { unsigned tag, id; std::vector<unsigned> classes; };
struct Decl { std::vector<Atom> selector; std::array<unsigned, 3> priority; unsigned value; };
using Rules = std::vector<Decl>;

static bool matches(const Decl& d, const Element& e) {
  for (auto a : d.selector) {
    bool ok = a.kind == K::Tag ? a.name == e.tag :
              a.kind == K::Id ? e.id != 0 && a.name == e.id :
              std::find(e.classes.begin(), e.classes.end(), a.name) != e.classes.end();
    if (!ok) return false;
  }
  return true;
}
static std::vector<Decl> collect(const Rules& rs, const Element& e) {
  std::vector<Decl> out;
  for (const auto& d : rs) if (matches(d, e)) out.push_back(d);
  return out;
}
static std::optional<unsigned> winner(const std::vector<Decl>& ds) {
  if (ds.empty()) return std::nullopt;
  Decl best = ds.front();
  for (size_t i = 1; i < ds.size(); ++i)
    if (ds[i].priority >= best.priority) best = ds[i];
  return best.value;
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule R(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }
static Ptr n(size_t x) { Ptr t=sym("z"); while(x--) t=app(sym("s"),t); return t; }
static Ptr ns(const std::vector<unsigned>& xs) { Ptr t=sym("nilN"); for(auto i=xs.rbegin();i!=xs.rend();++i)t=apps(sym("consN"),n(*i),t); return t; }
static Ptr atom(Atom a) { return app(sym(a.kind==K::Tag?"tag":a.kind==K::Id?"id":"cls"),n(a.name)); }
static Ptr sel(const std::vector<Atom>& xs) { Ptr t=sym("nilS"); for(auto i=xs.rbegin();i!=xs.rend();++i)t=apps(sym("consS"),atom(*i),t); return t; }
static Ptr elem(const Element& e) { return apps(sym("el"),n(e.tag),n(e.id),ns(e.classes)); }
static Ptr pri(const std::array<unsigned,3>& x) { return apps(sym("pri"),n(x[0]),n(x[1]),n(x[2])); }
static Ptr decl(const Decl& d) { return apps(sym("dcl"),sel(d.selector),pri(d.priority),n(d.value)); }
static Ptr list(const Rules& xs) { Ptr t=sym("nilD"); for(auto i=xs.rbegin();i!=xs.rend();++i)t=apps(sym("consD"),decl(*i),t); return t; }
static Ptr result(std::optional<unsigned> x) { return x ? app(sym("someResult"),n(*x)) : sym("noneResult"); }

int main() {
  const Element e{1, 7, {2,3}};
  const Rules cases[] = {
    {{{{K::Tag,1}}, {1,0,0}, 11}, {{{K::Class,2}}, {2,0,0}, 12}, {{{K::Id,7}}, {3,0,0}, 13}},
    {{{{K::Tag,1}}, {3,0,1}, 21}, {{{K::Id,8}}, {9,0,9}, 22}},
    {{{{K::Class,2}}, {2,0,0}, 31}, {{{K::Class,3}}, {2,0,0}, 32}},
    {{{{K::Tag,9}}, {9,9,9}, 41}, {{{K::Id,7}}, {1,0,0}, 42}},
    {{{{K::Tag,1},{K::Class,2}}, {2,0,0}, 51}, {{{K::Tag,1}}, {2,0,0}, 52}},
    {{{{K::Tag,1}}, {1,0,0}, 61}, {{{K::Class,2}}, {1,2,0}, 62}},
    {{{{K::Tag,1}}, {1,0,0}, 71}, {{{K::Class,2}}, {1,0,2}, 72}},
    {{{{K::Tag,1}}, {1,0,0}, 81}, {{{K::Class,2}}, {1,0,0}, 82}},
    {{{{K::Tag,9}}, {9,9,9}, 91}, {{{K::Id,8}}, {8,8,8}, 92}},
  };
  Signature sig; std::vector<Rule> rs; auto p=[](size_t i){return pvar(i);};
  Ptr B=sym("Bool"), N=sym("Nat"), L=sym("DeclList"), D=sym("Decl"), S=sym("SelList"), A=sym("Atom"), E=sym("Element"), P=sym("Priority");
  sig["Bool"]=type_sort(); sig["t"]=B; sig["f"]=B; sig["Nat"]=type_sort(); sig["z"]=N; sig["s"]=fn(N,N);
  sig["NameList"]=type_sort(); sig["nilN"]=sym("NameList"); sig["consN"]=fn(N,fn(sym("NameList"),sym("NameList")));
  sig["Element"]=type_sort(); sig["el"]=fn(N,fn(N,fn(sym("NameList"),E)));
  sig["Atom"]=type_sort(); sig["tag"]=fn(N,A); sig["id"]=fn(N,A); sig["cls"]=fn(N,A);
  sig["SelList"]=type_sort(); sig["nilS"]=S; sig["consS"]=fn(A,fn(S,S));
  sig["Decl"]=type_sort(); sig["dcl"]=fn(S,fn(P,fn(N,D))); sig["Priority"]=type_sort(); sig["pri"]=fn(N,fn(N,fn(N,P)));
  sig["DeclList"]=type_sort(); sig["nilD"]=L; sig["consD"]=fn(D,fn(L,L));
  sig["Result"]=type_sort(); sig["noneResult"]=sym("Result"); sig["someResult"]=fn(N,sym("Result"));
  for (auto [name,type] : std::vector<std::pair<const char*,Ptr>>{{"eq",fn(N,fn(N,B))},{"lt",fn(N,fn(N,B))},{"and",fn(B,fn(B,B))},{"or",fn(B,fn(B,B))},{"member",fn(sym("NameList"),fn(N,B))},{"matchAtom",fn(A,fn(E,B))},{"matchAll",fn(S,fn(E,B))},{"matchDecl",fn(D,fn(E,B))},{"collect",fn(L,fn(E,L))},{"cmpN",fn(N,fn(N,sym("Ord")))},{"then",fn(sym("Ord"),fn(sym("Ord"),sym("Ord")))},{"cmpP",fn(P,fn(P,sym("Ord")))},{"chooseBy",fn(sym("Ord"),fn(D,fn(D,D)))},{"choose",fn(D,fn(D,D))},{"best",fn(L,D)},{"value",fn(D,sym("Result"))},{"rulesAt",fn(N,L)},{"elementAt",fn(N,E)},{"observe",fn(N,sym("Result"))}}) sig[name]=type;
  sig["Ord"]=type_sort(); sig["less"]=sym("Ord"); sig["same"]=sym("Ord"); sig["more"]=sym("Ord");
  auto q=sym("t"), f=sym("f");
  rs.push_back(R(apps(sym("eq"),sym("z"),sym("z")),q)); rs.push_back(R(apps(sym("eq"),sym("z"),app(sym("s"),p(0))),f)); rs.push_back(R(apps(sym("eq"),app(sym("s"),p(0)),sym("z")),f)); rs.push_back(R(apps(sym("eq"),app(sym("s"),p(0)),app(sym("s"),p(1))),apps(sym("eq"),p(0),p(1))));
  rs.push_back(R(apps(sym("lt"),sym("z"),sym("z")),f)); rs.push_back(R(apps(sym("lt"),sym("z"),app(sym("s"),p(0))),q)); rs.push_back(R(apps(sym("lt"),app(sym("s"),p(0)),sym("z")),f)); rs.push_back(R(apps(sym("lt"),app(sym("s"),p(0)),app(sym("s"),p(1))),apps(sym("lt"),p(0),p(1))));
  rs.push_back(R(apps(sym("and"),q,p(0)),p(0))); rs.push_back(R(apps(sym("and"),f,p(0)),f)); rs.push_back(R(apps(sym("or"),q,p(0)),q)); rs.push_back(R(apps(sym("or"),f,p(0)),p(0)));
  rs.push_back(R(apps(sym("member"),sym("nilN"),p(0)),f)); rs.push_back(R(apps(sym("member"),apps(sym("consN"),p(0),p(1)),p(2)),apps(sym("or"),apps(sym("eq"),p(0),p(2)),apps(sym("member"),p(1),p(2)))));
  rs.push_back(R(apps(sym("matchAtom"),app(sym("tag"),p(0)),apps(sym("el"),p(1),p(2),p(3))),apps(sym("eq"),p(0),p(1))));
  rs.push_back(R(apps(sym("matchAtom"),app(sym("id"),p(0)),apps(sym("el"),p(1),sym("z"),p(2))),f)); rs.push_back(R(apps(sym("matchAtom"),app(sym("id"),p(0)),apps(sym("el"),p(1),p(2),p(3))),apps(sym("eq"),p(0),p(2))));
  rs.push_back(R(apps(sym("matchAtom"),app(sym("cls"),p(0)),apps(sym("el"),p(1),p(2),p(3))),apps(sym("member"),p(3),p(0))));
  rs.push_back(R(apps(sym("matchAll"),sym("nilS"),p(0)),q)); rs.push_back(R(apps(sym("matchAll"),apps(sym("consS"),p(0),p(1)),p(2)),apps(sym("and"),apps(sym("matchAtom"),p(0),p(2)),apps(sym("matchAll"),p(1),p(2)))));
  rs.push_back(R(apps(sym("matchDecl"),apps(sym("dcl"),p(0),p(1),p(2)),p(3)),apps(sym("matchAll"),p(0),p(3))));
  rs.push_back(R(apps(sym("collect"),sym("nilD"),p(0)),sym("nilD")));
  rs.push_back(R(apps(sym("collect"),apps(sym("consD"),p(0),p(1)),p(2)),apps(sym("collectStep"),p(0),apps(sym("collect"),p(1),p(2)),p(2))));
  sig["collectStep"]=fn(D,fn(L,fn(E,L)));
  rs.push_back(R(apps(sym("collectStep"),p(0),p(1),p(2)),apps(sym("collectIf"),apps(sym("matchDecl"),p(0),p(2)),p(0),p(1)))); sig["collectIf"]=fn(B,fn(D,fn(L,L)));
  rs.push_back(R(apps(sym("collectIf"),q,p(0),p(1)),apps(sym("consD"),p(0),p(1)))); rs.push_back(R(apps(sym("collectIf"),f,p(0),p(1)),p(1)));
  rs.push_back(R(apps(sym("cmpN"),sym("z"),sym("z")),sym("same"))); rs.push_back(R(apps(sym("cmpN"),sym("z"),app(sym("s"),p(0))),sym("less"))); rs.push_back(R(apps(sym("cmpN"),app(sym("s"),p(0)),sym("z")),sym("more"))); rs.push_back(R(apps(sym("cmpN"),app(sym("s"),p(0)),app(sym("s"),p(1))),apps(sym("cmpN"),p(0),p(1))));
  rs.push_back(R(apps(sym("then"),sym("less"),p(0)),sym("less"))); rs.push_back(R(apps(sym("then"),sym("more"),p(0)),sym("more"))); rs.push_back(R(apps(sym("then"),sym("same"),p(0)),p(0)));
  std::vector<Ptr> ca,cb,cc; for(int i=0;i<3;i++){ca.push_back(p(i));cb.push_back(p(i+3));cc.push_back(apps(sym("cmpN"),p(i),p(i+3)));}
  Ptr lex=cc[2]; for(int i=1;i>=0;i--) lex=apps(sym("then"),cc[i],lex);
  rs.push_back(R(apps(sym("cmpP"),roll(sym("pri"),ca),roll(sym("pri"),cb)),lex));
  sig["default"]=D; rs.push_back(R(apps(sym("chooseBy"),sym("more"),p(0),p(1)),p(0))); rs.push_back(R(apps(sym("chooseBy"),sym("same"),p(0),p(1)),p(1))); rs.push_back(R(apps(sym("chooseBy"),sym("less"),p(0),p(1)),p(1)));
  rs.push_back(R(apps(sym("choose"),p(0),sym("default")),p(0))); rs.push_back(R(apps(sym("choose"),apps(sym("dcl"),p(0),p(1),p(2)),apps(sym("dcl"),p(3),p(4),p(5))),apps(sym("chooseBy"),apps(sym("cmpP"),p(1),p(4)),apps(sym("dcl"),p(0),p(1),p(2)),apps(sym("dcl"),p(3),p(4),p(5)))));
  rs.push_back(R(app(sym("best"),sym("nilD")),sym("default"))); rs.push_back(R(app(sym("best"),apps(sym("consD"),p(0),p(1))),apps(sym("choose"),p(0),app(sym("best"),p(1)))));
  rs.push_back(R(app(sym("value"),sym("default")),sym("noneResult"))); rs.push_back(R(app(sym("value"),apps(sym("dcl"),p(0),p(1),p(2))),app(sym("someResult"),p(2))));
  rs.push_back(R(apps(sym("observe"),p(0)),app(sym("value"),app(sym("best"),apps(sym("collect"),app(sym("rulesAt"),p(0)),app(sym("elementAt"),p(0)))))));
  for(size_t i=0;i<std::size(cases);i++){rs.push_back(R(app(sym("rulesAt"),n(i)),list(cases[i]))); rs.push_back(R(app(sym("elementAt"),n(i)),elem(e)));}
  Ptr yes=sym("t"); auto ty=infer({},app(sym("observe"),n(0)),sig,rs); assert(ty&&conv(*ty,sym("Result"),rs));
  for(size_t i=0;i<std::size(cases);i++) assert(conv(nf(app(sym("observe"),n(i)),rs),result(winner(collect(cases[i],e))),rs));
  std::println("CSS selector collection plus cascade winner: C++ and typed λΠ agree on {} finite rule sets", std::size(cases));
  std::println("Scope: one element, conjunctive tag/id/class selectors, one property, 3-field abstract priority; later equal-priority input wins.");
}
