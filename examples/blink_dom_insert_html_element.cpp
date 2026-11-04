#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <vector>

using namespace lp;

enum class Tag { Html, P, Div, Span };
struct Attribute { unsigned name; unsigned value; };
using Attrs = std::vector<Attribute>;
struct Token { Tag tag; Attrs attrs; };
struct Node { unsigned id; unsigned parent; Tag tag; Attrs attrs; };
struct ParserState { std::vector<Node> dom; std::vector<unsigned> open_stack; unsigned next_id; };

static const char* tag_name(Tag t) {
    static constexpr std::array names={"htmlTag","pTag","divTag","spanTag"};
    return names[static_cast<size_t>(t)];
}
static ParserState cpp_insert_html_element(Token token, ParserState state) {
    // The normal AttachOrFosterParent path captures the insertion location
    // before pushing the new stack item, then attaches to that saved parent.
    const unsigned parent=state.open_stack.empty()?0:state.open_stack.front();
    const unsigned fresh=state.next_id;
    state.open_stack.insert(state.open_stack.begin(),fresh);
    state.dom.push_back({fresh,parent,token.tag,token.attrs});
    ++state.next_id;
    return state;
}

static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr t=sym("zero");while(n--)t=app(sym("succ"),t);return t;}
static Ptr tag_term(Tag t){return sym(tag_name(t));}
static Ptr attr_term(Attribute a){return apps(sym("attribute"),sym(a.name==0?"className":"idName"),sym(a.value==0?"alphaValue":"betaValue"));}
static Ptr attrs_term(const Attrs& attrs){Ptr result=sym("nilAttrs");for(auto i=attrs.rbegin();i!=attrs.rend();++i)result=apps(sym("consAttr"),attr_term(*i),result);return result;}
static Ptr node_term(Node n){return apps(sym("domNode"),nat(n.id),nat(n.parent),tag_term(n.tag),attrs_term(n.attrs));}
static Ptr node_list_term(const std::vector<Node>& nodes){
    Ptr result=sym("nilNodes");
    for(auto i=nodes.rbegin();i!=nodes.rend();++i)
        result=apps(sym("consNode"),node_term(*i),result);
    return result;
}
static Ptr id_list_term(const std::vector<unsigned>& ids){
    Ptr result=sym("nilIds");
    for(auto i=ids.rbegin();i!=ids.rend();++i)
        result=apps(sym("consId"),nat(*i),result);
    return result;
}
static Ptr state_term(const ParserState& s){
    return apps(sym("parserState"),node_list_term(s.dom),id_list_term(s.open_stack),nat(s.next_id));
}
static Ptr case_id(size_t i){return nat(static_cast<unsigned>(i));}

