#include "../src/lambda_pi.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <tuple>
#include <vector>

using namespace lp;

enum class Origin : unsigned {
  None = 0,
  UserAgent = 1,
  User = 2,
  PresentationalHint = 3,
  Author = 4,
  Animation = 5,
  Transition = 16,
};

struct Candidate {
  Origin origin;
  bool important;
  unsigned rule_index;
  unsigned value;
};

struct Case {
  Origin current_origin;
  bool current_important;
  unsigned current_rule_index;
  std::vector<Candidate> candidates;
};

static unsigned encode_origin(Origin origin, bool important) {
  const auto code = static_cast<unsigned>(origin);
  return important ? code ^ 0xfu : code;
}

static auto priority_key(const Candidate& candidate) {
  // Other CascadePriority fields are fixed to zero in this projection.
  return std::tuple(encode_origin(candidate.origin, candidate.important),
                    candidate.rule_index);
}

static Origin target_origin(Origin origin) {
  switch (origin) {
    case Origin::UserAgent: return Origin::None;
    case Origin::User: return Origin::UserAgent;
    case Origin::PresentationalHint:
    case Origin::Author:
    case Origin::Animation: return Origin::User;
    case Origin::None:
    case Origin::Transition: assert(false && "Blink marks these unreachable");
  }
  assert(false);
  return Origin::None;
}

static const Candidate* find_for_revert(const std::vector<Candidate>& sorted,
                                        Origin target) {
  for (const auto& candidate : sorted) {
    if (static_cast<unsigned>(target) >=
        static_cast<unsigned>(candidate.origin)) {
      return &candidate;
    }
  }
  return nullptr;
}

struct Result {
  bool unset = true;
  Origin origin = Origin::None;
  unsigned value = 0;
};

static Result cpp_resolve_to(const Candidate* candidate,
                             const Candidate& current) {
  if (!candidate || candidate->origin == Origin::None ||
      priority_key(*candidate) >= priority_key(current)) {
    return {};
  }
  return {false, candidate->origin, candidate->value};
}

