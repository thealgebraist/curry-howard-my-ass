#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <cstdint>

using namespace lp;

enum class Action : uint8_t {
    AbortFragment = 0,
    SchedulePump = 1,
    AttemptEnd = 2,
    EndIfDelayed = 3,
    ScheduleEndIfDelayed = 4,
    Idle = 5,
};

struct ParserInput {
    bool abort_requested;
    bool outermost_pump;
    bool yielded;
    bool attempt_eof;
    bool end_if_delayed;
    bool complete_or_stopped;
};

// C++ transcription of the selected decision boundary in
// HTMLDocumentParser::PumpTokenizerIfPossible at the pinned Chromium revision.
// It deliberately excludes tokenizer, task runner and document side effects.
static Action cpp_observe(ParserInput s) {
    if (s.abort_requested && s.outermost_pump) return Action::AbortFragment;
    if (s.yielded) return Action::SchedulePump;
    if (s.attempt_eof) return Action::AttemptEnd;
    if (s.end_if_delayed) {
        return s.complete_or_stopped ? Action::EndIfDelayed
                                    : Action::ScheduleEndIfDelayed;
    }
    return Action::Idle;
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Ptr curry(Ptr result, size_t n, Ptr arg) {
    while (n--) result = fn(arg, result);
    return result;
}
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr state(std::array<bool, 6> bits) {
    std::vector<Ptr> args;
    for (bool bit : bits) args.push_back(sym(bit ? "true" : "false"));
    return roll(sym("parserState"), args);
}
static Ptr nat(size_t n) {
    Ptr result = sym("zero");
    while (n--) result = app(sym("succ"), result);
    return result;
}
static size_t nat_value(Ptr t) {
    size_t n = 0;
    while (auto a = std::get_if<App>(&t->v)) {
        auto h = std::get_if<Sym>(&a->fn->v);
        if (!h || h->name != "succ") break;
        ++n;
        t = a->arg;
    }
    assert(std::get_if<Sym>(&t->v) && std::get<Sym>(t->v).name == "zero");
    return n;
}
static Ptr and_terms(std::vector<Ptr> terms) {
    Ptr result = sym("true");
    for (const auto& term : terms) result = apps(sym("andB"), term, result);
    return result;
}

int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto p = [](size_t i) { return pvar(i); };
    auto bit = sym("Bit"), number = sym("Nat"), state_type = sym("State");
    auto pairs = sym("PairList");
    auto yes = sym("true"), no = sym("false");

    sig["Bit"] = type_sort(); sig["true"] = bit; sig["false"] = bit;
    sig["Nat"] = type_sort(); sig["zero"] = number;
    sig["succ"] = fn(number, number);
    sig["State"] = type_sort(); sig["terminal"] = state_type;
    sig["parserState"] = curry(state_type, 6, bit);
    sig["PairList"] = type_sort(); sig["nilPair"] = pairs;
    sig["pairCons"] = fn(state_type, fn(state_type, fn(pairs, pairs)));
    sig["andB"] = fn(bit, fn(bit, bit));
    sig["orB"] = fn(bit, fn(bit, bit));
    sig["eqB"] = fn(bit, fn(bit, bit));
    sig["eqNat"] = fn(number, fn(number, bit));
    sig["eqState"] = fn(state_type, fn(state_type, bit));
    sig["memberPair"] = fn(pairs, fn(state_type, fn(state_type, bit)));
    sig["cppObserve"] = fn(state_type, number);
    sig["semanticObserve"] = fn(state_type, number);
    sig["cppNext"] = fn(state_type, state_type);
    sig["semanticNext"] = fn(state_type, state_type);
    sig["checkPairs"] = fn(pairs, fn(pairs, bit));
    sig["bisimCheck"] = fn(pairs, fn(state_type, fn(state_type, bit)));

    rules.push_back(rule(apps(sym("andB"), yes, yes), yes));
    rules.push_back(rule(apps(sym("andB"), yes, no), no));
    rules.push_back(rule(apps(sym("andB"), no, p(0)), no));
    rules.push_back(rule(apps(sym("orB"), yes, p(0)), yes));
    rules.push_back(rule(apps(sym("orB"), no, yes), yes));
    rules.push_back(rule(apps(sym("orB"), no, no), no));
    rules.push_back(rule(apps(sym("eqB"), yes, yes), yes));
    rules.push_back(rule(apps(sym("eqB"), no, no), yes));
    rules.push_back(rule(apps(sym("eqB"), yes, no), no));
    rules.push_back(rule(apps(sym("eqB"), no, yes), no));

    rules.push_back(rule(apps(sym("eqNat"), sym("zero"), sym("zero")), yes));
    rules.push_back(rule(apps(sym("eqNat"), sym("zero"), app(sym("succ"), p(0))), no));
    rules.push_back(rule(apps(sym("eqNat"), app(sym("succ"), p(0)), sym("zero")), no));
    rules.push_back(rule(apps(sym("eqNat"), app(sym("succ"), p(0)), app(sym("succ"), p(1))),
                         apps(sym("eqNat"), p(0), p(1))));

    rules.push_back(rule(apps(sym("eqState"), sym("terminal"), sym("terminal")), yes));
    Ptr arbitrary_state = roll(sym("parserState"),
                               {p(0), p(1), p(2), p(3), p(4), p(5)});
    rules.push_back(rule(apps(sym("eqState"), sym("terminal"), arbitrary_state), no));
    rules.push_back(rule(apps(sym("eqState"), arbitrary_state, sym("terminal")), no));
    std::vector<Ptr> bit_equalities;
    for (size_t i = 0; i < 6; ++i)
        bit_equalities.push_back(apps(sym("eqB"), p(2 * i), p(2 * i + 1)));
    rules.push_back(rule(apps(sym("eqState"), roll(sym("parserState"),
                                  {p(0), p(2), p(4), p(6), p(8), p(10)}),
                              roll(sym("parserState"),
                                  {p(1), p(3), p(5), p(7), p(9), p(11)})),
                         and_terms(bit_equalities)));

    // Reference coalgebra: priorities follow the observed C++ if/else order.
    auto parser_state = [&](Ptr a, Ptr outer, Ptr yielded, Ptr eof, Ptr delayed, Ptr complete) {
        return roll(sym("parserState"), {a, outer, yielded, eof, delayed, complete});
    };
    auto action_code = [](size_t i) { return nat(i); };
    rules.push_back(rule(app(sym("cppObserve"), parser_state(yes, yes, p(0), p(1), p(2), p(3))), action_code(0)));
    rules.push_back(rule(app(sym("cppObserve"), parser_state(p(0), p(1), yes, p(2), p(3), p(4))), action_code(1)));
    rules.push_back(rule(app(sym("cppObserve"), parser_state(p(0), p(1), no, yes, p(2), p(3))), action_code(2)));
    rules.push_back(rule(app(sym("cppObserve"), parser_state(p(0), p(1), no, no, yes, yes)), action_code(3)));
    rules.push_back(rule(app(sym("cppObserve"), parser_state(p(0), p(1), no, no, yes, no)), action_code(4)));
    rules.push_back(rule(app(sym("cppObserve"), parser_state(p(0), p(1), no, no, no, p(2))), action_code(5)));
    rules.push_back(rule(app(sym("cppObserve"), sym("terminal")), action_code(5)));

    // Semantic coalgebra: express the same priority as a denotational choice
    // tree rather than copying the C++ branch patterns.
    sig["ifB"] = fn(bit, fn(number, fn(number, number)));
    rules.push_back(rule(apps(sym("ifB"), yes, p(0), p(1)), p(0)));
    rules.push_back(rule(apps(sym("ifB"), no, p(0), p(1)), p(1)));
    Ptr semantic_body = apps(sym("ifB"), p(0),
        apps(sym("ifB"), p(5), action_code(3), action_code(4)), action_code(5));
    semantic_body = apps(sym("ifB"), p(1), action_code(2), semantic_body);
    semantic_body = apps(sym("ifB"), p(2), action_code(1), semantic_body);
    semantic_body = apps(sym("ifB"), apps(sym("andB"), p(3), p(4)), action_code(0), semantic_body);
    rules.push_back(rule(app(sym("semanticObserve"), parser_state(p(3), p(4), p(2), p(1), p(0), p(5))),
                         semantic_body));
    rules.push_back(rule(app(sym("semanticObserve"), sym("terminal")), action_code(5)));

    rules.push_back(rule(app(sym("cppNext"), p(0)), sym("terminal")));
    rules.push_back(rule(app(sym("semanticNext"), p(0)), sym("terminal")));
    rules.push_back(rule(apps(sym("memberPair"), p(0), sym("terminal"), sym("terminal")), yes));
    rules.push_back(rule(apps(sym("memberPair"), sym("nilPair"), p(0), p(1)), no));
    rules.push_back(rule(apps(sym("memberPair"), apps(sym("pairCons"), p(0), p(1), p(2)), p(3), p(4)),
        apps(sym("orB"), apps(sym("andB"), apps(sym("eqState"), p(0), p(3)),
                               apps(sym("eqState"), p(1), p(4))),
             apps(sym("memberPair"), p(2), p(3), p(4)))));
    rules.push_back(rule(apps(sym("checkPairs"), p(0), sym("nilPair")), yes));
    rules.push_back(rule(apps(sym("checkPairs"), p(0), apps(sym("pairCons"), p(1), p(2), p(3))),
        apps(sym("andB"), apps(sym("eqNat"), app(sym("cppObserve"), p(1)),
                                         app(sym("semanticObserve"), p(2))),
             apps(sym("andB"), apps(sym("memberPair"), p(0), app(sym("cppNext"), p(1)),
                                                       app(sym("semanticNext"), p(2))),
                  apps(sym("checkPairs"), p(0), p(3))))));
    rules.push_back(rule(apps(sym("bisimCheck"), p(0), p(1), p(2)),
        apps(sym("andB"), apps(sym("memberPair"), p(0), p(1), p(2)),
             apps(sym("checkPairs"), p(0), p(0)))));

    // Relation contains every valuation of the six finite control inputs,
    // related to the same valuation in the semantic machine, plus a terminal
    // pair. checkPairs verifies observation equality and successor closure.
    Ptr relation = sym("nilPair");
    for (unsigned mask = 1; mask < 64; ++mask) {
        std::array<bool, 6> flags{};
        for (unsigned i = 0; i < flags.size(); ++i) flags[i] = (mask & (1u << i)) != 0;
        Ptr q = state(flags);
        relation = apps(sym("pairCons"), q, q, relation);
    }
    Ptr initial = state({false, false, false, false, false, false});
    relation = apps(sym("pairCons"), initial, initial, relation);
    // The terminal pair receives a direct membership rewrite because every
    // modeled transition lands there; this avoids repeated scans of the full
    // finite relation during closure checking.
    relation = apps(sym("pairCons"), sym("terminal"), sym("terminal"), relation);
    Ptr verdict = apps(sym("bisimCheck"), relation, initial, initial);
    auto verdict_type = infer({}, verdict, sig, rules);
    assert(verdict_type && conv(*verdict_type, bit, rules));
    Ptr normalized_verdict = nf(verdict, rules);
    assert(conv(normalized_verdict, yes, rules));

    // An independent host-language execution checks the same 64 C++ inputs
    // against the λΠ semantic observer. This is a finite differential check;
    // the λΠ pair-list closure check is the bisimulation argument for this table.
    for (unsigned mask = 0; mask < 64; ++mask) {
        std::array<bool, 6> flags{};
        for (unsigned i = 0; i < flags.size(); ++i) flags[i] = (mask & (1u << i)) != 0;
        ParserInput in{flags[0], flags[1], flags[2], flags[3], flags[4], flags[5]};
        const size_t cpp_code = static_cast<size_t>(cpp_observe(in));
        const size_t encoded_cpp_code = nat_value(nf(app(sym("cppObserve"), state(flags)), rules));
        const size_t model_code = nat_value(nf(app(sym("semanticObserve"), state(flags)), rules));
        assert(cpp_code == encoded_cpp_code);
        assert(cpp_code == model_code);
    }

    std::println("Finite parser coalgebra bisimulation: {} related states, all observations and successors match",
                 65);
    std::println("Exhaustive C++/λΠ boundary comparison: 64 control-flag valuations matched");
    std::println("Scope: extracted decision fragment only; no claim about executing Chromium's renderer binary.");
}
