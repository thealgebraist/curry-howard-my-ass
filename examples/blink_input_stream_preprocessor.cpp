#include "../src/lambda_pi.hpp"

#include <cassert>
#include <iostream>
#include <tuple>
#include <vector>

using namespace lp;

enum class Unit { A, B, Wide, CR, LF, Nul, Replacement, Eof };
using Stream = std::vector<Unit>;

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
static Ptr input_term(bool skip_null, bool closed, const Stream& s) {
  return apps(sym("preprocess"), {sym(skip_null ? "yes" : "no"),
                                   sym(closed ? "yes" : "no"), raw_stream(s)});
}

int main() {
  const Ptr B = sym("Bool"), In = sym("InputUnit"), Raw = sym("RawUnit");
  const Ptr Out = sym("OutputUnit"), RI = sym("RawStream"), RO = sym("OutputStream");
  Signature sig;
  std::vector<Rule> rules;
  sig["Bool"] = type_sort(); sig["yes"] = B; sig["no"] = B;
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

  const std::vector<std::tuple<Stream, bool, bool>> cases = {
      {{Unit::A, Unit::CR, Unit::LF, Unit::B}, false, false},
      {{Unit::CR, Unit::CR, Unit::LF}, false, false},
      {{Unit::Nul, Unit::Wide, Unit::Nul}, false, false},
      {{Unit::Nul, Unit::A, Unit::Nul}, true, false},
      {{Unit::A, Unit::Nul}, false, true},
      {{Unit::Wide, Unit::Nul}, true, true},
      {{Unit::A, Unit::LF, Unit::B}, true, false},
  };
  for (const auto& [input, skip_null, closed] : cases) {
    const auto expected = cpp_preprocess(input, skip_null, closed);
    const Ptr observed = nf(input_term(skip_null, closed, input), rules);
    assert(conv(observed, out_stream(expected), rules));
  }
  std::cout << "Blink input-stream preprocessor model: " << cases.size()
            << " finite traces and a parametric ordinary-unit rule\n";
}
