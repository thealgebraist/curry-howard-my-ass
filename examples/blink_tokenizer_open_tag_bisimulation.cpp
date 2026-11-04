#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <vector>

using namespace lp;

enum class State { Data, TagOpen, EndTagOpen, TagName, MarkupOpen, BogusComment, CharacterReference };
enum class Input { AlphaLower, AlphaUpper, Bang, Slash, Question, Greater, Ampersand, LessThan, Eof, Other };
enum class Action { EmitCharacter, EmitEof, BeginStartTagLower, BeginEndTagLower,
                    EnterMarkupDeclaration, EnterEndTagOpen, EnterTagOpen,
                    EnterCharacterReference, ReconsumeInData, ParseErrorToData,
                    BufferLessThanSlash, ParseErrorToBogusComment, OpaqueContinue };
struct Transition { Action action; State next; };

static Transition cpp_step(State state, Input input) {
    switch (state) {
      case State::Data:
        if (input == Input::Ampersand) return {Action::EnterCharacterReference, State::CharacterReference};
        if (input == Input::LessThan) return {Action::EnterTagOpen, State::TagOpen};
        if (input == Input::Eof) return {Action::EmitEof, State::Data};
        return {Action::EmitCharacter, State::Data};
      case State::TagOpen:
        if (input == Input::AlphaLower || input == Input::AlphaUpper)
            return {Action::BeginStartTagLower, State::TagName};
        if (input == Input::Bang) return {Action::EnterMarkupDeclaration, State::MarkupOpen};
        if (input == Input::Slash) return {Action::EnterEndTagOpen, State::EndTagOpen};
        if (input == Input::Question)
            return {Action::ParseErrorToBogusComment, State::BogusComment};
        return {Action::ReconsumeInData, State::Data};
      case State::EndTagOpen:
        if (input == Input::AlphaLower || input == Input::AlphaUpper)
            return {Action::BeginEndTagLower, State::TagName};
        if (input == Input::Greater) return {Action::ParseErrorToData, State::Data};
        if (input == Input::Eof) return {Action::BufferLessThanSlash, State::Data};
        return {Action::ParseErrorToBogusComment, State::BogusComment};
      case State::TagName:
      case State::MarkupOpen:
      case State::BogusComment:
      case State::CharacterReference:
        return {Action::OpaqueContinue, state};
    }
    assert(false);
    return {Action::OpaqueContinue, state};
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr outcome(const char* action, const char* state) {
    return apps(sym("transition"), sym(action), sym(state));
}
static Ptr state_term(State s) {
    static constexpr std::array names={"dataState","tagOpenState","endTagOpenState","tagNameState",
                                       "markupOpenState","bogusCommentState","characterReferenceState"};
    return sym(names[static_cast<size_t>(s)]);
}
static Ptr input_term(Input i) {
    static constexpr std::array names={"alphaLower","alphaUpper","bang","slash","question",
                                       "greater","ampersand","lessThan","eofChar","otherChar"};
    return sym(names[static_cast<size_t>(i)]);
}
static const char* action_name(Action a) {
    static constexpr std::array names={"emitCharacter","emitEof","beginStartTagLower",
        "beginEndTagLower","enterMarkupDeclaration","enterEndTagOpen","enterTagOpen",
        "enterCharacterReference","reconsumeInData","parseErrorToData","bufferLessThanSlash",
        "parseErrorToBogusComment","opaqueContinue"};
    return names[static_cast<size_t>(a)];
}
int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto p=[](size_t i){return pvar(i);};
    const Ptr state=sym("TokenizerState"), input=sym("InputClass"), action=sym("Action");
    const Ptr transition=sym("Transition"), bit=sym("Bool");
    sig["TokenizerState"]=type_sort();
    for(State s:{State::Data,State::TagOpen,State::EndTagOpen,State::TagName,
                 State::MarkupOpen,State::BogusComment,State::CharacterReference})
        sig[to_string(state_term(s))]=state;
    sig["InputClass"]=type_sort();
    for(Input i:{Input::AlphaLower,Input::AlphaUpper,Input::Bang,Input::Slash,Input::Question,
                 Input::Greater,Input::Ampersand,Input::LessThan,Input::Eof,Input::Other})
        sig[to_string(input_term(i))]=input;
    sig["Action"]=type_sort();
    for(Action a:{Action::EmitCharacter,Action::EmitEof,Action::BeginStartTagLower,
                  Action::BeginEndTagLower,Action::EnterMarkupDeclaration,Action::EnterEndTagOpen,
                  Action::EnterTagOpen,Action::EnterCharacterReference,Action::ReconsumeInData,
                  Action::ParseErrorToData,Action::BufferLessThanSlash,
                  Action::ParseErrorToBogusComment,Action::OpaqueContinue})
        sig[action_name(a)]=action;
    sig["transition"]=fn(action,fn(state,transition));
    sig["step"]=fn(state,fn(input,transition));
    sig["Bool"]=type_sort(); sig["true"]=bit; sig["false"]=bit;
    sig["eqTransition"]=fn(transition,fn(transition,bit));
    sig["eqAction"]=fn(action,fn(action,bit));
    sig["eqState"]=fn(state,fn(state,bit));
    sig["andB"]=fn(bit,fn(bit,bit));
    sig["stateAt"]=fn(sym("Nat"),state); sig["inputAt"]=fn(sym("Nat"),input);
    sig["nativeAt"]=fn(sym("Nat"),transition); sig["semanticAt"]=fn(sym("Nat"),transition);
    sig["checkAt"]=fn(sym("Nat"),bit); sig["Nat"]=type_sort(); sig["zero"]=sym("Nat");
    sig["succ"]=fn(sym("Nat"),sym("Nat"));

    for(State s:{State::Data,State::TagOpen,State::EndTagOpen,State::TagName,
                 State::MarkupOpen,State::BogusComment,State::CharacterReference}) {
        for(Input i:{Input::AlphaLower,Input::AlphaUpper,Input::Bang,Input::Slash,Input::Question,
                     Input::Greater,Input::Ampersand,Input::LessThan,Input::Eof,Input::Other}) {
            const Transition t=cpp_step(s,i);
            rules.push_back(rule(apps(sym("step"),state_term(s),input_term(i)),
                                 outcome(action_name(t.action),
                                     [&]() -> const char* {
                                         static constexpr std::array names={"dataState","tagOpenState","endTagOpenState","tagNameState",
                                             "markupOpenState","bogusCommentState","characterReferenceState"};
                                         return names[static_cast<size_t>(t.next)];
                                     }())));
        }
    }
    rules.push_back(rule(app(sym("semanticAt"),p(0)),apps(sym("step"),app(sym("stateAt"),p(0)),app(sym("inputAt"),p(0)))));
    rules.push_back(rule(apps(sym("andB"),sym("true"),sym("true")),sym("true")));
    rules.push_back(rule(apps(sym("andB"),sym("true"),sym("false")),sym("false")));
    rules.push_back(rule(apps(sym("andB"),sym("false"),p(0)),sym("false")));
    for(Action a:{Action::EmitCharacter,Action::EmitEof,Action::BeginStartTagLower,
                  Action::BeginEndTagLower,Action::EnterMarkupDeclaration,Action::EnterEndTagOpen,
                  Action::EnterTagOpen,Action::EnterCharacterReference,Action::ReconsumeInData,
                  Action::ParseErrorToData,Action::BufferLessThanSlash,
                  Action::ParseErrorToBogusComment,Action::OpaqueContinue})
        for(Action b:{Action::EmitCharacter,Action::EmitEof,Action::BeginStartTagLower,
                      Action::BeginEndTagLower,Action::EnterMarkupDeclaration,Action::EnterEndTagOpen,
                      Action::EnterTagOpen,Action::EnterCharacterReference,Action::ReconsumeInData,
                      Action::ParseErrorToData,Action::BufferLessThanSlash,
                      Action::ParseErrorToBogusComment,Action::OpaqueContinue})
            rules.push_back(rule(apps(sym("eqAction"),sym(action_name(a)),sym(action_name(b))),
                                 sym(a==b?"true":"false")));
    for(State a:{State::Data,State::TagOpen,State::EndTagOpen,State::TagName,
                 State::MarkupOpen,State::BogusComment,State::CharacterReference})
        for(State b:{State::Data,State::TagOpen,State::EndTagOpen,State::TagName,
                     State::MarkupOpen,State::BogusComment,State::CharacterReference})
            rules.push_back(rule(apps(sym("eqState"),state_term(a),state_term(b)),
                                 sym(a==b?"true":"false")));
    rules.push_back(rule(
        apps(sym("eqTransition"),apps(sym("transition"),p(0),p(1)),
             apps(sym("transition"),p(2),p(3))),
        apps(sym("andB"),apps(sym("eqAction"),p(0),p(2)),
             apps(sym("eqState"),p(1),p(3)))));
    rules.push_back(rule(app(sym("checkAt"),p(0)),apps(sym("eqTransition"),app(sym("nativeAt"),p(0)),app(sym("semanticAt"),p(0)))));

    std::vector<std::pair<State,Input>> cases;
    for(State s:{State::Data,State::TagOpen,State::EndTagOpen,
                 State::TagName,State::MarkupOpen,State::BogusComment,State::CharacterReference})
        for(Input i:{Input::AlphaLower,Input::AlphaUpper,Input::Bang,Input::Slash,Input::Question,
                     Input::Greater,Input::Ampersand,Input::LessThan,Input::Eof,Input::Other})
            cases.emplace_back(s,i);

    for(size_t n=0;n<cases.size();++n) {
        Ptr id=sym(("case"+std::to_string(n)).c_str()); sig[to_string(id)]=sym("Nat");
        const auto [s,i]=cases[n];
        rules.push_back(rule(app(sym("stateAt"),id),state_term(s)));
        rules.push_back(rule(app(sym("inputAt"),id),input_term(i)));
        const Transition t=cpp_step(s,i);
        const auto result=outcome(action_name(t.action),
            [&]() -> const char* {
                static constexpr std::array names={"dataState","tagOpenState","endTagOpenState","tagNameState",
                    "markupOpenState","bogusCommentState","characterReferenceState"};
                return names[static_cast<size_t>(t.next)];
            }());
        rules.push_back(rule(app(sym("nativeAt"),id),result));
        const Ptr check=app(sym("checkAt"),id);
        const auto inferred=infer({},check,sig,rules);
        assert(inferred&&conv(*inferred,bit,rules));
        assert(conv(nf(check,rules),sym("true"),rules));
    }

    std::println("Blink tokenizer open-tag dispatch: {} state/input transitions agree between C++ and typed λΠ",cases.size());
    std::println("Scope: Data/TagOpen/EndTagOpen branches; later tag-name, markup, bogus-comment, and entity states are opaque.");
}
