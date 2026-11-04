#include "../src/lambda_pi.hpp"
#include <cassert>
#include <vector>

using namespace lp;

enum class Letter { A, B, C, D, I, V, Hash };
enum class Kind { Lower, Upper, Raw, Space, Slash, Greater, Eof };
struct Input { Kind kind; Letter value=Letter::A; };
enum class State { TagName, BeforeAttributeName, SelfClosingStartTag, Data };
enum class Action { AwaitMore, ConsumeWhitespace, EnterSelfClosing, EmitToken,
                    ParseErrorReconsumeEof };
struct Result { State state; Action action; std::vector<char> name; };

static char lower_char(Letter c) {
    switch(c) { case Letter::A:return 'a'; case Letter::B:return 'b';
        case Letter::C:return 'c'; case Letter::D:return 'd'; case Letter::I:return 'i';
        case Letter::V:return 'v'; case Letter::Hash:return '#'; }
    return '?';
}
static const char* char_name(Letter c) {
    switch(c) { case Letter::A:return "aChar"; case Letter::B:return "bChar";
        case Letter::C:return "cChar"; case Letter::D:return "dChar";
        case Letter::I:return "iChar"; case Letter::V:return "vChar";
        case Letter::Hash:return "hashChar"; }
    return "hashChar";
}
static Result cpp_scan(const std::vector<Input>& input) {
    Result out{State::TagName,Action::AwaitMore,{}};
    for(const auto& unit:input) {
        switch(unit.kind) {
            case Kind::Lower: case Kind::Upper: case Kind::Raw:
                // ToLowerCaseIfAlpha folds the represented uppercase letters.
                out.name.push_back(lower_char(unit.value)); break;
            case Kind::Space: out.state=State::BeforeAttributeName;
                out.action=Action::ConsumeWhitespace; return out;
            case Kind::Slash: out.state=State::SelfClosingStartTag;
                out.action=Action::EnterSelfClosing; return out;
            case Kind::Greater: out.state=State::Data;
                out.action=Action::EmitToken; return out;
            case Kind::Eof: out.state=State::Data;
                out.action=Action::ParseErrorReconsumeEof; return out;
        }
    }
    return out; // chunk ended while still in TagName
}
static Ptr fn(Ptr a,Ptr b) { return arrow(std::move(a),std::move(b)); }
static Rule rule(Ptr lhs,Ptr rhs) { return {std::move(lhs),std::move(rhs)}; }
static Ptr name_term(const std::vector<char>& chars) {
    Ptr result=sym("nilName");
    for(auto i=chars.rbegin();i!=chars.rend();++i) {
        const char* name_char=*i=='a'?"aChar":*i=='b'?"bChar":*i=='c'?"cChar":
            *i=='d'?"dChar":*i=='i'?"iChar":*i=='v'?"vChar":"hashChar";
        result=apps(sym("consName"),sym(name_char),result);
    }
    return result;
}
static Ptr state_term(State s) {
    return sym(s==State::TagName?"tagNameState":s==State::BeforeAttributeName?"beforeAttributeNameState":
        s==State::SelfClosingStartTag?"selfClosingStartTagState":"dataState");
}
static Ptr action_term(Action a) {
    return sym(a==Action::AwaitMore?"awaitMore":a==Action::ConsumeWhitespace?"consumeWhitespace":
        a==Action::EnterSelfClosing?"enterSelfClosing":a==Action::EmitToken?"emitToken":
        "parseErrorReconsumeEof");
}
static Ptr result_term(const Result& r) {
    return apps(sym("tagNameResult"),state_term(r.state),action_term(r.action),name_term(r.name));
}
static Ptr input_term(const Input& unit) {
    switch(unit.kind) {
        case Kind::Lower: return app(sym("lowerInput"),sym(char_name(unit.value)));
        case Kind::Upper: return app(sym("upperInput"),sym(char_name(unit.value)));
        case Kind::Raw: return app(sym("rawInput"),sym("hashChar"));
        case Kind::Space: return sym("spaceInput");
        case Kind::Slash: return sym("slashInput");
        case Kind::Greater: return sym("greaterInput");
        case Kind::Eof: return sym("eofInput");
    }
    assert(false); return sym("eofInput");
}
static Ptr inputs_term(const std::vector<Input>& input) {
    Ptr result=sym("nilInputSeq");
    for(auto i=input.rbegin();i!=input.rend();++i)
        result=apps(sym("consInput"),input_term(*i),result);
    return result;
}

