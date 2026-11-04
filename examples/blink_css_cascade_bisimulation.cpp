#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <tuple>
#include <vector>

using namespace lp;

struct Candidate {
    // Order-preserving abstraction of a restricted Blink priority projection:
    // specificity, proximity (nearer sorts later), stylesheet, rule, declaration.
    std::array<unsigned, 5> rank;
    unsigned value;
};
using CandidateList = std::vector<Candidate>;

// Operational reference: scan matched declarations, retaining the greatest
// rank. This transcribes only winner selection for one property; it is not a
// call into Blink's linked StyleCascade implementation.
static unsigned cpp_winner(const CandidateList& declarations) {
    Candidate best{{0, 0, 0, 0, 0}, 0};
    bool have_best = false;
    for (const Candidate& candidate : declarations) {
        if (!have_best || candidate.rank >= best.rank) {
            best = candidate;
            have_best = true;
        }
    }
    return best.value;
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr nat(size_t n) {
    Ptr t = sym("zero");
    while (n--) t = app(sym("succ"), t);
    return t;
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
static Ptr priority_term(const std::array<unsigned, 5>& rank) {
    std::vector<Ptr> fields;
    for (unsigned field : rank) fields.push_back(nat(field));
    return roll(sym("priority"), fields);
}
static Ptr candidate_term(const Candidate& c) {
    return apps(sym("declaration"), priority_term(c.rank), nat(c.value));
}
static Ptr candidate_list_term(const CandidateList& cs) {
    Ptr result = sym("nilDecl");
    for (auto it = cs.rbegin(); it != cs.rend(); ++it)
        result = apps(sym("consDecl"), candidate_term(*it), result);
    return result;
}
static Ptr apps_list(Ptr f, const std::vector<Ptr>& xs) {
    return roll(std::move(f), xs);
}

int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto p = [](size_t i) { return pvar(i); };
    auto bit = sym("Bool"), number = sym("Nat"), cmp = sym("Ordering");
    auto candidates = sym("DeclList"), decl = sym("Declaration");
    auto state_type = number, pairs = sym("PairList");
    auto yes = sym("true"), no = sym("false");

    sig["Bool"] = type_sort(); sig["true"] = bit; sig["false"] = bit;
    sig["Nat"] = type_sort(); sig["zero"] = number;
    sig["succ"] = fn(number, number);
    sig["Ordering"] = type_sort();
    sig["lt"] = cmp; sig["eq"] = cmp; sig["gt"] = cmp;
    sig["Priority"] = type_sort();
    auto priority_type = number;
    for (int i = 0; i < 5; ++i) priority_type = fn(number, priority_type);
    sig["priority"] = priority_type;
    sig["Declaration"] = type_sort();
    sig["declaration"] = fn(sym("Priority"), fn(number, decl));
    sig["DeclList"] = type_sort(); sig["nilDecl"] = candidates;
    sig["consDecl"] = fn(decl, fn(candidates, candidates));
    sig["defaultDecl"] = decl;
    sig["compareNat"] = fn(number, fn(number, cmp));
    sig["thenCompare"] = fn(cmp, fn(cmp, cmp));
    sig["comparePriority"] = fn(sym("Priority"), fn(sym("Priority"), cmp));
    sig["chooseBy"] = fn(cmp, fn(decl, fn(decl, decl)));
    sig["chooseDecl"] = fn(decl, fn(decl, decl));
    sig["bestDecl"] = fn(candidates, decl);
    sig["valueOf"] = fn(decl, number);
    sig["candidatesAt"] = fn(state_type, candidates);
    sig["cppObserve"] = fn(state_type, number);
    sig["semanticObserve"] = fn(state_type, number);
    sig["cppNext"] = fn(state_type, state_type);
    sig["semanticNext"] = fn(state_type, state_type);
    sig["eqNat"] = fn(number, fn(number, bit));
    sig["andB"] = fn(bit, fn(bit, bit));
    sig["orB"] = fn(bit, fn(bit, bit));
    sig["memberPair"] = fn(pairs, fn(state_type, fn(state_type, bit)));
    sig["PairList"] = type_sort(); sig["nilPair"] = pairs;
    sig["pairCons"] = fn(state_type, fn(state_type, fn(pairs, pairs)));
    sig["checkPairs"] = fn(pairs, fn(pairs, bit));
    sig["bisimCheck"] = fn(pairs, fn(state_type, fn(state_type, bit)));

    // Peano comparison and lexicographic priority comparison.
    rules.push_back(rule(apps(sym("compareNat"), sym("zero"), sym("zero")), sym("eq")));
    rules.push_back(rule(apps(sym("compareNat"), sym("zero"), app(sym("succ"), p(0))), sym("lt")));
    rules.push_back(rule(apps(sym("compareNat"), app(sym("succ"), p(0)), sym("zero")), sym("gt")));
    rules.push_back(rule(apps(sym("compareNat"), app(sym("succ"), p(0)), app(sym("succ"), p(1))),
                         apps(sym("compareNat"), p(0), p(1))));
    rules.push_back(rule(apps(sym("thenCompare"), sym("lt"), p(0)), sym("lt")));
    rules.push_back(rule(apps(sym("thenCompare"), sym("gt"), p(0)), sym("gt")));
    rules.push_back(rule(apps(sym("thenCompare"), sym("eq"), p(0)), p(0)));
    std::vector<Ptr> lhs_a, lhs_b, comparisons;
    for (size_t i = 0; i < 5; ++i) {
        lhs_a.push_back(p(i)); lhs_b.push_back(p(i + 5));
        comparisons.push_back(apps(sym("compareNat"), p(i), p(i + 5)));
    }
    Ptr lex = comparisons.back();
    for (int i = 3; i >= 0; --i) lex = apps(sym("thenCompare"), comparisons[i], lex);
    rules.push_back(rule(apps(sym("comparePriority"),
                              apps_list(sym("priority"), lhs_a),
                              apps_list(sym("priority"), lhs_b)), lex));

    rules.push_back(rule(apps(sym("chooseBy"), sym("gt"), p(0), p(1)), p(0)));
    rules.push_back(rule(apps(sym("chooseBy"), sym("lt"), p(0), p(1)), p(1)));
    rules.push_back(rule(apps(sym("chooseBy"), sym("eq"), p(0), p(1)), p(1)));
    rules.push_back(rule(apps(sym("chooseDecl"),
                              apps(sym("declaration"), p(0), p(1)), sym("defaultDecl")),
                         apps(sym("declaration"), p(0), p(1))));
    rules.push_back(rule(apps(sym("chooseDecl"),
                              apps(sym("declaration"), p(0), p(1)),
                              apps(sym("declaration"), p(2), p(3))),
                         apps(sym("chooseBy"), apps(sym("comparePriority"), p(0), p(2)),
                              apps(sym("declaration"), p(0), p(1)),
                              apps(sym("declaration"), p(2), p(3)))));
    rules.push_back(rule(app(sym("bestDecl"), sym("nilDecl")), sym("defaultDecl")));
    rules.push_back(rule(app(sym("bestDecl"), apps(sym("consDecl"), p(0), p(1))),
                         apps(sym("chooseDecl"), p(0), app(sym("bestDecl"), p(1)))));
    rules.push_back(rule(app(sym("valueOf"), sym("defaultDecl")), sym("zero")));
    rules.push_back(rule(app(sym("valueOf"), apps(sym("declaration"), p(0), p(1))), p(1)));
    rules.push_back(rule(app(sym("semanticObserve"), p(0)),
                         app(sym("valueOf"), app(sym("bestDecl"), app(sym("candidatesAt"), p(0))))));

    // Five finite matched-rule sets exercise specificity, proximity, sheet
    // order, and declaration order. Origin/importance, tree order, inline style
    // and layer are fixed across this model slice.
    const std::vector<CandidateList> cases = {
        {{{1, 2, 0, 9, 0}, 1}, {{3, 2, 0, 1, 0}, 2}},
        {{{3, 2, 0, 2, 0}, 1}, {{3, 2, 0, 4, 0}, 2}},
        {{{3, 1, 1, 2, 0}, 1}, {{3, 3, 0, 9, 0}, 2}},
        {{{3, 2, 1, 1, 0}, 1}, {{3, 2, 2, 0, 0}, 2}},
        {{{3, 2, 0, 4, 1}, 1}, {{3, 2, 0, 4, 3}, 2}},
    };
    for (size_t i = 0; i < cases.size(); ++i) {
        Ptr id = nat(i);
        rules.push_back(rule(app(sym("candidatesAt"), id), candidate_list_term(cases[i])));
        const unsigned observed = cpp_winner(cases[i]);
        rules.push_back(rule(app(sym("cppObserve"), id), nat(observed)));
    }
    Ptr terminal_id = nat(cases.size());
    rules.push_back(rule(app(sym("candidatesAt"), terminal_id), sym("nilDecl")));
    rules.push_back(rule(app(sym("cppObserve"), terminal_id), sym("zero")));
    rules.push_back(rule(app(sym("cppNext"), p(0)), terminal_id));
    rules.push_back(rule(app(sym("semanticNext"), p(0)), terminal_id));

    rules.push_back(rule(apps(sym("eqNat"), sym("zero"), sym("zero")), yes));
    rules.push_back(rule(apps(sym("eqNat"), sym("zero"), app(sym("succ"), p(0))), no));
    rules.push_back(rule(apps(sym("eqNat"), app(sym("succ"), p(0)), sym("zero")), no));
    rules.push_back(rule(apps(sym("eqNat"), app(sym("succ"), p(0)), app(sym("succ"), p(1))),
                         apps(sym("eqNat"), p(0), p(1))));
    rules.push_back(rule(apps(sym("andB"), yes, yes), yes));
    rules.push_back(rule(apps(sym("andB"), yes, no), no));
    rules.push_back(rule(apps(sym("andB"), no, p(0)), no));
    rules.push_back(rule(apps(sym("orB"), yes, p(0)), yes));
    rules.push_back(rule(apps(sym("orB"), no, yes), yes));
    rules.push_back(rule(apps(sym("orB"), no, no), no));
    rules.push_back(rule(apps(sym("memberPair"), sym("nilPair"), p(0), p(1)), no));
    rules.push_back(rule(apps(sym("memberPair"), apps(sym("pairCons"), p(0), p(1), p(2)), p(3), p(4)),
        apps(sym("orB"), apps(sym("andB"), apps(sym("eqNat"), p(0), p(3)),
                               apps(sym("eqNat"), p(1), p(4))),
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

    Ptr relation = sym("nilPair");
    for (int id = static_cast<int>(cases.size()); id >= 0; --id) {
        Ptr state_id = nat(static_cast<size_t>(id));
        relation = apps(sym("pairCons"), state_id, state_id, relation);
    }
    Ptr initial = nat(0);
    Ptr check = apps(sym("bisimCheck"), relation, initial, initial);
    auto check_type = infer({}, check, sig, rules);
    assert(check_type && conv(*check_type, bit, rules));
    assert(conv(nf(check, rules), yes, rules));

    // Compare independent native scanning with the λΠ denotation for each case.
    for (size_t i = 0; i < cases.size(); ++i) {
        Ptr observed = nf(app(sym("semanticObserve"), nat(i)), rules);
        assert(nat_value(observed) == cpp_winner(cases[i]));
    }

    std::println("CSS cascade coalgebra: {} related states; λΠ closure check reduces to true", cases.size() + 1);
    std::println("Native C++ scan and λΠ max-priority semantics agree on {} matched-declaration cases", cases.size());
    std::println("Scope: one property, author-normal origin, one tree scope/layer; selectors are assumed pre-matched.");
}
