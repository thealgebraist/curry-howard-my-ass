#include "../src/lambda_pi.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <tuple>
#include <vector>

using namespace lp;

enum class Unit { A, B, Wide, CR, LF, Nul, Replacement, Eof };
using Stream = std::vector<Unit>;

static constexpr std::array<const char*,76> tokenizer_state_names={
  "kDataState",
  "kCharacterReferenceInDataState",
  "kRCDATAState",
  "kCharacterReferenceInRCDATAState",
  "kRAWTEXTState",
  "kScriptDataState",
  "kPLAINTEXTState",
  "kTagOpenState",
  "kEndTagOpenState",
  "kTagNameState",
  "kRCDATALessThanSignState",
  "kRCDATAEndTagOpenState",
  "kRCDATAEndTagNameState",
  "kRAWTEXTLessThanSignState",
  "kRAWTEXTEndTagOpenState",
  "kRAWTEXTEndTagNameState",
  "kScriptDataLessThanSignState",
  "kScriptDataEndTagOpenState",
  "kScriptDataEndTagNameState",
  "kScriptDataEscapeStartState",
  "kScriptDataEscapeStartDashState",
  "kScriptDataEscapedState",
  "kScriptDataEscapedDashState",
  "kScriptDataEscapedDashDashState",
  "kScriptDataEscapedLessThanSignState",
  "kScriptDataEscapedEndTagOpenState",
  "kScriptDataEscapedEndTagNameState",
  "kScriptDataDoubleEscapeStartState",
  "kScriptDataDoubleEscapedState",
  "kScriptDataDoubleEscapedDashState",
  "kScriptDataDoubleEscapedDashDashState",
  "kScriptDataDoubleEscapedLessThanSignState",
  "kScriptDataDoubleEscapeEndState",
  "kBeforeAttributeNameState",
  "kAttributeNameState",
  "kAfterAttributeNameState",
  "kBeforeAttributeValueState",
  "kAttributeValueDoubleQuotedState",
  "kAttributeValueSingleQuotedState",
  "kAttributeValueUnquotedState",
  "kCharacterReferenceInAttributeValueState",
  "kAfterAttributeValueQuotedState",
  "kSelfClosingStartTagState",
  "kProcessingInstructionOpenState",
  "kProcessingInstructionTargetState",
  "kAfterProcessingInstructionTargetState",
  "kProcessingInstructionDataState",
  "kProcessingInstructionQuestionableState",
  "kBogusCommentState",
  "kContinueBogusCommentState",
  "kMarkupDeclarationOpenState",
  "kCommentStartState",
  "kCommentStartDashState",
  "kCommentState",
  "kCommentEndDashState",
  "kCommentEndState",
  "kCommentEndBangState",
  "kDOCTYPEState",
  "kBeforeDOCTYPENameState",
  "kDOCTYPENameState",
  "kAfterDOCTYPENameState",
  "kAfterDOCTYPEPublicKeywordState",
  "kBeforeDOCTYPEPublicIdentifierState",
  "kDOCTYPEPublicIdentifierDoubleQuotedState",
  "kDOCTYPEPublicIdentifierSingleQuotedState",
  "kAfterDOCTYPEPublicIdentifierState",
  "kBetweenDOCTYPEPublicAndSystemIdentifiersState",
  "kAfterDOCTYPESystemKeywordState",
  "kBeforeDOCTYPESystemIdentifierState",
  "kDOCTYPESystemIdentifierDoubleQuotedState",
  "kDOCTYPESystemIdentifierSingleQuotedState",
  "kAfterDOCTYPESystemIdentifierState",
  "kBogusDOCTYPEState",
  "kCDATASectionState",
  "kCDATASectionBracketState",
  "kCDATASectionEndState",
};
static bool cpp_should_skip_null(size_t state,bool force_replacement){
  return !force_replacement&&(state==0||state==2||state==4);
}

static Stream cpp_preprocess(const Stream& input, bool skip_null, bool closed) {
  Stream out;
  for (size_t i = 0; i < input.size(); ++i) {
    const Unit c = input[i];
    if (c == Unit::CR) {
      out.push_back(Unit::LF);
      if (i + 1 < input.size() && input[i + 1] == Unit::LF) ++i;
    } else if (c == Unit::Nul) {
      if (closed && i + 1 == input.size()) out.push_back(Unit::Eof);
      else if (!skip_null) out.push_back(Unit::Replacement);
    } else {
      out.push_back(c);
    }
  }
  return out;
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }
static Ptr apps(Ptr f, std::initializer_list<Ptr> xs) {
  for (const auto& x : xs) f = app(f, x);
  return f;
}