int main() {
    Signature sig; std::vector<Rule> rules;
    auto pv=[](size_t i){return pvar(i);};
    const Ptr bit=sym("Bool"), yes=sym("true");
    const Ptr name_char=sym("NameChar"), name=sym("Name"), input=sym("Input"),
        input_seq=sym("InputSeq"), state=sym("TokenizerState"), action=sym("TagNameAction"),
        result=sym("TagNameResult");
    sig["Bool"]=type_sort(); sig["true"]=bit; sig["false"]=bit;
    sig["NameChar"]=type_sort();
    for(const char* c:{"aChar","bChar","cChar","dChar","iChar","vChar","hashChar"}) sig[c]=name_char;
    sig["Name"]=type_sort(); sig["nilName"]=name;
    sig["consName"]=fn(name_char,fn(name,name)); sig["snocName"]=fn(name,fn(name_char,name));
    sig["Input"]=type_sort(); sig["lowerInput"]=fn(name_char,input);
    sig["upperInput"]=fn(name_char,input); sig["rawInput"]=fn(name_char,input);
    for(const char* c:{"spaceInput","slashInput","greaterInput","eofInput"}) sig[c]=input;
    sig["InputSeq"]=type_sort(); sig["nilInputSeq"]=input_seq;
    sig["consInput"]=fn(input,fn(input_seq,input_seq));
    sig["TokenizerState"]=type_sort();
    for(const char* c:{"tagNameState","beforeAttributeNameState","selfClosingStartTagState","dataState"}) sig[c]=state;
    sig["TagNameAction"]=type_sort();
    for(const char* c:{"awaitMore","consumeWhitespace","enterSelfClosing","emitToken","parseErrorReconsumeEof"}) sig[c]=action;
    sig["TagNameResult"]=type_sort();
    sig["tagNameResult"]=fn(state,fn(action,fn(name,result)));
    sig["scanTagName"]=fn(name,fn(input_seq,result));
    sig["lowercaseAlpha"]=fn(name_char,name_char);
    sig["nativeAt"]=fn(sym("Case"),result); sig["semanticAt"]=fn(sym("Case"),result);
    sig["inputAt"]=fn(sym("Case"),input_seq); sig["Case"]=type_sort();

    for(const char* c:{"aChar","bChar","cChar","dChar","iChar","vChar","hashChar"})
        rules.push_back(rule(app(sym("lowercaseAlpha"),sym(c)),sym(c)));
    rules.push_back(rule(apps(sym("snocName"),sym("nilName"),pv(0)),apps(sym("consName"),pv(0),sym("nilName"))));
    rules.push_back(rule(apps(sym("snocName"),apps(sym("consName"),pv(0),pv(1)),pv(2)),
        apps(sym("consName"),pv(0),apps(sym("snocName"),pv(1),pv(2)))));
    rules.push_back(rule(apps(sym("scanTagName"),pv(0),sym("nilInputSeq")),
        apps(sym("tagNameResult"),sym("tagNameState"),sym("awaitMore"),pv(0))));
    // The letter payload is a canonical case-fold class; lower and upper
    // inputs both pass through the explicit normalization operator.
    for(const char* c:{"aChar","bChar","cChar","dChar","iChar","vChar"}) {
        rules.push_back(rule(apps(sym("scanTagName"),pv(0),apps(sym("consInput"),app(sym("lowerInput"),sym(c)),pv(1))),
            apps(sym("scanTagName"),apps(sym("snocName"),pv(0),app(sym("lowercaseAlpha"),sym(c))),pv(1))));
        rules.push_back(rule(apps(sym("scanTagName"),pv(0),apps(sym("consInput"),app(sym("upperInput"),sym(c)),pv(1))),
            apps(sym("scanTagName"),apps(sym("snocName"),pv(0),app(sym("lowercaseAlpha"),sym(c))),pv(1))));
    }
    rules.push_back(rule(apps(sym("scanTagName"),pv(0),apps(sym("consInput"),app(sym("rawInput"),sym("hashChar")),pv(1))),
        apps(sym("scanTagName"),apps(sym("snocName"),pv(0),sym("hashChar")),pv(1))));
    rules.push_back(rule(apps(sym("scanTagName"),pv(0),apps(sym("consInput"),sym("spaceInput"),pv(1))),
        apps(sym("tagNameResult"),sym("beforeAttributeNameState"),sym("consumeWhitespace"),pv(0))));
    rules.push_back(rule(apps(sym("scanTagName"),pv(0),apps(sym("consInput"),sym("slashInput"),pv(1))),
        apps(sym("tagNameResult"),sym("selfClosingStartTagState"),sym("enterSelfClosing"),pv(0))));
    rules.push_back(rule(apps(sym("scanTagName"),pv(0),apps(sym("consInput"),sym("greaterInput"),pv(1))),
        apps(sym("tagNameResult"),sym("dataState"),sym("emitToken"),pv(0))));
    rules.push_back(rule(apps(sym("scanTagName"),pv(0),apps(sym("consInput"),sym("eofInput"),pv(1))),
        apps(sym("tagNameResult"),sym("dataState"),sym("parseErrorReconsumeEof"),pv(0))));
    rules.push_back(rule(app(sym("semanticAt"),pv(0)),
        apps(sym("scanTagName"),sym("nilName"),app(sym("inputAt"),pv(0)))));

    const std::vector<std::vector<Input>> cases={
        {{Kind::Upper,Letter::D},{Kind::Lower,Letter::I},{Kind::Upper,Letter::V},{Kind::Greater}}, // DiV -> div
        {{Kind::Upper,Letter::A},{Kind::Lower,Letter::B},{Kind::Space}},
        {{Kind::Upper,Letter::C},{Kind::Slash}},
        {{Kind::Lower,Letter::A},{Kind::Upper,Letter::B},{Kind::Eof}},
        {{Kind::Upper,Letter::D}}, // chunk boundary retains accumulated name
        {},
        {{Kind::Raw,Letter::Hash},{Kind::Upper,Letter::A},{Kind::Greater}},
        {{Kind::Upper,Letter::A},{Kind::Lower,Letter::B},{Kind::Raw,Letter::Hash},{Kind::Slash}},
        {{Kind::Greater}}};
    for(size_t i=0;i<cases.size();++i) {
        const auto case_name="tagNameCase"+std::to_string(i); sig[case_name]=sym("Case");
        const Result native=cpp_scan(cases[i]);
        rules.push_back(rule(app(sym("inputAt"),sym(case_name)),inputs_term(cases[i])));
        rules.push_back(rule(app(sym("nativeAt"),sym(case_name)),result_term(native)));
        auto semantic=app(sym("semanticAt"),sym(case_name));
        auto native_result=app(sym("nativeAt"),sym(case_name));
        auto native_type=infer({},native_result,sig,rules),semantic_type=infer({},semantic,sig,rules);
        assert(native_type&&semantic_type&&conv(*native_type,result,rules)&&conv(*semantic_type,result,rules));
        assert(conv(nf(native_result,rules),nf(semantic,rules),rules));
    }
    assert(cpp_scan(cases[0]).name==std::vector<char>({'d','i','v'}));
    std::println("Blink tokenizer TagName scan: {} finite character sequences agree (upper/lower normalization and four delimiters)",cases.size());
    std::println("Scope: bounded alphabet accumulation; no token attributes, null replacement, or segmented-stream preprocessor behavior.");
}
