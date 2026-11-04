#include "../src/lambda_pi.hpp"
#include <cassert>
#include <vector>

using namespace lp;

enum class Char { A, B, C, D, I, V, Hash, DoubleQuote, SingleQuote, LessThan };
enum class Kind { Lower, Upper, RawName, Space, Slash, Equals, Greater, Eof,
                  DoubleQuote, SingleQuote, LessThan };
struct Input { Kind kind; Char value=Char::A; };
enum class State { AttributeName, AfterAttributeName, SelfClosingStartTag,
                   BeforeAttributeValue, Data };
enum class Action { AwaitMore, ConsumeWhitespace, EnterSelfClosing,
                    BeginAttributeValue, EmitToken, ParseErrorReconsumeEof };
struct Result { State state; Action action; bool parse_error=false; std::vector<char> name; };

static char character(Char c) {
    switch(c) {
        case Char::A:return 'a'; case Char::B:return 'b'; case Char::C:return 'c';
        case Char::D:return 'd'; case Char::I:return 'i'; case Char::V:return 'v';
        case Char::Hash:return '#'; case Char::DoubleQuote:return '"';
        case Char::SingleQuote:return '\''; case Char::LessThan:return '<';
    }
    return '?';
}
static const char* char_name(Char c) {
    switch(c) {
        case Char::A:return "aChar"; case Char::B:return "bChar"; case Char::C:return "cChar";
        case Char::D:return "dChar"; case Char::I:return "iChar"; case Char::V:return "vChar";
        case Char::Hash:return "hashChar";
        case Char::DoubleQuote:return "doubleQuoteChar"; case Char::SingleQuote:return "singleQuoteChar";
        case Char::LessThan:return "lessThanChar";
    }
    return "hashChar";
}
static Result cpp_scan(const std::vector<char>& initial,const std::vector<Input>& inputs) {
    Result result{State::AttributeName,Action::AwaitMore,false,initial};
    for(const auto& unit:inputs) {
        switch(unit.kind) {
            case Kind::Lower: case Kind::Upper: case Kind::RawName:
                // ToLowerCaseIfAlpha folds the represented ASCII class; RawName is preserved.
                result.name.push_back(character(unit.value)); break;
            case Kind::Space: result.state=State::AfterAttributeName;
                result.action=Action::ConsumeWhitespace; return result;
            case Kind::Slash: result.state=State::SelfClosingStartTag;
                result.action=Action::EnterSelfClosing; return result;
            case Kind::Equals: result.state=State::BeforeAttributeValue;
                result.action=Action::BeginAttributeValue; return result;
            case Kind::Greater: result.state=State::Data; result.action=Action::EmitToken; return result;
            case Kind::Eof: result.state=State::Data; result.parse_error=true;
                result.action=Action::ParseErrorReconsumeEof; return result;
            case Kind::DoubleQuote: case Kind::SingleQuote: case Kind::LessThan:
                result.name.push_back(character(unit.value));
                result.parse_error=true; break;
        }
    }
    // Exhaustion waits while preserving the accumulator and any parse error.
    return result;
}
static Ptr fn(Ptr a,Ptr b) { return arrow(std::move(a),std::move(b)); }
static Rule rule(Ptr lhs,Ptr rhs) { return {std::move(lhs),std::move(rhs)}; }
static Ptr name_term(const std::vector<char>& chars) {
    Ptr result=sym("nilName");
    for(auto i=chars.rbegin();i!=chars.rend();++i) {
        const char* c=*i=='a'?"aChar":*i=='b'?"bChar":*i=='c'?"cChar":*i=='d'?"dChar":
            *i=='i'?"iChar":*i=='v'?"vChar":*i=='#'?"hashChar":*i=='"'?"doubleQuoteChar":
            *i=='\''?"singleQuoteChar":"lessThanChar";
        result=apps(sym("consName"),sym(c),result);
    }
    return result;
}
static const char* state_name(State state) {
    switch(state) { case State::AttributeName:return "attributeNameState";
        case State::AfterAttributeName:return "afterAttributeNameState";
        case State::SelfClosingStartTag:return "selfClosingStartTagState";
        case State::BeforeAttributeValue:return "beforeAttributeValueState";
        case State::Data:return "dataState"; }
    return "dataState";
}
static const char* action_name(Action action) {
    switch(action) { case Action::AwaitMore:return "awaitMore";
        case Action::ConsumeWhitespace:return "consumeWhitespace";
        case Action::EnterSelfClosing:return "enterSelfClosing";
        case Action::BeginAttributeValue:return "beginAttributeValue";
        case Action::EmitToken:return "emitToken";
        case Action::ParseErrorReconsumeEof:return "parseErrorReconsumeEof"; }
    return "awaitMore";
}
static Ptr result_term(const Result& r) {
    return apps(sym("attributeNameResult"),sym(state_name(r.state)),
                sym(action_name(r.action)),sym(r.parse_error?"true":"false"),name_term(r.name));
}
static Ptr input_term(const Input& in) {
    switch(in.kind) {
        case Kind::Lower:return app(sym("lowerInput"),sym(char_name(in.value)));
        case Kind::Upper:return app(sym("upperInput"),sym(char_name(in.value)));
        case Kind::RawName:return app(sym("rawNameInput"),sym("hashChar"));
        case Kind::Space:return sym("spaceInput"); case Kind::Slash:return sym("slashInput");
        case Kind::Equals:return sym("equalsInput"); case Kind::Greater:return sym("greaterInput");
        case Kind::Eof:return sym("eofInput"); case Kind::DoubleQuote:return sym("doubleQuoteInput");
        case Kind::SingleQuote:return sym("singleQuoteInput"); case Kind::LessThan:return sym("lessThanInput");
    }
    assert(false); return sym("eofInput");
}
static Ptr input_seq_term(const std::vector<Input>& inputs) {
    Ptr result=sym("nilInputSeq");
    for(auto i=inputs.rbegin();i!=inputs.rend();++i)
        result=apps(sym("consInput"),input_term(*i),result);
    return result;
}