static const char* code_name(Unit u) {
  switch (u) {
    case Unit::A: return "codeA";
    case Unit::B: return "codeB";
    case Unit::Wide: return "codeWide";
    default: return "codeA";
  }
}

static Ptr raw_unit(Unit u) {
  switch (u) {
    case Unit::CR: return sym("rawCR");
    case Unit::LF: return sym("rawLF");
    case Unit::Nul: return sym("rawNul");
    default: return app(sym("rawOrdinary"), sym(code_name(u)));
  }
}
static Ptr out_unit(Unit u) {
  switch (u) {
    case Unit::LF: return sym("outLF");
    case Unit::Nul: return sym("outNul");
    case Unit::Replacement: return sym("replacement");
    case Unit::Eof: return sym("eofMarker");
    default: return app(sym("outOrdinary"), sym(code_name(u)));
  }
}
static Ptr raw_stream(const Stream& s) {
  Ptr t = sym("nilRaw");
  for (auto it = s.rbegin(); it != s.rend(); ++it)
    t = apps(sym("consRaw"), {raw_unit(*it), t});
  return t;
}
static Ptr out_stream(const Stream& s) {
  Ptr t = sym("nilOut");
  for (auto it = s.rbegin(); it != s.rend(); ++it)
    t = apps(sym("consOut"), {out_unit(*it), t});
  return t;
}
static Ptr input_term(size_t state,bool force_replacement,bool closed,const Stream& s) {
  const Ptr policy=apps(sym("shouldSkipNullCharacters"),{sym("tokenizerState"+std::to_string(state)),sym(force_replacement?"yes":"no")});
  return apps(sym("preprocess"), {policy,sym(closed ? "yes" : "no"),raw_stream(s)});
}

