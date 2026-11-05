#include "../src/lambda_pi.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

enum class Unit { A, B, CR, LF, Nul, Replacement, Eof };
using Stream = std::vector<Unit>;

static Stream cpp_preprocess(const Stream& input, bool skip_null, bool closed) {
  Stream out;
  for (size_t i = 0; i < input.size(); ++i) {
    const Unit c = input[i];
    if (c == Unit::CR) {
      out.push_back(Unit::LF);
      if (i + 1 < input.size() && input[i + 1] == Unit::LF) ++i;
    } else if (c == Unit::Nul) {
      if (closed && i + 1 == input.size()) {
        out.push_back(Unit::Eof);
      } else if (!skip_null) {
        out.push_back(Unit::Replacement);
      }
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

static const char* unit_name(Unit u) {
  switch (u) {
    case Unit::A: return "rawA";
    case Unit::B: return "rawB";
    case Unit::CR: return "rawCR";
    case Unit::LF: return "rawLF";
    case Unit::Nul: return "rawNul";
    case Unit::Replacement: return "replacement";
    case Unit::Eof: return "eofMarker";
  }
  return "rawA";
}

static Ptr stream_term(const Stream& s, bool output = false) {
  Ptr t = sym("nilStream");
  for (auto it = s.rbegin(); it != s.rend(); ++it)
    t = apps(sym("consStream"), {sym(unit_name(*it)), t});
  (void)output;
  return t;
}

static Ptr input_term(bool skip_null, bool closed, const Stream& s) {
  return apps(sym("preprocess"), {sym(skip_null ? "yes" : "no"),
                                   sym(closed ? "yes" : "no"), stream_term(s)});
}

int main() {
  const Ptr B = sym("Bool"), C = sym("StreamUnit"), L = sym("Stream");
  Signature sig;
  std::vector<Rule> rules;
  sig["Bool"] = type_sort(); sig["yes"] = B; sig["no"] = B;
  sig["StreamUnit"] = type_sort();
  for (const char* c : {"rawA", "rawB", "rawCR", "rawLF", "rawNul", "replacement", "eofMarker"}) sig[c] = C;
  sig["Stream"] = type_sort(); sig["nilStream"] = L;
  sig["consStream"] = fn(C, fn(L, L));
  sig["preprocess"] = fn(B, fn(B, fn(L, L)));

  // Variables 0/1/2 are skip-null, closed-stream, and input tail.
  rules.push_back(rule(apps(sym("preprocess"), {pvar(0), pvar(1), sym("nilStream")}), sym("nilStream")));
  // CR always becomes LF; an immediately following LF is consumed with it.
  rules.push_back(rule(apps(sym("preprocess"), {pvar(0), pvar(1), apps(sym("consStream"), {sym("rawCR"), apps(sym("consStream"), {sym("rawLF"), pvar(2)})})}),
                       apps(sym("consStream"), {sym("rawLF"), apps(sym("preprocess"), {pvar(0), pvar(1), pvar(2)})})));
  rules.push_back(rule(apps(sym("preprocess"), {pvar(0), pvar(1), apps(sym("consStream"), {sym("rawCR"), pvar(2)})}),
                       apps(sym("consStream"), {sym("rawLF"), apps(sym("preprocess"), {pvar(0), pvar(1), pvar(2)})})));
  // A closed stream whose only remaining code unit is NUL uses NUL as EOF marker.
  rules.push_back(rule(apps(sym("preprocess"), {pvar(0), sym("yes"), apps(sym("consStream"), {sym("rawNul"), sym("nilStream")})}),
                       apps(sym("consStream"), {sym("eofMarker"), sym("nilStream")})));
  // Otherwise NUL is either skipped or replaced by U+FFFD.
  rules.push_back(rule(apps(sym("preprocess"), {sym("yes"), pvar(1), apps(sym("consStream"), {sym("rawNul"), pvar(2)})}),
                       apps(sym("preprocess"), {sym("yes"), pvar(1), pvar(2)})));
  rules.push_back(rule(apps(sym("preprocess"), {sym("no"), pvar(1), apps(sym("consStream"), {sym("rawNul"), pvar(2)})}),
                       apps(sym("consStream"), {sym("replacement"), apps(sym("preprocess"), {sym("no"), pvar(1), pvar(2)})})));
  for (const char* c : {"rawA", "rawB", "rawLF"})
    rules.push_back(rule(apps(sym("preprocess"), {pvar(0), pvar(1), apps(sym("consStream"), {sym(c), pvar(2)})}),
                         apps(sym("consStream"), {sym(c), apps(sym("preprocess"), {pvar(0), pvar(1), pvar(2)})})));

  RuleIndex index(rules);
  const std::vector<std::tuple<Stream, bool, bool>> cases = {
      {{Unit::A, Unit::CR, Unit::LF, Unit::B}, false, false},
      {{Unit::CR, Unit::CR, Unit::LF}, false, false},
      {{Unit::Nul, Unit::A, Unit::Nul}, false, false},
      {{Unit::Nul, Unit::A, Unit::Nul}, true, false},
      {{Unit::A, Unit::Nul}, false, true},
      {{Unit::A, Unit::Nul}, true, true},
      {{Unit::A, Unit::LF, Unit::B}, true, false},
  };
  for (const auto& [input, skip_null, closed] : cases) {
    const auto expected = cpp_preprocess(input, skip_null, closed);
    const Ptr observed = nf(input_term(skip_null, closed, input), rules);
    const Ptr wanted = stream_term(expected, true);
    assert(conv(observed, wanted, rules));
  }
  std::cout << "Blink input-stream preprocessor model: " << cases.size()
            << " finite traces represented by C++ and λΠ terms\n";
}
