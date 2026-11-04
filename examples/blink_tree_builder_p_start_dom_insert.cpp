#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <vector>

using namespace lp;
enum class Tag { P, Div, Button, Html, Span };
static const char* tn(Tag t){static constexpr std::array n={"pTag","divTag","buttonTag","htmlTag","spanTag"};return n[static_cast<size_t>(t)];}
struct Entry{unsigned id;Tag tag;};
struct Attr{unsigned name,value;};
struct Node{unsigned id,parent;Tag tag;std::vector<Attr> attrs;};
struct State{std::vector<Node> dom;std::vector<Entry> stack;unsigned next;};
using Attrs=std::vector<Attr>;
static bool scope_p(const std::vector<Entry>& s){for(auto e:s){if(e.tag==Tag::P)return true;if(e.tag==Tag::Button||e.tag==Tag::Html)return false;}return false;}
static State cpp_transition(State s,Tag tag,const Attrs& attrs){
 if(scope_p(s.stack)){size_t i=0;while(i<s.stack.size()&&s.stack[i].tag!=Tag::P)++i;if(i<s.stack.size())s.stack.erase(s.stack.begin(),s.stack.begin()+static_cast<std::ptrdiff_t>(i+1));}
 unsigned parent=s.stack.empty()?0:s.stack.front().id;unsigned id=s.next++;
 s.dom.push_back({id,parent,tag,attrs});s.stack.insert(s.stack.begin(),{id,tag});return s;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr n(unsigned x){Ptr r=sym("zero");while(x--)r=app(sym("succ"),r);return r;}
static Ptr stack_term(const std::vector<Entry>& s){Ptr r=sym("nilStack");for(auto i=s.rbegin();i!=s.rend();++i)r=apps(sym("consEntry"),apps(sym("entry"),n(i->id),sym(tn(i->tag))),r);return r;}
static Ptr attrs_term(const Attrs& a){Ptr r=sym("nilAttrs");for(auto i=a.rbegin();i!=a.rend();++i)r=apps(sym("consAttr"),apps(sym("attr"),sym(i->name?"idName":"className"),sym(i->value?"beta":"alpha")),r);return r;}
static Ptr node_term(const Node& x){return apps(sym("node"),n(x.id),n(x.parent),sym(tn(x.tag)),attrs_term(x.attrs));}
static Ptr nodes_term(const std::vector<Node>& xs){Ptr r=sym("nilNodes");for(auto i=xs.rbegin();i!=xs.rend();++i)r=apps(sym("consNode"),node_term(*i),r);return r;}
static Ptr state_term(const State& s){return apps(sym("state"),nodes_term(s.dom),stack_term(s.stack),n(s.next));}
int main(){
 Signature sig;std::vector<Rule> rs;auto p=[](int i){return pvar(i);};
 Ptr nat=sym("Nat"),tag=sym("Tag"),attrs=sym("Attrs"),stack=sym("Stack"),entry=sym("Entry"),node=sym("Node"),nodes=sym("Nodes"),state=sym("State"),bit=sym("Bool");
 sig["Nat"]=type_sort();sig["zero"]=nat;sig["succ"]=fn(nat,nat);sig["Tag"]=type_sort();for(Tag t:{Tag::P,Tag::Div,Tag::Button,Tag::Html,Tag::Span})sig[tn(t)]=tag;
 sig["Bool"]=type_sort();sig["true"]=bit;sig["false"]=bit;sig["AttributeName"]=type_sort();sig["className"]=sym("AttributeName");sig["idName"]=sym("AttributeName");sig["AttributeValue"]=type_sort();sig["alpha"]=sym("AttributeValue");sig["beta"]=sym("AttributeValue");sig["Attr"]=type_sort();sig["attr"]=fn(sym("AttributeName"),fn(sym("AttributeValue"),sym("Attr")));
 sig["Attrs"]=type_sort();sig["nilAttrs"]=attrs;sig["consAttr"]=fn(sym("Attr"),fn(attrs,attrs));sig["Entry"]=type_sort();sig["entry"]=fn(nat,fn(tag,entry));sig["Stack"]=type_sort();sig["nilStack"]=stack;sig["consEntry"]=fn(entry,fn(stack,stack));sig["Nodes"]=type_sort();sig["nilNodes"]=nodes;sig["Node"]=type_sort();sig["node"]=fn(nat,fn(nat,fn(tag,fn(attrs,node))));sig["consNode"]=fn(node,fn(nodes,nodes));sig["State"]=type_sort();sig["state"]=fn(nodes,fn(stack,fn(nat,state)));
 sig["isP"]=fn(tag,bit);sig["isMarker"]=fn(tag,bit);sig["scopeP"]=fn(stack,bit);sig["dropP"]=fn(stack,stack);sig["ifStack"]=fn(bit,fn(stack,fn(stack,stack)));sig["popForBlock"]=fn(stack,stack);sig["parentOf"]=fn(stack,nat);sig["appendNode"]=fn(nodes,fn(node,nodes));sig["insert"]=fn(tag,fn(attrs,fn(state,state)));
 sig["inputState"]=fn(nat,state);sig["inputTag"]=fn(nat,tag);sig["inputAttrs"]=fn(nat,attrs);sig["sem"]=fn(nat,state);sig["native"]=fn(nat,state);
 Ptr yes=sym("true"),no=sym("false");
 rs.push_back(rule(apps(sym("isP"),sym("pTag")),yes));rs.push_back(rule(apps(sym("isP"),p(0)),no));
 rs.push_back(rule(apps(sym("isMarker"),sym("buttonTag")),yes));rs.push_back(rule(apps(sym("isMarker"),sym("htmlTag")),yes));rs.push_back(rule(apps(sym("isMarker"),p(0)),no));
 rs.push_back(rule(app(sym("scopeP"),sym("nilStack")),no));rs.push_back(rule(apps(sym("scopeP"),apps(sym("consEntry"),apps(sym("entry"),p(0),sym("pTag")),p(1))),yes));
 rs.push_back(rule(apps(sym("scopeP"),apps(sym("consEntry"),apps(sym("entry"),p(0),sym("buttonTag")),p(1))),no));rs.push_back(rule(apps(sym("scopeP"),apps(sym("consEntry"),apps(sym("entry"),p(0),sym("htmlTag")),p(1))),no));
 rs.push_back(rule(apps(sym("scopeP"),apps(sym("consEntry"),apps(sym("entry"),p(0),p(1)),p(2))),apps(sym("scopeP"),p(2))));
 rs.push_back(rule(app(sym("dropP"),sym("nilStack")),sym("nilStack")));rs.push_back(rule(apps(sym("dropP"),apps(sym("consEntry"),apps(sym("entry"),p(0),sym("pTag")),p(1))),p(1)));
 rs.push_back(rule(apps(sym("dropP"),apps(sym("consEntry"),p(0),p(1))),apps(sym("dropP"),p(1))));
 rs.push_back(rule(apps(sym("ifStack"),yes,p(0),p(1)),p(0)));rs.push_back(rule(apps(sym("ifStack"),no,p(0),p(1)),p(1)));
 rs.push_back(rule(apps(sym("popForBlock"),p(0)),apps(sym("ifStack"),app(sym("scopeP"),p(0)),app(sym("dropP"),p(0)),p(0))));
 rs.push_back(rule(app(sym("parentOf"),sym("nilStack")),sym("zero")));rs.push_back(rule(apps(sym("parentOf"),apps(sym("consEntry"),apps(sym("entry"),p(0),p(1)),p(2))),p(0)));
 rs.push_back(rule(apps(sym("appendNode"),sym("nilNodes"),p(0)),apps(sym("consNode"),p(0),sym("nilNodes"))));rs.push_back(rule(apps(sym("appendNode"),apps(sym("consNode"),p(0),p(1)),p(2)),apps(sym("consNode"),p(0),apps(sym("appendNode"),p(1),p(2)))));
 rs.push_back(rule(apps(sym("insert"),p(0),p(1),apps(sym("state"),p(2),p(3),p(4))),apps(sym("state"),apps(sym("appendNode"),p(2),apps(sym("node"),p(4),app(sym("parentOf"),p(3)),p(0),p(1))),apps(sym("consEntry"),apps(sym("entry"),p(4),p(0)),p(3)),app(sym("succ"),p(4)))));
 rs.push_back(rule(apps(sym("stateNodes"),apps(sym("state"),p(0),p(1),p(2))),p(0)));rs.push_back(rule(apps(sym("stateStack"),apps(sym("state"),p(0),p(1),p(2))),p(1)));rs.push_back(rule(apps(sym("stateNext"),apps(sym("state"),p(0),p(1),p(2))),p(2)));
 rs.push_back(rule(app(sym("sem"),p(0)),apps(sym("insert"),app(sym("inputTag"),p(0)),app(sym("inputAttrs"),p(0)),apps(sym("state"),app(sym("stateNodes"),app(sym("inputState"),p(0))),app(sym("popForBlock"),app(sym("stateStack"),app(sym("inputState"),p(0)))),app(sym("stateNext"),app(sym("inputState"),p(0)))))));
 sig["stateNodes"]=fn(state,nodes);sig["stateStack"]=fn(state,stack);sig["stateNext"]=fn(state,nat);
 // Projections are intentionally defined only for the finite fixture constants below.
 std::array<State,4> ss={
  State{{{0,0,Tag::Html,{}},{1,0,Tag::Div,{}},{2,1,Tag::P,{}}},{{2,Tag::P},{1,Tag::Div},{0,Tag::Html}},3},
  State{{{0,0,Tag::Html,{}},{1,0,Tag::P,{}},{2,1,Tag::Button,{}},{3,2,Tag::Div,{}}},{{3,Tag::Div},{2,Tag::Button},{1,Tag::P},{0,Tag::Html}},4},
  State{{{0,0,Tag::Html,{}},{1,0,Tag::Div,{}}},{{1,Tag::Div},{0,Tag::Html}},2},
  State{{{0,0,Tag::Html,{}},{1,0,Tag::P,{}},{2,1,Tag::Div,{}}},{{2,Tag::Div},{1,Tag::P},{0,Tag::Html}},3}};
 std::array<Tag,4> ts={Tag::Div,Tag::Span,Tag::Div,Tag::Span};std::array<Attrs,4> as={Attrs{{0,0}},Attrs{},Attrs{{1,1},{0,1}},Attrs{{1,0}}};
 for(size_t i=0;i<ss.size();++i){Ptr id=n(i),inp=app(sym("inputState"),id);rs.push_back(rule(inp,state_term(ss[i])));rs.push_back(rule(app(sym("inputTag"),id),sym(tn(ts[i]))));rs.push_back(rule(app(sym("inputAttrs"),id),attrs_term(as[i])));State out=cpp_transition(ss[i],ts[i],as[i]);rs.push_back(rule(app(sym("native"),id),state_term(out)));}
 // Compare each normalized semantic state to the C++ transcription via conversion.
 for(size_t i=0;i<ss.size();++i){Ptr x=app(sym("sem"),n(i));auto ty=infer({},x,sig,rs);assert(ty&&conv(*ty,state,rs));assert(conv(nf(x,rs),state_term(cpp_transition(ss[i],ts[i],as[i])),rs));}
 std::println("Blink p-scope block-start + attributed insertion: {} finite states agree in typed λΠ",ss.size());
}