int main(){
    Signature sig;std::vector<Rule> rules;auto p=[](size_t i){return pvar(i);};
    const Ptr natty=sym("Nat"),tag=sym("Tag"),node=sym("DomNode");
    const Ptr dom=sym("DomNodes"),ids=sym("OpenIds"),state=sym("ParserState");
    const Ptr bit=sym("Bool"),yes=sym("true"),no=sym("false");
    sig["Nat"]=type_sort();sig["zero"]=natty;sig["succ"]=fn(natty,natty);
    sig["Tag"]=type_sort();for(Tag t:{Tag::Html,Tag::P,Tag::Div,Tag::Span})sig[tag_name(t)]=tag;
    const Ptr attr=sym("Attribute"), attrs=sym("Attributes");
    sig["AttributeName"]=type_sort();sig["className"]=sym("AttributeName");sig["idName"]=sym("AttributeName");
    sig["AttributeValue"]=type_sort();sig["alphaValue"]=sym("AttributeValue");sig["betaValue"]=sym("AttributeValue");
    sig["Attribute"]=type_sort();sig["attribute"]=fn(sym("AttributeName"),fn(sym("AttributeValue"),attr));
    sig["Attributes"]=type_sort();sig["nilAttrs"]=attrs;sig["consAttr"]=fn(attr,fn(attrs,attrs));
    sig["DomNode"]=type_sort();sig["domNode"]=fn(natty,fn(natty,fn(tag,fn(attrs,node))));
    sig["DomNodes"]=type_sort();sig["nilNodes"]=dom;sig["consNode"]=fn(node,fn(dom,dom));
    sig["OpenIds"]=type_sort();sig["nilIds"]=ids;sig["consId"]=fn(natty,fn(ids,ids));
    sig["ParserState"]=type_sort();sig["parserState"]=fn(dom,fn(ids,fn(natty,state)));
    sig["parentOf"]=fn(ids,natty);sig["appendNode"]=fn(dom,fn(node,dom));
    sig["insertHtmlElement"]=fn(tag,fn(attrs,fn(state,state)));
    sig["inputState"]=fn(natty,state);sig["inputTag"]=fn(natty,tag);sig["inputAttrs"]=fn(natty,attrs);
    sig["nativeResult"]=fn(natty,state);sig["semanticResult"]=fn(natty,state);
    sig["eqNat"]=fn(natty,fn(natty,bit));sig["eqTag"]=fn(tag,fn(tag,bit));
    sig["eqName"]=fn(sym("AttributeName"),fn(sym("AttributeName"),bit));
    sig["eqValue"]=fn(sym("AttributeValue"),fn(sym("AttributeValue"),bit));
    sig["eqAttr"]=fn(attr,fn(attr,bit));sig["eqAttrs"]=fn(attrs,fn(attrs,bit));
    sig["eqNode"]=fn(node,fn(node,bit));sig["eqNodes"]=fn(dom,fn(dom,bit));
    sig["eqIds"]=fn(ids,fn(ids,bit));sig["eqState"]=fn(state,fn(state,bit));
    sig["andB"]=fn(bit,fn(bit,bit));sig["checkAt"]=fn(natty,bit);

    rules.push_back(rule(app(sym("parentOf"),sym("nilIds")),sym("zero")));
    rules.push_back(rule(app(sym("parentOf"),apps(sym("consId"),p(0),p(1))),p(0)));
    rules.push_back(rule(apps(sym("appendNode"),sym("nilNodes"),p(0)),
                         apps(sym("consNode"),p(0),sym("nilNodes"))));
    rules.push_back(rule(apps(sym("appendNode"),apps(sym("consNode"),p(0),p(1)),p(2)),
                         apps(sym("consNode"),p(0),apps(sym("appendNode"),p(1),p(2)))));
    rules.push_back(rule(apps(sym("insertHtmlElement"),p(0),p(4),
                              apps(sym("parserState"),p(1),p(2),p(3))),
        apps(sym("parserState"),
             apps(sym("appendNode"),p(1),apps(sym("domNode"),p(3),app(sym("parentOf"),p(2)),p(0),p(4))),
             apps(sym("consId"),p(3),p(2)),app(sym("succ"),p(3)))));

    rules.push_back(rule(apps(sym("andB"),yes,yes),yes));
    rules.push_back(rule(apps(sym("andB"),yes,no),no));
    rules.push_back(rule(apps(sym("andB"),no,p(0)),no));
    rules.push_back(rule(apps(sym("eqNat"),sym("zero"),sym("zero")),yes));
    rules.push_back(rule(apps(sym("eqNat"),sym("zero"),app(sym("succ"),p(0))),no));
    rules.push_back(rule(apps(sym("eqNat"),app(sym("succ"),p(0)),sym("zero")),no));
    rules.push_back(rule(apps(sym("eqNat"),app(sym("succ"),p(0)),app(sym("succ"),p(1))),
                         apps(sym("eqNat"),p(0),p(1))));
    for(Tag a:{Tag::Html,Tag::P,Tag::Div,Tag::Span})for(Tag b:{Tag::Html,Tag::P,Tag::Div,Tag::Span})
        rules.push_back(rule(apps(sym("eqTag"),tag_term(a),tag_term(b)),sym(a==b?"true":"false")));
    for(const char* a:{"className","idName"})for(const char* b:{"className","idName"})
        rules.push_back(rule(apps(sym("eqName"),sym(a),sym(b)),sym(std::string(a)==b?"true":"false")));
    for(const char* a:{"alphaValue","betaValue"})for(const char* b:{"alphaValue","betaValue"})
        rules.push_back(rule(apps(sym("eqValue"),sym(a),sym(b)),sym(std::string(a)==b?"true":"false")));
    rules.push_back(rule(apps(sym("eqAttr"),apps(sym("attribute"),p(0),p(1)),apps(sym("attribute"),p(2),p(3))),
        apps(sym("andB"),apps(sym("eqName"),p(0),p(2)),apps(sym("eqValue"),p(1),p(3)))));
    rules.push_back(rule(apps(sym("eqAttrs"),sym("nilAttrs"),sym("nilAttrs")),yes));
    rules.push_back(rule(apps(sym("eqAttrs"),sym("nilAttrs"),apps(sym("consAttr"),p(0),p(1))),no));
    rules.push_back(rule(apps(sym("eqAttrs"),apps(sym("consAttr"),p(0),p(1)),sym("nilAttrs")),no));
    rules.push_back(rule(apps(sym("eqAttrs"),apps(sym("consAttr"),p(0),p(1)),apps(sym("consAttr"),p(2),p(3))),
        apps(sym("andB"),apps(sym("eqAttr"),p(0),p(2)),apps(sym("eqAttrs"),p(1),p(3)))));
    rules.push_back(rule(apps(sym("eqNode"),
        apps(sym("domNode"),p(0),p(1),p(2),p(6)),apps(sym("domNode"),p(3),p(4),p(5),p(7))),
        apps(sym("andB"),apps(sym("eqNat"),p(0),p(3)),
            apps(sym("andB"),apps(sym("eqNat"),p(1),p(4)),
                apps(sym("andB"),apps(sym("eqTag"),p(2),p(5)),apps(sym("eqAttrs"),p(6),p(7)))))));
    rules.push_back(rule(apps(sym("eqNodes"),sym("nilNodes"),sym("nilNodes")),yes));
    rules.push_back(rule(apps(sym("eqNodes"),sym("nilNodes"),apps(sym("consNode"),p(0),p(1))),no));
    rules.push_back(rule(apps(sym("eqNodes"),apps(sym("consNode"),p(0),p(1)),sym("nilNodes")),no));
    rules.push_back(rule(apps(sym("eqNodes"),apps(sym("consNode"),p(0),p(1)),
                              apps(sym("consNode"),p(2),p(3))),
        apps(sym("andB"),apps(sym("eqNode"),p(0),p(2)),apps(sym("eqNodes"),p(1),p(3)))));
    rules.push_back(rule(apps(sym("eqIds"),sym("nilIds"),sym("nilIds")),yes));
    rules.push_back(rule(apps(sym("eqIds"),sym("nilIds"),apps(sym("consId"),p(0),p(1))),no));
    rules.push_back(rule(apps(sym("eqIds"),apps(sym("consId"),p(0),p(1)),sym("nilIds")),no));
    rules.push_back(rule(apps(sym("eqIds"),apps(sym("consId"),p(0),p(1)),
                              apps(sym("consId"),p(2),p(3))),
        apps(sym("andB"),apps(sym("eqNat"),p(0),p(2)),apps(sym("eqIds"),p(1),p(3)))));
    rules.push_back(rule(apps(sym("eqState"),
        apps(sym("parserState"),p(0),p(1),p(2)),apps(sym("parserState"),p(3),p(4),p(5))),
        apps(sym("andB"),apps(sym("eqNodes"),p(0),p(3)),
            apps(sym("andB"),apps(sym("eqIds"),p(1),p(4)),apps(sym("eqNat"),p(2),p(5))))));
    rules.push_back(rule(app(sym("semanticResult"),p(0)),
        apps(sym("insertHtmlElement"),app(sym("inputTag"),p(0)),app(sym("inputAttrs"),p(0)),app(sym("inputState"),p(0)))));
    rules.push_back(rule(app(sym("checkAt"),p(0)),
        apps(sym("eqState"),app(sym("nativeResult"),p(0)),app(sym("semanticResult"),p(0)))));

    const ParserState s0{{{0,0,Tag::Html,{}}},{0},1};
    const ParserState s1{{{0,0,Tag::Html,{}},{1,0,Tag::Div,{}},{2,1,Tag::P,{}}},{2,1,0},3};
    const ParserState s2{{{0,0,Tag::Html,{}},{1,0,Tag::Div,{}},{2,0,Tag::Span,{}}},{1,0},3};
    const ParserState s3{{{0,0,Tag::Html,{}},{1,0,Tag::Div,{}}},{},2};
    const std::array<ParserState,4> inputs={s0,s1,s2,s3};
    const std::array<Token,4> tokens={Token{Tag::P,{}},Token{Tag::Span,{{0,0},{1,1}}},Token{Tag::Div,{{1,0},{0,1}}},Token{Tag::Html,{{0,1}}}};
    for(size_t i=0;i<inputs.size();++i){
        const Ptr id=case_id(i);
        const ParserState native=cpp_insert_html_element(tokens[i],inputs[i]);
        rules.push_back(rule(app(sym("inputState"),id),state_term(inputs[i])));
        rules.push_back(rule(app(sym("inputTag"),id),tag_term(tokens[i].tag)));
        rules.push_back(rule(app(sym("inputAttrs"),id),attrs_term(tokens[i].attrs)));
        rules.push_back(rule(app(sym("nativeResult"),id),state_term(native)));
        const Ptr check=app(sym("checkAt"),id);
        const auto type=infer({},check,sig,rules);
        assert(type&&conv(*type,bit,rules));
        assert(conv(nf(check,rules),yes,rules));
    }
    std::println("Blink normal HTML insertion: {} finite DOM/stack states agree between C++ and typed λΠ",inputs.size());
    std::println("Scope: ordered finite attribute pairs are copied to new node before normal append; excludes custom/sanitizer paths and foster parenting.");
}