int main() {
    Signature sig; std::vector<Rule> rules;
    auto pv=[](size_t i){return pvar(i);};
    const Ptr ch=sym("NameChar"), name=sym("Name"), input=sym("Input"), seq=sym("InputSeq"),
        state=sym("TokenizerState"), action=sym("AttributeNameAction"), result=sym("AttributeNameResult");
    sig["NameChar"]=type_sort();
    for(const char* c:{"aChar","bChar","cChar","dChar","iChar","vChar","hashChar","doubleQuoteChar","singleQuoteChar","lessThanChar"}) sig[c]=ch;
    sig["Name"]=type_sort(); sig["nilName"]=name; sig["consName"]=fn(ch,fn(name,name));
    sig["snocName"]=fn(name,fn(ch,name)); sig["lowercaseAlpha"]=fn(ch,ch);
    sig["Input"]=type_sort(); sig["lowerInput"]=fn(ch,input); sig["upperInput"]=fn(ch,input);
    sig["rawNameInput"]=fn(ch,input);
    for(const char* c:{"spaceInput","slashInput","equalsInput","greaterInput","eofInput",
        "doubleQuoteInput","singleQuoteInput","lessThanInput"}) sig[c]=input;
    sig["InputSeq"]=type_sort(); sig["nilInputSeq"]=seq; sig["consInput"]=fn(input,fn(seq,seq));
    sig["TokenizerState"]=type_sort();
    for(const char* c:{"attributeNameState","afterAttributeNameState","selfClosingStartTagState",
        "beforeAttributeValueState","dataState"}) sig[c]=state;
    sig["AttributeNameAction"]=type_sort();
    for(const char* c:{"awaitMore","consumeWhitespace","enterSelfClosing","beginAttributeValue",
        "emitToken","parseErrorReconsumeEof"}) sig[c]=action;
    sig["AttributeNameResult"]=type_sort();
    sig["Bool"]=type_sort(); sig["true"]=sym("Bool"); sig["false"]=sym("Bool");
    sig["attributeNameResult"]=fn(state,fn(action,fn(sym("Bool"),fn(name,result))));
    sig["scanAttributeName"]=fn(sym("Bool"),fn(name,fn(seq,result)));
    sig["Case"]=type_sort(); sig["nameAt"]=fn(sym("Case"),name); sig["inputAt"]=fn(sym("Case"),seq);
    sig["nativeAt"]=fn(sym("Case"),result); sig["semanticAt"]=fn(sym("Case"),result);

    for(const char* c:{"aChar","bChar","cChar","dChar","iChar","vChar"})
        rules.push_back(rule(app(sym("lowercaseAlpha"),sym(c)),sym(c)));
    rules.push_back(rule(apps(sym("snocName"),sym("nilName"),pv(0)),apps(sym("consName"),pv(0),sym("nilName"))));
    rules.push_back(rule(apps(sym("snocName"),apps(sym("consName"),pv(0),pv(1)),pv(2)),
        apps(sym("consName"),pv(0),apps(sym("snocName"),pv(1),pv(2)))));
    rules.push_back(rule(apps(sym("scanAttributeName"),pv(0),pv(1),sym("nilInputSeq")),
        apps(sym("attributeNameResult"),sym("attributeNameState"),sym("awaitMore"),pv(0),pv(1))));
    for(const char* c:{"aChar","bChar","cChar","dChar","iChar","vChar"}) {
        rules.push_back(rule(apps(sym("scanAttributeName"),pv(0),pv(1),apps(sym("consInput"),app(sym("lowerInput"),sym(c)),pv(2))),
            apps(sym("scanAttributeName"),pv(0),apps(sym("snocName"),pv(1),app(sym("lowercaseAlpha"),sym(c))),pv(2))));
        rules.push_back(rule(apps(sym("scanAttributeName"),pv(0),pv(1),apps(sym("consInput"),app(sym("upperInput"),sym(c)),pv(2))),
            apps(sym("scanAttributeName"),pv(0),apps(sym("snocName"),pv(1),app(sym("lowercaseAlpha"),sym(c))),pv(2))));
    }
    rules.push_back(rule(apps(sym("scanAttributeName"),pv(0),pv(1),apps(sym("consInput"),app(sym("rawNameInput"),sym("hashChar")),pv(2))),
        apps(sym("scanAttributeName"),pv(0),apps(sym("snocName"),pv(1),sym("hashChar")),pv(2))));
    rules.push_back(rule(apps(sym("scanAttributeName"),pv(0),pv(1),apps(sym("consInput"),sym("spaceInput"),pv(2))),
        apps(sym("attributeNameResult"),sym("afterAttributeNameState"),sym("consumeWhitespace"),pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanAttributeName"),pv(0),pv(1),apps(sym("consInput"),sym("slashInput"),pv(2))),
        apps(sym("attributeNameResult"),sym("selfClosingStartTagState"),sym("enterSelfClosing"),pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanAttributeName"),pv(0),pv(1),apps(sym("consInput"),sym("equalsInput"),pv(2))),
        apps(sym("attributeNameResult"),sym("beforeAttributeValueState"),sym("beginAttributeValue"),pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanAttributeName"),pv(0),pv(1),apps(sym("consInput"),sym("greaterInput"),pv(2))),
        apps(sym("attributeNameResult"),sym("dataState"),sym("emitToken"),pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanAttributeName"),pv(0),pv(1),apps(sym("consInput"),sym("eofInput"),pv(2))),
        apps(sym("attributeNameResult"),sym("dataState"),sym("parseErrorReconsumeEof"),sym("true"),pv(1))));
    for(const auto& bad:std::vector<std::pair<const char*,const char*>>{
        {"doubleQuoteInput","doubleQuoteChar"},{"singleQuoteInput","singleQuoteChar"},{"lessThanInput","lessThanChar"}})
        rules.push_back(rule(apps(sym("scanAttributeName"),pv(0),pv(1),apps(sym("consInput"),sym(bad.first),pv(2))),
            apps(sym("scanAttributeName"),sym("true"),apps(sym("snocName"),pv(1),sym(bad.second)),pv(2))));
    rules.push_back(rule(app(sym("semanticAt"),pv(0)),
        apps(sym("scanAttributeName"),sym("false"),app(sym("nameAt"),pv(0)),app(sym("inputAt"),pv(0)))));

    struct Case { std::vector<char> initial; std::vector<Input> input; };
    const std::vector<Case> cases={
        {{'d'},{{Kind::Upper,Char::I},{Kind::Upper,Char::V},{Kind::Equals}}},
        {{'a'},{{Kind::RawName,Char::Hash},{Kind::Space}}},
        {{'b'},{{Kind::Slash}}},
        {{'c'},{{Kind::Greater}}},
        {{'a'},{{Kind::Eof}}},
        {{'a'},{{Kind::DoubleQuote,Char::DoubleQuote}}},
        {{'b'},{{Kind::SingleQuote,Char::SingleQuote}}},
        {{'c'},{{Kind::LessThan,Char::LessThan}}},
        {{'a'},{{Kind::DoubleQuote,Char::DoubleQuote},{Kind::Greater}}},
        {{'b'},{{Kind::LessThan,Char::LessThan},{Kind::Lower,Char::C},{Kind::Space}}},
        {{},{}},
        {{'d'},{{Kind::Upper,Char::I}}},
        {{},{{Kind::Equals}}},
        {{'a'},{{Kind::Lower,Char::B},{Kind::RawName,Char::Hash},{Kind::Equals}}}};
    for(size_t i=0;i<cases.size();++i) {
        const auto id="attributeNameCase"+std::to_string(i); sig[id]=sym("Case");
        const Result native=cpp_scan(cases[i].initial,cases[i].input);
        rules.push_back(rule(app(sym("nameAt"),sym(id)),name_term(cases[i].initial)));
        rules.push_back(rule(app(sym("inputAt"),sym(id)),input_seq_term(cases[i].input)));
        rules.push_back(rule(app(sym("nativeAt"),sym(id)),result_term(native)));
        const auto expected_type=infer({},app(sym("nativeAt"),sym(id)),sig,rules);
        const auto actual_type=infer({},app(sym("semanticAt"),sym(id)),sig,rules);
        assert(expected_type&&actual_type&&conv(*expected_type,result,rules)&&conv(*actual_type,result,rules));
        if(!conv(nf(app(sym("nativeAt"),sym(id)),rules),nf(app(sym("semanticAt"),sym(id)),rules),rules)) {
            std::println("failed case {} native={} semantic={}",i,
                to_string(nf(app(sym("nativeAt"),sym(id)),rules)),
                to_string(nf(app(sym("semanticAt"),sym(id)),rules)));
            assert(false);
        }
    }
    std::println("Blink tokenizer AttributeName scan: {} finite accumulator/delimiter sequences agree",cases.size());
    std::println("Scope: attribute-name accumulation and selected delimiters; duplicate attributes, range tracking, null preprocessing, and values are omitted.");
}
