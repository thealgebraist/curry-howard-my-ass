#include "../src/lambda_pi.hpp"
#include <cassert>

using namespace lp;

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }

int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto p = [](size_t i) { return pvar(i); };
    auto boolean = sym("Bool"), action = sym("ParserAction");
    auto yes = sym("true"), no = sym("false");

    sig["Bool"] = type_sort();
    sig["true"] = boolean;
    sig["false"] = boolean;
    sig["ParserAction"] = type_sort();
    for (const char* action_name : {"abortFragment", "schedulePump", "attemptEnd",
                                    "endIfDelayed", "scheduleEndIfDelayed", "idle"})
        sig[action_name] = action;

    sig["shouldPumpTokenizer"] = fn(boolean, fn(boolean, fn(boolean, boolean)));
    rules.push_back(rule(apps(sym("shouldPumpTokenizer"), yes, p(0), p(1)), no));
    rules.push_back(rule(apps(sym("shouldPumpTokenizer"), no, no, p(0)), yes));
    rules.push_back(rule(apps(sym("shouldPumpTokenizer"), no, yes, yes), yes));
    rules.push_back(rule(apps(sym("shouldPumpTokenizer"), no, yes, no), no));

    // Abstract the branch ordering in HTMLDocumentParser::PumpTokenizerIfPossible.
    // Fragment-parser abort is handled after tokenizer pumping and before the
    // yielded/EOF/delayed completion branches.
    Ptr resolve_type = action;
    for (int i = 0; i < 6; ++i) resolve_type = fn(boolean, resolve_type);
    sig["resolveParser"] = resolve_type;

    rules.push_back(rule(apps(sym("resolveParser"), yes, yes, p(0), p(1), p(2), p(3)),
                         sym("abortFragment")));
    rules.push_back(rule(apps(sym("resolveParser"), p(0), p(1), yes, p(2), p(3), p(4)),
                         sym("schedulePump")));
    rules.push_back(rule(apps(sym("resolveParser"), p(0), p(1), no, yes, p(2), p(3)),
                         sym("attemptEnd")));
    rules.push_back(rule(apps(sym("resolveParser"), p(0), p(1), no, no, yes, yes),
                         sym("endIfDelayed")));
    rules.push_back(rule(apps(sym("resolveParser"), p(0), p(1), no, no, yes, no),
                         sym("scheduleEndIfDelayed")));
    rules.push_back(rule(apps(sym("resolveParser"), p(0), p(1), no, no, no, p(2)),
                         sym("idle")));

    // Inputs are: fragment-abort-requested, outermost-pump-session,
    // tokenizer-yielded, should-attempt-EOF, should-end-if-delayed,
    // should-complete-or-stop.
    Ptr case_yield = apps(sym("resolveParser"), no, yes, yes, no, no, no);
    Ptr case_eof = apps(sym("resolveParser"), no, yes, no, yes, yes, yes);
    Ptr case_delayed = apps(sym("resolveParser"), no, yes, no, no, yes, yes);
    Ptr case_reschedule = apps(sym("resolveParser"), no, yes, no, no, yes, no);
    Ptr case_fragment_abort = apps(sym("resolveParser"), yes, yes, no, no, yes, yes);

    const std::vector<std::pair<Ptr, Ptr>> examples = {
        {case_yield, sym("schedulePump")},
        {case_eof, sym("attemptEnd")},
        {case_delayed, sym("endIfDelayed")},
        {case_reschedule, sym("scheduleEndIfDelayed")},
        {case_fragment_abort, sym("abortFragment")},
    };
    for (const auto& [input, expected] : examples) {
        auto inferred = infer({}, input, sig, rules);
        assert(inferred && conv(*inferred, action, rules));
        assert(conv(nf(input, rules), expected, rules));
    }

    const std::vector<std::pair<Ptr, Ptr>> pump_gates = {
        {apps(sym("shouldPumpTokenizer"), yes, no, yes), no},
        {apps(sym("shouldPumpTokenizer"), no, no, no), yes},
        {apps(sym("shouldPumpTokenizer"), no, yes, yes), yes},
        {apps(sym("shouldPumpTokenizer"), no, yes, no), no},
    };
    for (const auto& [input, expected] : pump_gates) {
        auto inferred = infer({}, input, sig, rules);
        assert(inferred && conv(*inferred, boolean, rules));
        assert(conv(nf(input, rules), expected, rules));
    }

    std::println("Blink parser lifecycle branch model: {} cases reduce to typed ParserAction terms",
                 examples.size());
    std::println("Tokenizer gate: {} cases reduce to Bool; tokenizer/tree-builder behavior is not modeled.",
                 pump_gates.size());
}
