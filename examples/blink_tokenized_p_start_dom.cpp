#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <cctype>
#include <string>
#include <vector>
using namespace lp;

enum class Tag { P, Div, Button, Html, Span };
static const char* tn(Tag t){static constexpr std::array names={"pTag","divTag","buttonTag","htmlTag","spanTag"};return names[static_cast<size_t>(t)];}
struct Attribute { std::string name,value; };
struct Token { Tag tag; std::vector<Attribute> attrs; };
struct Entry { unsigned id; Tag tag; };
struct Node { unsigned id,parent; Tag tag; std::vector<Attribute> attrs; };
struct State { std::vector<Node> dom; std::vector<Entry> stack; unsigned next; };
static Token cpp_tokenize(const std::string& in){
  assert(in.rfind("<p",0)==0);size_t i=2;Token t{Tag::P,{}};
  while(i<in.size()){while(i<in.size()&&std::isspace(static_cast<unsigned char>(in[i])))++i;if(i==in.size())break;if(in[i]=='>'){++i;assert(i==in.size());return t;}
    size_t b=i;while(i<in.size()&&in[i]!='='&&!std::isspace(static_cast<unsigned char>(in[i]))&&in[i]!='>')++i;assert(i>b&&i<in.size()&&in[i]=='=');std::string name=in.substr(b,i-b);++i;assert(i<in.size());char q=0;if(in[i]=='"'||in[i]=='\'')q=in[i++];std::string value;
    while(i<in.size()){if(q&&in[i]==q){++i;break;}if(!q&&(std::isspace(static_cast<unsigned char>(in[i]))||in[i]=='>'))break;if(in.compare(i,5,"&amp;")==0){value.push_back('&');i+=5;}else value.push_back(in[i++]);}
    t.attrs.push_back({std::move(name),std::move(value)});
  }assert(false&&"unterminated start tag");return t;
}
static bool in_scope_p(const std::vector<Entry>& s){for(auto e:s){if(e.tag==Tag::P)return true;if(e.tag==Tag::Button||e.tag==Tag::Html)return false;}return false;}
static State cpp_insert_p(State s,const Token& t){assert(t.tag==Tag::P);if(in_scope_p(s.stack)){size_t i=0;while(i<s.stack.size()&&s.stack[i].tag!=Tag::P)++i;if(i<s.stack.size())s.stack.erase(s.stack.begin(),s.stack.begin()+static_cast<std::ptrdiff_t>(i+1));}unsigned parent=s.stack.empty()?0:s.stack.front().id;unsigned id=s.next++;s.dom.push_back({id,parent,t.tag,t.attrs});s.stack.insert(s.stack.begin(),{id,t.tag});return s;}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr n(unsigned x){Ptr r=sym("z");while(x--)r=app(sym("s"),r);return r;}
static Ptr c(unsigned char x){return sym("c"+std::to_string(x));}
static Ptr text_term(const std::string& s){Ptr r=sym("nilText");for(auto i=s.rbegin();i!=s.rend();++i)r=apps(sym("consText"),c((unsigned char)*i),r);return r;}
static Ptr attrs_term(const std::vector<Attribute>& as){Ptr r=sym("nilAttrs");for(auto i=as.rbegin();i!=as.rend();++i)r=apps(sym("consAttr"),apps(sym("attr"),text_term(i->name),text_term(i->value)),r);return r;}
static Ptr stack_term(const std::vector<Entry>& s){Ptr r=sym("nilStack");for(auto i=s.rbegin();i!=s.rend();++i)r=apps(sym("consEntry"),apps(sym("entry"),n(i->id),sym(tn(i->tag))),r);return r;}
static Ptr node_term(const Node& x){return apps(sym("node"),n(x.id),n(x.parent),sym(tn(x.tag)),attrs_term(x.attrs));}
static Ptr nodes_term(const std::vector<Node>& xs){Ptr r=sym("nilNodes");for(auto i=xs.rbegin();i!=xs.rend();++i)r=apps(sym("consNode"),node_term(*i),r);return r;}
static Ptr state_term(const State& s){return apps(sym("state"),nodes_term(s.dom),stack_term(s.stack),n(s.next));}
int main(){
 Signature sig;std::vector<Rule> rs;auto p=[](int i){return pvar(i);};
 Ptr Nat=sym("Nat"),Bool=sym("Bool"),Char=sym("Char"),Text=sym("Text"),Attr=sym("Attr"),Attrs=sym("Attrs"),TagT=sym("Tag"),Stack=sym("Stack"),EntryT=sym("Entry"),NodeT=sym("Node"),Nodes=sym("Nodes"),StateT=sym("State"),TokenT=sym("StartTagToken");
 sig["Nat"]=type_sort();sig["z"]=Nat;sig["s"]=fn(Nat,Nat);sig["Bool"]=type_sort();sig["true"]=Bool;sig["false"]=Bool;sig["Char"]=type_sort();
 std::string alphabet="<> =\"'&;abcdefghijklmnopqrstuvwxyz-";for(unsigned char x:alphabet)sig["c"+std::to_string(x)]=Char;
 sig["Text"]=type_sort();sig["nilText"]=Text;sig["consText"]=fn(Char,fn(Text,Text));sig["snocText"]=fn(Text,fn(Char,Text));
 sig["Attr"]=type_sort();sig["attr"]=fn(Text,fn(Text,Attr));sig["Attrs"]=type_sort();sig["nilAttrs"]=Attrs;sig["consAttr"]=fn(Attr,fn(Attrs,Attrs));sig["snocAttr"]=fn(Attrs,fn(Attr,Attrs));
 sig["Tag"]=type_sort();for(Tag t:{Tag::P,Tag::Div,Tag::Button,Tag::Html,Tag::Span})sig[tn(t)]=TagT;
 sig["StartTagToken"]=type_sort();sig["startTagToken"]=fn(TagT,fn(Attrs,TokenT));sig["tokenTag"]=fn(TokenT,TagT);sig["tokenAttrs"]=fn(TokenT,Attrs);sig["tokenizeStartTag"]=fn(Text,TokenT);sig["parseAttrs"]=fn(Text,fn(Attrs,TokenT));sig["readAttrName"]=fn(Text,fn(Text,fn(Attrs,TokenT)));sig["beforeAttrValue"]=fn(Text,fn(Text,fn(Attrs,TokenT)));sig["readDoubleValue"]=fn(Text,fn(Text,fn(Text,fn(Attrs,TokenT))));sig["readSingleValue"]=fn(Text,fn(Text,fn(Text,fn(Attrs,TokenT))));sig["readUnquotedValue"]=fn(Text,fn(Text,fn(Text,fn(Attrs,TokenT))));sig["decodeAmpName"]=fn(Text,Text);
 sig["Stack"]=type_sort();sig["nilStack"]=Stack;sig["Entry"]=type_sort();sig["entry"]=fn(Nat,fn(TagT,EntryT));sig["consEntry"]=fn(EntryT,fn(Stack,Stack));sig["Node"]=type_sort();sig["node"]=fn(Nat,fn(Nat,fn(TagT,fn(Attrs,NodeT))));sig["Nodes"]=type_sort();sig["nilNodes"]=Nodes;sig["consNode"]=fn(NodeT,fn(Nodes,Nodes));sig["State"]=type_sort();sig["state"]=fn(Nodes,fn(Stack,fn(Nat,StateT)));
 sig["scopeP"]=fn(Stack,Bool);sig["dropP"]=fn(Stack,Stack);sig["popForPStart"]=fn(Stack,Stack);sig["parentOf"]=fn(Stack,Nat);sig["appendNode"]=fn(Nodes,fn(NodeT,Nodes));sig["insertP"]=fn(TokenT,fn(StateT,StateT));sig["inputState"]=fn(Nat,StateT);sig["inputSource"]=fn(Nat,Text);sig["sem"]=fn(Nat,StateT);sig["native"]=fn(Nat,StateT);
 auto yes=sym("true"),no=sym("false");
 // Supported tokenizer transcription: <p, ASCII attribute names, quoted/unquoted values, &amp;, and >.
 rs.push_back(R(apps(sym("tokenizeStartTag"),apps(sym("consText"),c('<'),apps(sym("consText"),c('p'),p(0)))),apps(sym("parseAttrs"),p(0),sym("nilAttrs"))));
 rs.push_back(R(apps(sym("parseAttrs"),sym("nilText"),p(0)),apps(sym("startTagToken"),sym("pTag"),p(0))));
 rs.push_back(R(apps(sym("parseAttrs"),apps(sym("consText"),c(' '),p(0)),p(1)),apps(sym("parseAttrs"),p(0),p(1))));
 rs.push_back(R(apps(sym("parseAttrs"),apps(sym("consText"),c('>'),sym("nilText")),p(0)),apps(sym("startTagToken"),sym("pTag"),p(0))));
 rs.push_back(R(apps(sym("parseAttrs"),apps(sym("consText"),p(0),p(1)),p(2)),apps(sym("readAttrName"),p(1),apps(sym("consText"),p(0),sym("nilText")),p(2))));
 rs.push_back(R(apps(sym("readAttrName"),apps(sym("consText"),c('='),p(0)),p(1),p(2)),apps(sym("beforeAttrValue"),p(0),p(1),p(2))));
 rs.push_back(R(apps(sym("readAttrName"),apps(sym("consText"),p(0),p(1)),p(2),p(3)),apps(sym("readAttrName"),p(1),apps(sym("snocText"),p(2),p(0)),p(3))));
 rs.push_back(R(apps(sym("beforeAttrValue"),apps(sym("consText"),c('"'),p(0)),p(1),p(2)),apps(sym("readDoubleValue"),p(0),p(1),sym("nilText"),p(2))));
 rs.push_back(R(apps(sym("beforeAttrValue"),apps(sym("consText"),c('\''),p(0)),p(1),p(2)),apps(sym("readSingleValue"),p(0),p(1),sym("nilText"),p(2))));
 rs.push_back(R(apps(sym("beforeAttrValue"),apps(sym("consText"),p(0),p(1)),p(2),p(3)),apps(sym("readUnquotedValue"),p(1),p(2),apps(sym("consText"),p(0),sym("nilText")),p(3))));
 // Entity value is derived from the typed amp-name decoder result, not injected into the token fixture.
 Ptr ampName=apps(sym("consText"),c('a'),apps(sym("consText"),c('m'),apps(sym("consText"),c('p'),apps(sym("consText"),c(';'),sym("nilText")))));
 for(const char* scanner:{"readDoubleValue","readSingleValue","readUnquotedValue"}){
   const char end=std::string_view(scanner)=="readDoubleValue"?'"':std::string_view(scanner)=="readSingleValue"?'\'':'>';
   if(end=='>')rs.push_back(R(apps(sym(scanner),apps(sym("consText"),c('>'),p(0)),p(1),p(2),p(3)),apps(sym("startTagToken"),sym("pTag"),apps(sym("snocAttr"),p(3),apps(sym("attr"),p(1),p(2))))));
   else rs.push_back(R(apps(sym(scanner),apps(sym("consText"),c(end),p(0)),p(1),p(2),p(3)),apps(sym("parseAttrs"),p(0),apps(sym("snocAttr"),p(3),apps(sym("attr"),p(1),p(2))))));
   Ptr entInput=apps(sym("consText"),c('&'),ampName);
   Ptr afterEntity=apps(sym(scanner),p(0),p(1),apps(sym("appendText"),p(2),app(sym("decodeAmpName"),ampName)),p(3));
   rs.push_back(R(apps(sym(scanner),apps(sym("consText"),c('&'),apps(sym("consText"),c('a'),apps(sym("consText"),c('m'),apps(sym("consText"),c('p'),apps(sym("consText"),c(';'),p(0)))))),p(1),p(2),p(3)),afterEntity));
   if(std::string_view(scanner)=="readUnquotedValue")rs.push_back(R(apps(sym(scanner),apps(sym("consText"),c(' '),p(0)),p(1),p(2),p(3)),apps(sym("parseAttrs"),p(0),apps(sym("snocAttr"),p(3),apps(sym("attr"),p(1),p(2))))));
   rs.push_back(R(apps(sym(scanner),apps(sym("consText"),p(0),p(1)),p(2),p(3),p(4)),apps(sym(scanner),p(1),p(2),apps(sym("snocText"),p(3),p(0)),p(4))));
 }
 rs.push_back(R(apps(sym("decodeAmpName"),ampName),apps(sym("consText"),c('&'),sym("nilText"))));
 rs.push_back(R(apps(sym("appendText"),sym("nilText"),p(0)),p(0)));rs.push_back(R(apps(sym("appendText"),apps(sym("consText"),p(0),p(1)),p(2)),apps(sym("consText"),p(0),apps(sym("appendText"),p(1),p(2)))));
 rs.push_back(R(apps(sym("snocText"),sym("nilText"),p(0)),apps(sym("consText"),p(0),sym("nilText"))));rs.push_back(R(apps(sym("snocText"),apps(sym("consText"),p(0),p(1)),p(2)),apps(sym("consText"),p(0),apps(sym("snocText"),p(1),p(2)))));
 rs.push_back(R(apps(sym("snocAttr"),sym("nilAttrs"),p(0)),apps(sym("consAttr"),p(0),sym("nilAttrs"))));rs.push_back(R(apps(sym("snocAttr"),apps(sym("consAttr"),p(0),p(1)),p(2)),apps(sym("consAttr"),p(0),apps(sym("snocAttr"),p(1),p(2)))));
 rs.push_back(R(app(sym("tokenTag"),apps(sym("startTagToken"),p(0),p(1))),p(0)));rs.push_back(R(app(sym("tokenAttrs"),apps(sym("startTagToken"),p(0),p(1))),p(1)));
 // p-start scope pop followed by token-derived attributed element insertion.
 rs.push_back(R(apps(sym("scopeP"),sym("nilStack")),no));rs.push_back(R(apps(sym("scopeP"),apps(sym("consEntry"),apps(sym("entry"),p(0),sym("pTag")),p(1))),yes));rs.push_back(R(apps(sym("scopeP"),apps(sym("consEntry"),apps(sym("entry"),p(0),sym("buttonTag")),p(1))),no));rs.push_back(R(apps(sym("scopeP"),apps(sym("consEntry"),apps(sym("entry"),p(0),sym("htmlTag")),p(1))),no));rs.push_back(R(apps(sym("scopeP"),apps(sym("consEntry"),apps(sym("entry"),p(0),p(1)),p(2))),apps(sym("scopeP"),p(2))));
 rs.push_back(R(apps(sym("dropP"),sym("nilStack")),sym("nilStack")));rs.push_back(R(apps(sym("dropP"),apps(sym("consEntry"),apps(sym("entry"),p(0),sym("pTag")),p(1))),p(1)));rs.push_back(R(apps(sym("dropP"),apps(sym("consEntry"),p(0),p(1))),apps(sym("dropP"),p(1))));rs.push_back(R(apps(sym("popForPStart"),p(0)),apps(sym("ifStack"),app(sym("scopeP"),p(0)),app(sym("dropP"),p(0)),p(0))));rs.push_back(R(apps(sym("ifStack"),yes,p(0),p(1)),p(0)));rs.push_back(R(apps(sym("ifStack"),no,p(0),p(1)),p(1)));
 rs.push_back(R(app(sym("parentOf"),sym("nilStack")),sym("z")));rs.push_back(R(apps(sym("parentOf"),apps(sym("consEntry"),apps(sym("entry"),p(0),p(1)),p(2))),p(0)));rs.push_back(R(apps(sym("appendNode"),sym("nilNodes"),p(0)),apps(sym("consNode"),p(0),sym("nilNodes"))));rs.push_back(R(apps(sym("appendNode"),apps(sym("consNode"),p(0),p(1)),p(2)),apps(sym("consNode"),p(0),apps(sym("appendNode"),p(1),p(2)))));
 rs.push_back(R(apps(sym("insertP"),apps(sym("startTagToken"),sym("pTag"),p(0)),apps(sym("state"),p(1),p(2),p(3))),apps(sym("state"),apps(sym("appendNode"),p(1),apps(sym("node"),p(3),app(sym("parentOf"),p(2)),sym("pTag"),p(0))),apps(sym("consEntry"),apps(sym("entry"),p(3),sym("pTag")),p(2)),app(sym("s"),p(3)))));
 sig["isInputState"]=fn(Nat,StateT);sig["stateNodes"]=fn(StateT,Nodes);sig["stateStack"]=fn(StateT,Stack);sig["stateNext"]=fn(StateT,Nat);rs.push_back(R(app(sym("stateNodes"),apps(sym("state"),p(0),p(1),p(2))),p(0)));rs.push_back(R(app(sym("stateStack"),apps(sym("state"),p(0),p(1),p(2))),p(1)));rs.push_back(R(app(sym("stateNext"),apps(sym("state"),p(0),p(1),p(2))),p(2)));
 rs.push_back(R(app(sym("sem"),p(0)),apps(sym("insertP"),app(sym("tokenizeStartTag"),app(sym("inputSource"),p(0))),apps(sym("state"),app(sym("stateNodes"),app(sym("inputState"),p(0))),app(sym("popForPStart"),app(sym("stateStack"),app(sym("inputState"),p(0)))),app(sym("stateNext"),app(sym("inputState"),p(0)))))));
 std::vector<std::string> inputs={"<p>","<p id=\"alpha&amp;beta\" class='beta'>","<p class='beta' id=\"alpha\">"};
 std::array<State,3> states={State{{{0,0,Tag::Html,{}},{1,0,Tag::Div,{}},{2,1,Tag::P,{}}},{{2,Tag::P},{1,Tag::Div},{0,Tag::Html}},3},State{{{0,0,Tag::Html,{}},{1,0,Tag::Div,{}}},{{1,Tag::Div},{0,Tag::Html}},2},State{{{0,0,Tag::Html,{}},{1,0,Tag::P,{}},{2,1,Tag::Button,{}},{3,2,Tag::Span,{}}},{{3,Tag::Span},{2,Tag::Button},{1,Tag::P},{0,Tag::Html}},4}};
 for(size_t i=0;i<inputs.size();++i){Ptr id=n(i);Token token=cpp_tokenize(inputs[i]);State out=cpp_insert_p(states[i],token);rs.push_back(R(app(sym("inputSource"),id),text_term(inputs[i])));rs.push_back(R(app(sym("inputState"),id),state_term(states[i])));rs.push_back(R(app(sym("isInputState"),id),state_term(states[i])));rs.push_back(R(app(sym("native"),id),state_term(out)));Ptr token_actual=nf(app(sym("tokenizeStartTag"),app(sym("inputSource"),id)),rs);Ptr token_expected=apps(sym("startTagToken"),sym(tn(token.tag)),attrs_term(token.attrs));Ptr term=app(sym("sem"),id);auto ty=infer({},term,sig,rs);if(!conv(token_actual,token_expected,rs)||!ty||!conv(*ty,StateT,rs)||!conv(nf(term,rs),state_term(out),rs)){std::println("token-to-DOM comparison failed case {} tokenActual={} tokenExpected={} stateActual={}",i,to_string(token_actual),to_string(token_expected),to_string(nf(term,rs)));return 2;}}
 std::println("Typed tokenizer StartTag → p-scope/DOM insertion: {} finite raw tags agree; attrs are scanned from input and carried in source order",inputs.size());std::println("Scope: bounded <p> tokenizer transcription supports ASCII lowercase names, ordinary quoted/unquoted text, and &amp; values. DOM node/document identity, duplicate normalization, custom elements, sanitizer/script handling, and full parse-error/reconsume behavior are abstracted.");
}