int main() {
  const Ptr B = sym("Bool"), In = sym("InputUnit"), Raw = sym("RawUnit");
  const Ptr Out = sym("OutputUnit"), RI = sym("RawStream"), RO = sym("OutputStream"), TS=sym("TokenizerState");
  Signature sig;
  std::vector<Rule> rules;
  sig["Bool"] = type_sort(); sig["yes"] = B; sig["no"] = B;
  sig["TokenizerState"]=type_sort();
  for(size_t i=0;i<tokenizer_state_names.size();++i)sig["tokenizerState"+std::to_string(i)]=TS;
  sig["shouldSkipNullCharacters"]=fn(TS,fn(B,B));
  sig["InputUnit"] = type_sort();
  for (const char* c : {"codeA", "codeB", "codeWide"}) sig[c] = In;
  sig["RawUnit"] = type_sort();
  sig["rawCR"] = Raw; sig["rawLF"] = Raw; sig["rawNul"] = Raw;
  sig["rawOrdinary"] = fn(In, Raw);
  sig["OutputUnit"] = type_sort();
  sig["outLF"] = Out; sig["outNul"] = Out; sig["replacement"] = Out; sig["eofMarker"] = Out;
  sig["outOrdinary"] = fn(In, Out);
  sig["RawStream"] = type_sort(); sig["nilRaw"] = RI;
  sig["consRaw"] = fn(Raw, fn(RI, RI));
  sig["OutputStream"] = type_sort(); sig["nilOut"] = RO;
  sig["consOut"] = fn(Out, fn(RO, RO));
  sig["preprocess"] = fn(B, fn(B, fn(RI, RO)));
  for(size_t i=0;i<tokenizer_state_names.size();++i){
    const bool state_skips=(i==0||i==2||i==4);
    rules.push_back(rule(apps(sym("shouldSkipNullCharacters"),{sym("tokenizerState"+std::to_string(i)),sym("no")}),sym(state_skips?"yes":"no")));
    rules.push_back(rule(apps(sym("shouldSkipNullCharacters"),{sym("tokenizerState"+std::to_string(i)),sym("yes")}),sym("no")));
  }

  // Variables 0/1/2 are skip-null, closed-stream, and input tail.
  rules.push_back(rule(apps(sym("preprocess"), {pvar(0), pvar(1), sym("nilRaw")}), sym("nilOut")));
  // CR becomes LF; an immediately following LF is consumed with it.
  rules.push_back(rule(apps(sym("preprocess"), {pvar(0), pvar(1), apps(sym("consRaw"), {sym("rawCR"), apps(sym("consRaw"), {sym("rawLF"), pvar(2)})})}),
                       apps(sym("consOut"), {sym("outLF"), apps(sym("preprocess"), {pvar(0), pvar(1), pvar(2)})})));
  rules.push_back(rule(apps(sym("preprocess"), {pvar(0), pvar(1), apps(sym("consRaw"), {sym("rawCR"), pvar(2)})}),
                       apps(sym("consOut"), {sym("outLF"), apps(sym("preprocess"), {pvar(0), pvar(1), pvar(2)})})));
  // Closed source with one remaining NUL uses it as the EOF marker.
  rules.push_back(rule(apps(sym("preprocess"), {pvar(0), sym("yes"), apps(sym("consRaw"), {sym("rawNul"), sym("nilRaw")})}),
                       apps(sym("consOut"), {sym("eofMarker"), sym("nilOut")})));
  rules.push_back(rule(apps(sym("preprocess"), {sym("yes"), pvar(1), apps(sym("consRaw"), {sym("rawNul"), pvar(2)})}),
                       apps(sym("preprocess"), {sym("yes"), pvar(1), pvar(2)})));
  rules.push_back(rule(apps(sym("preprocess"), {sym("no"), pvar(1), apps(sym("consRaw"), {sym("rawNul"), pvar(2)})}),
                       apps(sym("consOut"), {sym("replacement"), apps(sym("preprocess"), {sym("no"), pvar(1), pvar(2)})})));
  rules.push_back(rule(apps(sym("preprocess"), {pvar(0), pvar(1), apps(sym("consRaw"), {sym("rawLF"), pvar(2)})}),
                       apps(sym("consOut"), {sym("outLF"), apps(sym("preprocess"), {pvar(0), pvar(1), pvar(2)})})));
  // The ordinary-code-unit branch is parametric in its payload.
  rules.push_back(rule(apps(sym("preprocess"), {pvar(0), pvar(1), apps(sym("consRaw"), {app(sym("rawOrdinary"), pvar(3)), pvar(2)})}),
                       apps(sym("consOut"), {app(sym("outOrdinary"), pvar(3)), apps(sym("preprocess"), {pvar(0), pvar(1), pvar(2)})})));

  const std::vector<std::tuple<Stream,size_t,bool,bool>> cases = {
      {{Unit::A, Unit::CR, Unit::LF, Unit::B},0,false,false},
      {{Unit::CR, Unit::CR, Unit::LF},2,false,false},
      {{Unit::Nul, Unit::Wide, Unit::Nul},38,false,false},
      {{Unit::Nul, Unit::A, Unit::Nul},0,false,false},
      {{Unit::A, Unit::Nul},0,true,false},
      {{Unit::Wide, Unit::Nul},38,false,true},
      {{Unit::A, Unit::LF, Unit::B},5,false,false},
  };
  for(const auto& [input,state,force_replacement,closed]:cases){
    const bool skip_null=cpp_should_skip_null(state,force_replacement);
    const auto expected=cpp_preprocess(input,skip_null,closed);
    const Ptr observed=nf(input_term(state,force_replacement,closed,input),rules);
    assert(conv(observed,out_stream(expected),rules));
  }
  // The source policy is exhaustive over the pinned tokenizer state enum and
  // both force-replacement settings. These encoded checks were not executed.
  for(size_t state=0;state<tokenizer_state_names.size();++state)for(bool force:{false,true}){
    const Ptr observed=nf(apps(sym("shouldSkipNullCharacters"),{sym("tokenizerState"+std::to_string(state)),sym(force?"yes":"no")}),rules);
    assert(conv(observed,sym(cpp_should_skip_null(state,force)?"yes":"no"),rules));
  }
  std::cout << "Blink input-stream preprocessor model: " << cases.size()
            << " finite traces; null policy encoded for " << tokenizer_state_names.size()
            << " tokenizer states and both force settings\n";
}
