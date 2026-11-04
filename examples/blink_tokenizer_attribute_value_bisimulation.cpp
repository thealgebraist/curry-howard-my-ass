#include "../src/lambda_pi.hpp"
#include <cassert>
#include <vector>

using namespace lp;

enum class Ch { A, B, UpperA, Hash, LessThan, Equals, Backtick, Slash,
                DoubleQuote, SingleQuote, Space, Greater };
enum class Kind { Text, Space, DoubleQuote, SingleQuote, Ampersand, Greater, Eof };
struct Input { Kind kind; Ch value=Ch::A; };
enum class State { BeforeValue, DoubleQuoted, SingleQuoted, Unquoted, AfterQuoted,
                   BeforeAttributeName, SelfClosingStartTag, Data,
                   OpaqueRefDouble, OpaqueRefSingle, OpaqueRefUnquoted };
enum class Action { AwaitMore, EndAttribute, EnterSelfClosing, EmitToken,
                    EmitSelfClosingToken, ParseErrorReconsumeData,
                    ParseErrorReconsumeBeforeAttributeName, OpaqueCharacterReference };
struct Result { State state; Action action; bool parse_error; std::vector<Ch> value; };

static const char* char_name(Ch c) {
    switch(c) {
        case Ch::A:return "aChar"; case Ch::B:return "bChar"; case Ch::UpperA:return "upperAChar";
        case Ch::Hash:return "hashChar"; case Ch::LessThan:return "lessThanChar";
        case Ch::Equals:return "equalsChar"; case Ch::Backtick:return "backtickChar";
        case Ch::Slash:return "slashChar"; case Ch::DoubleQuote:return "doubleQuoteChar";
        case Ch::SingleQuote:return "singleQuoteChar"; case Ch::Space:return "spaceChar";
        case Ch::Greater:return "greaterChar";
    }
    return "aChar";
}
static Result cpp_scan(State state,std::vector<Ch> value,const std::vector<Input>& inputs) {
    bool error=false;
    Result out{state,Action::AwaitMore,error,std::move(value)};
    for(const auto& in:inputs) {
        const bool text=in.kind==Kind::Text;
        const Ch c=text?in.value:in.kind==Kind::Space?Ch::Space:
            in.kind==Kind::DoubleQuote?Ch::DoubleQuote:in.kind==Kind::SingleQuote?Ch::SingleQuote:
            in.kind==Kind::Greater?Ch::Greater:Ch::A;
        if(out.state==State::BeforeValue) {
            if(in.kind==Kind::Space) continue;
            if(in.kind==Kind::DoubleQuote) { out.state=State::DoubleQuoted; continue; }
            if(in.kind==Kind::SingleQuote) { out.state=State::SingleQuoted; continue; }
            if(in.kind==Kind::Ampersand) { out.state=State::OpaqueRefUnquoted; out.action=Action::OpaqueCharacterReference; return out; }
            if(in.kind==Kind::Greater) { out.parse_error=true; out.state=State::Data; out.action=Action::EmitToken; return out; }
            if(in.kind==Kind::Eof) { out.parse_error=true; out.state=State::Data; out.action=Action::ParseErrorReconsumeData; return out; }
            if(c==Ch::LessThan||c==Ch::Equals||c==Ch::Backtick) out.parse_error=true;
            out.value.push_back(c); out.state=State::Unquoted; continue;
        }
        if(out.state==State::DoubleQuoted||out.state==State::SingleQuoted) {
            const bool is_double=out.state==State::DoubleQuoted;
            if((is_double&&in.kind==Kind::DoubleQuote)||(!is_double&&in.kind==Kind::SingleQuote)) {
                out.state=State::AfterQuoted; continue;
            }
            if(in.kind==Kind::Ampersand) {
                out.state=is_double?State::OpaqueRefDouble:State::OpaqueRefSingle;
                out.action=Action::OpaqueCharacterReference; return out;
            }
            if(in.kind==Kind::Eof) { out.parse_error=true; out.state=State::Data; out.action=Action::ParseErrorReconsumeData; return out; }
            out.value.push_back(c); continue;
        }
        if(out.state==State::Unquoted) {
            if(in.kind==Kind::Space) { out.state=State::BeforeAttributeName; out.action=Action::EndAttribute; return out; }
            if(in.kind==Kind::Ampersand) { out.state=State::OpaqueRefUnquoted; out.action=Action::OpaqueCharacterReference; return out; }
            if(in.kind==Kind::Greater) { out.state=State::Data; out.action=Action::EmitToken; return out; }
            if(in.kind==Kind::Eof) { out.parse_error=true; out.state=State::Data; out.action=Action::ParseErrorReconsumeData; return out; }
            if(in.kind==Kind::DoubleQuote||in.kind==Kind::SingleQuote||
               (text&&(c==Ch::LessThan||c==Ch::Equals||c==Ch::Backtick))) out.parse_error=true;
            out.value.push_back(c); continue;
        }
        if(out.state==State::AfterQuoted) {
            if(in.kind==Kind::Space) { out.state=State::BeforeAttributeName; out.action=Action::EndAttribute; return out; }
            if(text&&c==Ch::Slash) { out.state=State::SelfClosingStartTag; out.action=Action::EnterSelfClosing; return out; }
            if(in.kind==Kind::Greater) { out.state=State::Data; out.action=Action::EmitToken; return out; }
            if(in.kind==Kind::Eof) { out.parse_error=true; out.state=State::Data; out.action=Action::ParseErrorReconsumeData; return out; }
            out.parse_error=true; out.state=State::BeforeAttributeName;
            out.action=Action::ParseErrorReconsumeBeforeAttributeName; return out;
        }
        if(out.state==State::SelfClosingStartTag) {
            if(in.kind==Kind::Greater) { out.state=State::Data; out.action=Action::EmitSelfClosingToken; return out; }
            if(in.kind==Kind::Eof) { out.parse_error=true; out.state=State::Data; out.action=Action::ParseErrorReconsumeData; return out; }
            out.parse_error=true; out.state=State::BeforeAttributeName;
            out.action=Action::ParseErrorReconsumeBeforeAttributeName; return out;
        }
        // Opaque character-reference handoffs are terminal observations here.
        assert(out.state==State::Data||out.state==State::BeforeAttributeName||
               out.state==State::OpaqueRefDouble||out.state==State::OpaqueRefSingle||
               out.state==State::OpaqueRefUnquoted);
        return out;
    }
    return out;
}
static Ptr fn(Ptr a,Ptr b) { return arrow(std::move(a),std::move(b)); }
static Rule rule(Ptr lhs,Ptr rhs) { return {std::move(lhs),std::move(rhs)}; }
static Ptr value_term(const std::vector<Ch>& value) {
    Ptr result=sym("nilValue");
    for(auto i=value.rbegin();i!=value.rend();++i)
        result=apps(sym("consValue"),sym(char_name(*i)),result);
    return result;
}
static const char* state_name(State s) {
    switch(s) {
        case State::BeforeValue:return "beforeValueState"; case State::DoubleQuoted:return "doubleQuotedState";
        case State::SingleQuoted:return "singleQuotedState"; case State::Unquoted:return "unquotedState";
        case State::AfterQuoted:return "afterQuotedState"; case State::BeforeAttributeName:return "beforeAttributeNameState";
        case State::SelfClosingStartTag:return "selfClosingStartTagState"; case State::Data:return "dataState";
        case State::OpaqueRefDouble:return "opaqueRefDoubleState"; case State::OpaqueRefSingle:return "opaqueRefSingleState";
        case State::OpaqueRefUnquoted:return "opaqueRefUnquotedState";
    }
    return "dataState";
}
static const char* action_name(Action a) {
    switch(a) {
        case Action::AwaitMore:return "awaitMore"; case Action::EndAttribute:return "endAttribute";
        case Action::EnterSelfClosing:return "enterSelfClosing"; case Action::EmitToken:return "emitToken";
        case Action::EmitSelfClosingToken:return "emitSelfClosingToken";
        case Action::ParseErrorReconsumeData:return "parseErrorReconsumeData";
        case Action::ParseErrorReconsumeBeforeAttributeName:return "parseErrorReconsumeBeforeAttributeName";
        case Action::OpaqueCharacterReference:return "opaqueCharacterReference";
    }
    return "awaitMore";
}
static Ptr result_term(const Result& r) {
    return apps(sym("valueResult"),sym(state_name(r.state)),sym(action_name(r.action)),
                sym(r.parse_error?"true":"false"),value_term(r.value));
}
static Ptr input_term(const Input& in) {
    switch(in.kind) {
        case Kind::Text:return app(sym("textInput"),sym(char_name(in.value)));
        case Kind::Space:return sym("spaceInput"); case Kind::DoubleQuote:return sym("doubleQuoteInput");
        case Kind::SingleQuote:return sym("singleQuoteInput"); case Kind::Ampersand:return sym("ampInput");
        case Kind::Greater:return sym("greaterInput"); case Kind::Eof:return sym("eofInput");
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
    const Ptr ch=sym("ValueChar"), value=sym("Value"), input=sym("Input"), seq=sym("InputSeq"),
        state=sym("ValueState"), action=sym("ValueAction"), result_type=sym("ValueResult"), bit=sym("Bool");
    const std::vector<const char*> chars={"aChar","bChar","upperAChar","hashChar","lessThanChar",
        "equalsChar","backtickChar","slashChar","doubleQuoteChar","singleQuoteChar","spaceChar","greaterChar"};
    sig["ValueChar"]=type_sort(); for(auto c:chars) sig[c]=ch;
    sig["Value"]=type_sort(); sig["nilValue"]=value; sig["consValue"]=fn(ch,fn(value,value));
    sig["snocValue"]=fn(value,fn(ch,value)); sig["Input"]=type_sort(); sig["textInput"]=fn(ch,input);
    for(const char* c:{"spaceInput","doubleQuoteInput","singleQuoteInput","ampInput","greaterInput","eofInput"}) sig[c]=input;
    sig["InputSeq"]=type_sort(); sig["nilInputSeq"]=seq; sig["consInput"]=fn(input,fn(seq,seq));
    sig["ValueState"]=type_sort();
    for(const char* c:{"beforeValueState","doubleQuotedState","singleQuotedState","unquotedState","afterQuotedState",
        "beforeAttributeNameState","selfClosingStartTagState","dataState","opaqueRefDoubleState","opaqueRefSingleState","opaqueRefUnquotedState"}) sig[c]=state;
    sig["ValueAction"]=type_sort();
    for(const char* c:{"awaitMore","endAttribute","enterSelfClosing","emitToken","emitSelfClosingToken",
        "parseErrorReconsumeData","parseErrorReconsumeBeforeAttributeName","opaqueCharacterReference"}) sig[c]=action;
    sig["Bool"]=type_sort(); sig["true"]=bit; sig["false"]=bit;
    sig["ValueResult"]=type_sort(); sig["valueResult"]=fn(state,fn(action,fn(bit,fn(value,result_type))));
    sig["snocValue"]=fn(value,fn(ch,value)); sig["scanValue"]=fn(state,fn(bit,fn(value,fn(seq,result_type))));
    sig["Case"]=type_sort(); sig["stateAt"]=fn(sym("Case"),state); sig["errorAt"]=fn(sym("Case"),bit);
    sig["valueAt"]=fn(sym("Case"),value); sig["inputAt"]=fn(sym("Case"),seq);
    sig["nativeAt"]=fn(sym("Case"),result_type); sig["semanticAt"]=fn(sym("Case"),result_type);

    rules.push_back(rule(apps(sym("snocValue"),sym("nilValue"),pv(0)),apps(sym("consValue"),pv(0),sym("nilValue"))));
    rules.push_back(rule(apps(sym("snocValue"),apps(sym("consValue"),pv(0),pv(1)),pv(2)),
        apps(sym("consValue"),pv(0),apps(sym("snocValue"),pv(1),pv(2)))));
    rules.push_back(rule(apps(sym("scanValue"),pv(0),pv(1),pv(2),sym("nilInputSeq")),
        apps(sym("valueResult"),pv(0),sym("awaitMore"),pv(1),pv(2))));

    auto result=[&](const char* st,const char* act,Ptr err,Ptr val){return apps(sym("valueResult"),sym(st),sym(act),err,val);};
    auto scan=[&](const char* st,Ptr err,Ptr val,Ptr rest){return apps(sym("scanValue"),sym(st),err,val,rest);};
    auto append=[&](Ptr val,const char* c){return apps(sym("snocValue"),val,sym(c));};
    // BeforeAttributeValue: whitespace is skipped; quotes select modes;
    // ampersand is reconsumed in unquoted mode and immediately reaches the
    // explicit opaque character-reference boundary.
    rules.push_back(rule(apps(sym("scanValue"),sym("beforeValueState"),pv(0),pv(1),apps(sym("consInput"),sym("spaceInput"),pv(2))),scan("beforeValueState",pv(0),pv(1),pv(2))));
    rules.push_back(rule(apps(sym("scanValue"),sym("beforeValueState"),pv(0),pv(1),apps(sym("consInput"),sym("doubleQuoteInput"),pv(2))),scan("doubleQuotedState",pv(0),pv(1),pv(2))));
    rules.push_back(rule(apps(sym("scanValue"),sym("beforeValueState"),pv(0),pv(1),apps(sym("consInput"),sym("singleQuoteInput"),pv(2))),scan("singleQuotedState",pv(0),pv(1),pv(2))));
    rules.push_back(rule(apps(sym("scanValue"),sym("beforeValueState"),pv(0),pv(1),apps(sym("consInput"),sym("ampInput"),pv(2))),result("opaqueRefUnquotedState","opaqueCharacterReference",pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("beforeValueState"),pv(0),pv(1),apps(sym("consInput"),sym("greaterInput"),pv(2))),result("dataState","emitToken",sym("true"),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("beforeValueState"),pv(0),pv(1),apps(sym("consInput"),sym("eofInput"),pv(2))),result("dataState","parseErrorReconsumeData",sym("true"),pv(1))));
    for(const char* c:chars) {
        const bool err=std::string_view(c)=="lessThanChar"||std::string_view(c)=="equalsChar"||std::string_view(c)=="backtickChar";
        rules.push_back(rule(apps(sym("scanValue"),sym("beforeValueState"),pv(0),pv(1),apps(sym("consInput"),app(sym("textInput"),sym(c)),pv(2))),
            scan("unquotedState",err?sym("true"):pv(0),append(pv(1),c),pv(2))));
    }
    // Quoted states close only on their matching quote, preserve all other
    // represented characters, and hand off ampersands without decoding.
    for(const auto& cfg:std::vector<std::tuple<const char*,const char*,const char*>>{
        {"doubleQuotedState","doubleQuoteInput","doubleQuoteChar"},
        {"singleQuotedState","singleQuoteInput","singleQuoteChar"}}) {
        auto st=std::get<0>(cfg), quote=std::get<1>(cfg), quote_char=std::get<2>(cfg);
        const char* opaque=std::string_view(st)=="doubleQuotedState"?"opaqueRefDoubleState":"opaqueRefSingleState";
        rules.push_back(rule(apps(sym("scanValue"),sym(st),pv(0),pv(1),apps(sym("consInput"),sym(quote),pv(2))),scan("afterQuotedState",pv(0),pv(1),pv(2))));
        rules.push_back(rule(apps(sym("scanValue"),sym(st),pv(0),pv(1),apps(sym("consInput"),sym("ampInput"),pv(2))),result(opaque,"opaqueCharacterReference",pv(0),pv(1))));
        rules.push_back(rule(apps(sym("scanValue"),sym(st),pv(0),pv(1),apps(sym("consInput"),sym("eofInput"),pv(2))),result("dataState","parseErrorReconsumeData",sym("true"),pv(1))));
        for(const char* c:chars) if(std::string_view(c)!=quote_char)
            rules.push_back(rule(apps(sym("scanValue"),sym(st),pv(0),pv(1),apps(sym("consInput"),app(sym("textInput"),sym(c)),pv(2))),
                scan(st,pv(0),append(pv(1),c),pv(2))));
        const char* other_quote=std::string_view(st)=="doubleQuotedState"?"singleQuoteInput":"doubleQuoteInput";
        const char* other_char=std::string_view(st)=="doubleQuotedState"?"singleQuoteChar":"doubleQuoteChar";
        rules.push_back(rule(apps(sym("scanValue"),sym(st),pv(0),pv(1),apps(sym("consInput"),sym(other_quote),pv(2))),scan(st,pv(0),append(pv(1),other_char),pv(2))));
        rules.push_back(rule(apps(sym("scanValue"),sym(st),pv(0),pv(1),apps(sym("consInput"),sym("spaceInput"),pv(2))),scan(st,pv(0),append(pv(1),"spaceChar"),pv(2))));
        rules.push_back(rule(apps(sym("scanValue"),sym(st),pv(0),pv(1),apps(sym("consInput"),sym("greaterInput"),pv(2))),scan(st,pv(0),append(pv(1),"greaterChar"),pv(2))));
    }
    // Unquoted value terminators and parse-error characters.
    rules.push_back(rule(apps(sym("scanValue"),sym("unquotedState"),pv(0),pv(1),apps(sym("consInput"),sym("spaceInput"),pv(2))),result("beforeAttributeNameState","endAttribute",pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("unquotedState"),pv(0),pv(1),apps(sym("consInput"),sym("ampInput"),pv(2))),result("opaqueRefUnquotedState","opaqueCharacterReference",pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("unquotedState"),pv(0),pv(1),apps(sym("consInput"),sym("greaterInput"),pv(2))),result("dataState","emitToken",pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("unquotedState"),pv(0),pv(1),apps(sym("consInput"),sym("eofInput"),pv(2))),result("dataState","parseErrorReconsumeData",sym("true"),pv(1))));
    for(const char* c:chars) {
        const bool err=std::string_view(c)=="lessThanChar"||std::string_view(c)=="equalsChar"||std::string_view(c)=="backtickChar";
        rules.push_back(rule(apps(sym("scanValue"),sym("unquotedState"),pv(0),pv(1),apps(sym("consInput"),app(sym("textInput"),sym(c)),pv(2))),
            scan("unquotedState",err?sym("true"):pv(0),append(pv(1),c),pv(2))));
    }
    for(const auto& cfg:std::vector<std::pair<const char*,const char*>>{{"doubleQuoteInput","doubleQuoteChar"},{"singleQuoteInput","singleQuoteChar"}})
        rules.push_back(rule(apps(sym("scanValue"),sym("unquotedState"),pv(0),pv(1),apps(sym("consInput"),sym(cfg.first),pv(2))),
            scan("unquotedState",sym("true"),append(pv(1),cfg.second),pv(2))));
    // Post-quote and self-closing dispatch boundaries.
    rules.push_back(rule(apps(sym("scanValue"),sym("afterQuotedState"),pv(0),pv(1),apps(sym("consInput"),sym("spaceInput"),pv(2))),result("beforeAttributeNameState","endAttribute",pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("afterQuotedState"),pv(0),pv(1),apps(sym("consInput"),app(sym("textInput"),sym("slashChar")),pv(2))),result("selfClosingStartTagState","enterSelfClosing",pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("afterQuotedState"),pv(0),pv(1),apps(sym("consInput"),sym("greaterInput"),pv(2))),result("dataState","emitToken",pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("afterQuotedState"),pv(0),pv(1),apps(sym("consInput"),sym("eofInput"),pv(2))),result("dataState","parseErrorReconsumeData",sym("true"),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("afterQuotedState"),pv(0),pv(1),apps(sym("consInput"),pv(2),pv(3))),result("beforeAttributeNameState","parseErrorReconsumeBeforeAttributeName",sym("true"),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("selfClosingStartTagState"),pv(0),pv(1),apps(sym("consInput"),sym("greaterInput"),pv(2))),result("dataState","emitSelfClosingToken",pv(0),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("selfClosingStartTagState"),pv(0),pv(1),apps(sym("consInput"),sym("eofInput"),pv(2))),result("dataState","parseErrorReconsumeData",sym("true"),pv(1))));
    rules.push_back(rule(apps(sym("scanValue"),sym("selfClosingStartTagState"),pv(0),pv(1),apps(sym("consInput"),pv(2),pv(3))),result("beforeAttributeNameState","parseErrorReconsumeBeforeAttributeName",sym("true"),pv(1))));

    struct Case { State state; std::vector<Ch> initial; std::vector<Input> input; };
    const std::vector<Case> cases={
        {State::BeforeValue,{},{{Kind::Space},{Kind::DoubleQuote},{Kind::Text,Ch::A},{Kind::Space},{Kind::Greater},{Kind::Text,Ch::B},{Kind::DoubleQuote},{Kind::Greater}}},
        {State::BeforeValue,{},{{Kind::SingleQuote},{Kind::Text,Ch::UpperA},{Kind::SingleQuote},{Kind::Space}}},
        {State::BeforeValue,{},{{Kind::Text,Ch::A},{Kind::Text,Ch::B},{Kind::Space}}},
        {State::BeforeValue,{},{{Kind::Greater}}}, {State::BeforeValue,{},{{Kind::Eof}}},
        {State::BeforeValue,{},{{Kind::Text,Ch::LessThan},{Kind::Text,Ch::B},{Kind::Greater}}},
        {State::BeforeValue,{},{{Kind::Text,Ch::Equals},{Kind::Text,Ch::B},{Kind::Greater}}},
        {State::BeforeValue,{},{{Kind::Ampersand}}},
        {State::DoubleQuoted,{Ch::A},{{Kind::Ampersand}}},
        {State::SingleQuoted,{Ch::B},{{Kind::Ampersand}}},
        {State::Unquoted,{Ch::A},{{Kind::Ampersand}}},
        {State::BeforeValue,{},{{Kind::Text,Ch::A},{Kind::Text,Ch::Slash},{Kind::Greater}}},
        {State::Unquoted,{},{{Kind::DoubleQuote},{Kind::Text,Ch::B},{Kind::Greater}}},
        {State::DoubleQuoted,{Ch::A},{{Kind::Eof}}},
        {State::DoubleQuoted,{Ch::A},{}},
        {State::AfterQuoted,{Ch::A},{{Kind::Text,Ch::B}}},
        {State::AfterQuoted,{Ch::A},{{Kind::Text,Ch::Slash},{Kind::Greater}}},
        {State::SelfClosingStartTag,{Ch::A},{{Kind::Greater}}},
        {State::SelfClosingStartTag,{Ch::A},{{Kind::Text,Ch::B}}},
        {State::Unquoted,{Ch::A},{{Kind::Eof}}}};
    for(size_t i=0;i<cases.size();++i) {
        const auto id="valueCase"+std::to_string(i); sig[id]=sym("Case");
        const auto& c=cases[i]; const auto native=cpp_scan(c.state,c.initial,c.input);
        rules.push_back(rule(app(sym("stateAt"),sym(id)),sym(state_name(c.state))));
        rules.push_back(rule(app(sym("errorAt"),sym(id)),sym("false")));
        rules.push_back(rule(app(sym("valueAt"),sym(id)),value_term(c.initial)));
        rules.push_back(rule(app(sym("inputAt"),sym(id)),input_seq_term(c.input)));
        rules.push_back(rule(app(sym("nativeAt"),sym(id)),result_term(native)));
        const auto actual=app(sym("semanticAt"),sym(id));
        rules.push_back(rule(app(sym("semanticAt"),sym(id)),apps(sym("scanValue"),app(sym("stateAt"),sym(id)),
            app(sym("errorAt"),sym(id)),app(sym("valueAt"),sym(id)),app(sym("inputAt"),sym(id)))));
        const auto nt=infer({},app(sym("nativeAt"),sym(id)),sig,rules),at=infer({},actual,sig,rules);
        assert(nt&&at&&conv(*nt,result_type,rules)&&conv(*at,result_type,rules));
        assert(conv(nf(app(sym("nativeAt"),sym(id)),rules),nf(actual,rules),rules));
    }
    std::println("Blink tokenizer attribute-value path: {} finite traces agree; character-reference resolution remains opaque",cases.size());
}