static Result cpp_revert(const Case& input) {
  auto candidates = input.candidates;
  std::sort(candidates.begin(), candidates.end(), [](const auto& a,
                                                      const auto& b) {
    return priority_key(a) > priority_key(b);
  });
  const Candidate current{input.current_origin, input.current_important,
                          input.current_rule_index, 0};
  return cpp_resolve_to(find_for_revert(candidates,
                                        target_origin(input.current_origin)),
                        current);
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Ptr ap(Ptr f, std::initializer_list<Ptr> args) {
  for (const auto& arg : args) f = app(f, arg);
  return f;
}
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr nat(unsigned value) {
  Ptr out = sym("z");
  while (value--) out = app(sym("s"), out);
  return out;
}
static Ptr boolean(bool value) { return sym(value ? "yes" : "no"); }
static Ptr candidate_term(const Candidate& c) {
  return ap(sym("candidate"), {nat(static_cast<unsigned>(c.origin)),
                                boolean(c.important), nat(c.rule_index),
                                nat(c.value)});
}
static Ptr candidates_term(std::vector<Candidate> candidates) {
  std::sort(candidates.begin(), candidates.end(), [](const auto& a,
                                                      const auto& b) {
    return priority_key(a) > priority_key(b);
  });
  Ptr list = sym("nilCandidate");
  for (auto it = candidates.rbegin(); it != candidates.rend(); ++it) {
    list = ap(sym("consCandidate"), {candidate_term(*it), list});
  }
  return list;
}
static Ptr value_term(const Result& result) {
  return result.unset ? sym("unsetResult")
                      : ap(sym("resolvedResult"),
                           {nat(static_cast<unsigned>(result.origin)),
                            nat(result.value)});
}

int main() {
  const Origin UA = Origin::UserAgent;
  const Origin User = Origin::User;
  const Origin Hint = Origin::PresentationalHint;
  const Origin Author = Origin::Author;
  const Origin Animation = Origin::Animation;

  const std::vector<Case> cases = {
      {UA, false, 1, {{UA, false, 0, 11}, {Origin::None, false, 0, 99}}},
      {User, false, 4, {{User, false, 7, 20}, {UA, false, 1, 11}}},
      {Author, false, 8,
       {{Author, false, 9, 30}, {User, false, 3, 20}, {UA, false, 1, 10}}},
      {Hint, false, 5,
       {{Author, false, 7, 31}, {User, false, 2, 21}, {UA, false, 1, 11}}},
      {Animation, false, 6,
       {{Animation, false, 8, 32}, {User, false, 3, 22}}},
      {Author, true, 5,
       {{User, true, 1, 41}, {User, false, 7, 21}, {UA, false, 1, 11}}},
      {Author, false, 8, {{User, false, 3, 22}}},
      {Author, false, 8, {}},
  };

  Signature sig;
  std::vector<Rule> rules;
  auto p = [](size_t i) { return pvar(i); };
  const Ptr N = sym("Nat"), B = sym("Bool"), O = sym("Ordering");
  const Ptr C = sym("Candidate"), CL = sym("CandidateList");
  const Ptr R = sym("Result");
  sig["Nat"] = type_sort(); sig["z"] = N; sig["s"] = fn(N, N);
  sig["Bool"] = type_sort(); sig["no"] = B; sig["yes"] = B;
  sig["Ordering"] = type_sort();
  for (const char* name : {"less", "equal", "greater"}) sig[name] = O;
  sig["Candidate"] = type_sort();
  sig["candidate"] = fn(N, fn(B, fn(N, fn(N, C))));
  sig["CandidateList"] = type_sort(); sig["nilCandidate"] = CL;
  sig["consCandidate"] = fn(C, fn(CL, CL));
  sig["Result"] = type_sort(); sig["unsetResult"] = R;
  sig["resolvedResult"] = fn(N, fn(N, R));
  sig["encodeOrigin"] = fn(N, fn(B, N));
  sig["targetOrigin"] = fn(N, N);
  sig["cmpNat"] = fn(N, fn(N, O));
  sig["cmpBool"] = fn(B, fn(B, O));
  sig["thenCmp"] = fn(O, fn(O, O));
  sig["priority"] = fn(N, fn(B, fn(N, sym("Priority"))));
  sig["Priority"] = type_sort();
  sig["cmpPriority"] = fn(sym("Priority"), fn(sym("Priority"), O));
  sig["originOf"] = fn(C, N);
  sig["priorityOf"] = fn(C, sym("Priority"));
  sig["valueOf"] = fn(C, N);
  sig["findOrigin"] = fn(CL, fn(N, C));
  sig["findStep"] = fn(O, fn(C, fn(CL, fn(N, C))));
  sig["resolveTo"] = fn(C, fn(sym("Priority"), R));
  sig["resolveStep"] = fn(O, fn(C, R));
  sig["revert"] = fn(CL, fn(N, fn(B, fn(N, R))));
  sig["caseAt"] = fn(N, R);
  sig["observe"] = fn(N, R);

  auto yes = sym("yes"), no = sym("no");
  // The encoded origin/importance field is the only varying high priority
  // field in these fixtures; rule index is the only varying lower field.
  for (unsigned code : {0u, 1u, 2u, 3u, 4u, 5u, 16u}) {
    rules.push_back(rule(ap(sym("encodeOrigin"), {nat(code), no}), nat(code)));
    rules.push_back(rule(ap(sym("encodeOrigin"), {nat(code), yes}),
                         nat(code ^ 0xfu)));
  }
  rules.push_back(rule(ap(sym("targetOrigin"), {nat(1)}), nat(0)));
  rules.push_back(rule(ap(sym("targetOrigin"), {nat(2)}), nat(1)));
  for (unsigned code : {3u, 4u, 5u})
    rules.push_back(rule(ap(sym("targetOrigin"), {nat(code)}), nat(2)));

  rules.push_back(rule(ap(sym("cmpNat"), {sym("z"), sym("z")}), sym("equal")));
  rules.push_back(rule(ap(sym("cmpNat"), {sym("z"), app(sym("s"), p(0))}),
                         sym("less")));
  rules.push_back(rule(ap(sym("cmpNat"), {app(sym("s"), p(0)), sym("z")}),
                         sym("greater")));
  rules.push_back(rule(ap(sym("cmpNat"),
                          {app(sym("s"), p(0)), app(sym("s"), p(1))}),
                         ap(sym("cmpNat"), {p(0), p(1)})));
  rules.push_back(rule(ap(sym("cmpBool"), {no, no}), sym("equal")));
  rules.push_back(rule(ap(sym("cmpBool"), {yes, yes}), sym("equal")));
  rules.push_back(rule(ap(sym("cmpBool"), {no, yes}), sym("less")));
  rules.push_back(rule(ap(sym("cmpBool"), {yes, no}), sym("greater")));
  for (const char* first : {"less", "equal", "greater"})
    for (const char* second : {"less", "equal", "greater"})
      rules.push_back(rule(ap(sym("thenCmp"), {sym(first), sym(second)}),
                           sym(std::string(first) != "equal" ? first : second)));
  rules.push_back(rule(
      ap(sym("priority"), {p(0), p(1), p(2)}),
      ap(sym("priorityKey"), {ap(sym("encodeOrigin"), {p(0), p(1)}), p(2)})));
  sig["priorityKey"] = fn(N, fn(N, sym("Priority")));
  rules.push_back(rule(ap(sym("cmpPriority"),
                          {ap(sym("priorityKey"), {p(0), p(1)}),
                           ap(sym("priorityKey"), {p(2), p(3)})}),
                       ap(sym("thenCmp"), {ap(sym("cmpNat"), {p(0), p(2)}),
                                            ap(sym("cmpNat"), {p(1), p(3)})})));
  rules.push_back(rule(ap(sym("originOf"),
                          {ap(sym("candidate"), {p(0), p(1), p(2), p(3)})}),
                       p(0)));
  rules.push_back(rule(ap(sym("priorityOf"),
                          {ap(sym("candidate"), {p(0), p(1), p(2), p(3)})}),
                       ap(sym("priority"), {p(0), p(1), p(2)})));
  rules.push_back(rule(ap(sym("valueOf"),
                          {ap(sym("candidate"), {p(0), p(1), p(2), p(3)})}),
                       p(3)));

  rules.push_back(rule(ap(sym("findOrigin"), {sym("nilCandidate"), p(0)}),
                       sym("noCandidate")));
  sig["noCandidate"] = C;
  rules.push_back(rule(ap(sym("findOrigin"),
                          {ap(sym("consCandidate"), {p(0), p(1)}), p(2)}),
                       ap(sym("findStep"),
                          {ap(sym("cmpNat"), {p(2), app(sym("originOf"), p(0))}),
                           p(0), p(1), p(2)})));
  rules.push_back(rule(ap(sym("findStep"), {sym("less"), p(0), p(1), p(2)}),
                       ap(sym("findOrigin"), {p(1), p(2)})));
  rules.push_back(rule(ap(sym("findStep"), {sym("equal"), p(0), p(1), p(2)}),
                       p(0)));
  rules.push_back(rule(ap(sym("findStep"), {sym("greater"), p(0), p(1), p(2)}),
                       p(0)));
  rules.push_back(rule(ap(sym("resolveTo"), {sym("noCandidate"), p(0)}),
                       sym("unsetResult")));
  rules.push_back(rule(
      ap(sym("resolveTo"),
         {ap(sym("candidate"), {nat(0), p(0), p(1), p(2)}), p(3)}),
      sym("unsetResult")));
  rules.push_back(rule(ap(sym("resolveTo"),
                          {ap(sym("candidate"), {p(0), p(1), p(2), p(3)}), p(4)}),
                       ap(sym("resolveStep"),
                          {ap(sym("cmpPriority"),
                              {ap(sym("priority"), {p(0), p(1), p(2)}), p(4)}),
                           ap(sym("candidate"), {p(0), p(1), p(2), p(3)}),
                           sym("unsetResult")})));
  rules.push_back(rule(ap(sym("resolveStep"), {sym("less"), p(0), p(1)}),
                       ap(sym("resolvedResult"),
                          {app(sym("originOf"), p(0)), app(sym("valueOf"), p(0))})));
  rules.push_back(rule(ap(sym("resolveStep"), {sym("equal"), p(0), p(1)}),
                       p(1)));
  rules.push_back(rule(ap(sym("resolveStep"), {sym("greater"), p(0), p(1)}),
                       p(1)));
  rules.push_back(rule(ap(sym("revert"), {p(0), p(1), p(2), p(3)}),
                       ap(sym("resolveTo"),
                          {ap(sym("findOrigin"),
                              {p(0), app(sym("targetOrigin"), p(1))}),
                           ap(sym("priority"), {p(1), p(2), p(3)})})));
  for (size_t i = 0; i < cases.size(); ++i) {
    rules.push_back(rule(app(sym("caseAt"), nat(i)),
                         ap(sym("revert"),
                            {candidates_term(cases[i].candidates),
                             nat(static_cast<unsigned>(cases[i].current_origin)),
                             boolean(cases[i].current_important),
                             nat(cases[i].current_rule_index)})));
  }
  rules.push_back(rule(app(sym("observe"), p(0)), app(sym("caseAt"), p(0))));

  for (size_t i = 0; i < cases.size(); ++i) {
    const Result expected = cpp_revert(cases[i]);
    const Ptr term = app(sym("observe"), nat(static_cast<unsigned>(i)));
    auto type = infer({}, term, sig, rules);
    assert(type && conv(*type, R, rules));
    Ptr actual = nf(term, rules);
    Ptr expected_term = value_term(expected);
    if (!conv(actual, expected_term, rules)) {
      std::println("case {} mismatch: actual={} expected={}", i,
                   to_string(actual), to_string(expected_term));
      return 2;
    }
  }
  unsigned fallback_cases = 0;
  const Candidate current{Author, false, 8, 0};
  for (const Candidate candidate : {
           Candidate{Author, false, 8, 51},
           Candidate{Author, false, 9, 52},
           Candidate{Author, false, 7, 53},
       }) {
    const Ptr current_priority = ap(
        sym("priority"),
        {nat(static_cast<unsigned>(current.origin)),
         boolean(current.important), nat(current.rule_index)});
    const Ptr term = ap(sym("resolveTo"),
                        {candidate_term(candidate), current_priority});
    auto type = infer({}, term, sig, rules);
    assert(type && conv(*type, R, rules));
    Ptr actual = nf(term, rules);
    Ptr expected = value_term(cpp_resolve_to(&candidate, current));
    assert(conv(actual, expected, rules));
    ++fallback_cases;
  }
  std::println("Blink CSS revert origin scan agrees with the C++ reference on {} finite cases",
               cases.size() + fallback_cases);
  std::println("Scope: fixed tree/layer/flag fields; models origin target, map filtering, and candidate priority fallback.");
}
