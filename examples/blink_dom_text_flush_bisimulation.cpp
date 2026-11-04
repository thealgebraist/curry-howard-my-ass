#include "../src/lambda_pi.hpp"
#include <cassert>
#include <string>
#include <vector>
using namespace lp;

enum class Kind { Element, Text };
struct Child { Kind kind; unsigned id; std::string data; };
struct Pending { unsigned parent; std::string data; };
struct Result { unsigned parent; std::vector<Child> children; unsigned next_id; enum Action { Discard, Insert, Coalesce } action; unsigned affected_id; };

static Result cpp_flush(Pending p, std::vector<Child> children, bool valid, unsigned next) {
  if (p.data.empty() || !valid) return {p.parent,std::move(children),next,Result::Discard,0};
  if (!children.empty() && children.back().kind==Kind::Text) {
    const unsigned reused_id=children.back().id;
    children.back().data+=p.data;
    return {p.parent,std::move(children),next,Result::Coalesce,reused_id};
  }
  children.push_back({Kind::Text,next,p.data});
  return {p.parent,std::move(children),next+1,Result::Insert,next};
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr n(unsigned x){Ptr r=sym("z");while(x--)r=app(sym("s"),r);return r;}
static Ptr tx(const std::string& s){return sym(s.empty()?"emptyText":s);}
static Ptr children_term(const std::vector<Child>& cs){Ptr r=sym("nilChildren");for(auto i=cs.rbegin();i!=cs.rend();++i){Ptr c=i->kind==Kind::Text?apps(sym("textChild"),n(i->id),tx(i->data)):app(sym("elementChild"),n(i->id));r=apps(sym("consChild"),c,r);}return r;}
static Ptr pending_term(Pending p){return apps(sym("pending"),n(p.parent),tx(p.data));}
static Ptr action_term(const Result& r){return apps(sym(r.action==Result::Discard?"discarded":r.action==Result::Insert?"insertedText":"appendedText"),n(r.affected_id));}
static Ptr result_term(const Result& r){return apps(sym("flushResult"),n(r.parent),children_term(r.children),n(r.next_id),action_term(r));}
static Ptr app4(Ptr f,Ptr a,Ptr b,Ptr c,Ptr d){return apps(std::move(f),std::move(a),std::move(b),std::move(c),std::move(d));}

int main(){
 Signature sig;std::vector<Rule> rs;auto p=[](int i){return pvar(i);};
 Ptr Nat=sym("Nat"),Text=sym("Text"),Bool=sym("Bool"),ChildT=sym("Child"),Children=sym("Children"),PendingT=sym("Pending"),ResultT=sym("FlushResult");
 sig["Nat"]=type_sort();sig["z"]=Nat;sig["s"]=fn(Nat,Nat);
 sig["Text"]=type_sort();for(auto x:{"emptyText","a","b","ab","c","ac"})sig[x]=Text;sig["cat"]=fn(Text,fn(Text,Text));
 sig["Bool"]=type_sort();sig["yes"]=Bool;sig["no"]=Bool;
 sig["Child"]=type_sort();sig["elementChild"]=fn(Nat,ChildT);sig["textChild"]=fn(Nat,fn(Text,ChildT));
 sig["Children"]=type_sort();sig["nilChildren"]=Children;sig["consChild"]=fn(ChildT,fn(Children,Children));
 sig["Pending"]=type_sort();sig["pending"]=fn(Nat,fn(Text,PendingT));
 sig["Action"]=type_sort();for(auto x:{"discarded","insertedText","appendedText"})sig[x]=fn(Nat,sym("Action"));
 sig["FlushResult"]=type_sort();sig["flushResult"]=fn(Nat,fn(Children,fn(Nat,fn(sym("Action"),ResultT))));
 sig["appendAtEnd"]=fn(Nat,fn(Children,fn(Text,fn(Nat,ResultT))));
 sig["wrapResult"]=fn(ChildT,fn(ResultT,ResultT));
 sig["flushPending"]=fn(PendingT,fn(Children,fn(Bool,fn(Nat,ResultT))));
 // Append at end: reuse the final Text node identity when it exists; otherwise
 // append a new Text child with the next modeled identity.
 rs.push_back(rule(apps(sym("cat"),sym("emptyText"),p(0)),p(0)));
 rs.push_back(rule(apps(sym("cat"),sym("a"),sym("b")),sym("ab")));
 rs.push_back(rule(apps(sym("cat"),sym("a"),sym("c")),sym("ac")));
 rs.push_back(rule(apps(sym("appendAtEnd"),p(0),sym("nilChildren"),p(1),p(2)),
   apps(sym("flushResult"),p(0),apps(sym("consChild"),apps(sym("textChild"),p(2),p(1)),sym("nilChildren")),app(sym("s"),p(2)),apps(sym("insertedText"),p(2)))));
 rs.push_back(rule(apps(sym("appendAtEnd"),p(0),apps(sym("consChild"),apps(sym("textChild"),p(1),p(2)),sym("nilChildren")),p(3),p(4)),
   apps(sym("flushResult"),p(0),apps(sym("consChild"),apps(sym("textChild"),p(1),apps(sym("cat"),p(2),p(3))),sym("nilChildren")),p(4),apps(sym("appendedText"),p(1)))));
 rs.push_back(rule(apps(sym("appendAtEnd"),p(0),apps(sym("consChild"),p(1),p(2)),p(3),p(4)),
   apps(sym("wrapResult"),p(1),app4(sym("appendAtEnd"),p(0),p(2),p(3),p(4)))));
 rs.push_back(rule(apps(sym("wrapResult"),p(0),apps(sym("flushResult"),p(1),p(2),p(3),p(4))),
   apps(sym("flushResult"),p(1),apps(sym("consChild"),p(0),p(2)),p(3),p(4))));
 rs.push_back(rule(apps(sym("flushPending"),apps(sym("pending"),p(0),sym("emptyText")),p(1),p(2),p(3)),
   apps(sym("flushResult"),p(0),p(1),p(3),apps(sym("discarded"),n(0)))));
 rs.push_back(rule(apps(sym("flushPending"),apps(sym("pending"),p(0),p(1)),p(2),sym("no"),p(3)),
   apps(sym("flushResult"),p(0),p(2),p(3),apps(sym("discarded"),n(0)))));
 rs.push_back(rule(apps(sym("flushPending"),apps(sym("pending"),p(0),p(1)),p(2),sym("yes"),p(3)),
   app4(sym("appendAtEnd"),p(0),p(2),p(1),p(3))));
 rs.push_back(rule(apps(sym("cat"),sym("ab"),sym("c")),sym("abc")));

 RuleIndex ix(rs);
 struct Case { Pending pending; std::vector<Child> children; bool valid; unsigned next; };
 const std::vector<Case> cases={
   {{7,"a"},{},true,10},
   {{7,"b"},{{Kind::Text,4,"a"}},true,10},
   {{7,"c"},{{Kind::Element,2,{}},{Kind::Text,4,"a"}},true,10},
   {{7,"b"},{{Kind::Element,2,{}}},true,10},
   {{7,""},{{Kind::Element,2,{}}},true,10},
   {{7,"b"},{{Kind::Text,4,"a"}},false,10},
   {{7,"c"},{{Kind::Element,2,{}},{Kind::Element,3,{}}},true,10}
 };
 for(const auto& c:cases){
   Result native=cpp_flush(c.pending,c.children,c.valid,c.next);
   Ptr term=apps(sym("flushPending"),pending_term(c.pending),children_term(c.children),sym(c.valid?"yes":"no"),n(c.next));
   auto ty=infer({},term,sig,ix);assert(ty&&conv(*ty,ResultT,ix));
   Ptr actual=nf(term,ix),expected=result_term(native);
   if(!conv(actual,expected,ix)){std::println("flush mismatch expected={} actual={}",to_string(expected),to_string(actual));return 2;}
 }
 std::println("Blink DOM text flush: {} finite child-list mutations agree; insertion allocates a Text child, coalescing preserves its identity",cases.size());
 std::println("Scope: FlushPendingText at a null next_child location, so mutation is at the end of one parent's child list. Invalid/empty flushes discard; adjustment, templates, foster parenting, actual Blink Node pointers, and String canonicalization are outside this model.");
}
