#include "../src/lambda_pi.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

enum class Unit { A, B, CR, LF, Nul, Replacement, Eof };
using Stream = std::vector<Unit>;
enum class Op { Peek, Advance };
struct ReadResult { bool has_character; Unit character; Stream remaining; bool skip_next_lf; unsigned line; };
struct Case { Op op; Stream input; bool skip_next_lf; bool closed; bool skip_null; unsigned line; };

static ReadResult cpp_peek(const Stream& input, bool skip_next_lf,
                           bool closed, bool skip_null, unsigned line) {
  if (input.empty()) return {false, Unit::Eof, {}, skip_next_lf, line};
  const Unit c = input.front();
  if (c == Unit::CR) return {true, Unit::LF, input, true, line};
  if (c == Unit::LF && skip_next_lf) {
    Stream rest(input.begin()+1, input.end());
    return cpp_peek(rest, false, closed, skip_null, line+1);
  }
  if (c == Unit::Nul) {
    if (closed && input.size() == 1)
      return {true, Unit::Nul, input, false, line};
    if (skip_null) {
      Stream rest(input.begin()+1, input.end());
      return cpp_peek(rest, false, closed, skip_null, line);
    }
    return {true, Unit::Replacement, input, false, line};
  }
  return {true, c, input, false, line};
}

static ReadResult cpp_read(const Case& c) {
  if (c.op == Op::Peek)
    return cpp_peek(c.input, c.skip_next_lf, c.closed, c.skip_null, c.line);
  assert(!c.input.empty()); // Blink Advance requires a non-empty current segment.
  const Unit current = c.input.front();
  Stream rest(c.input.begin()+1, c.input.end());
  const unsigned line = c.line + (current == Unit::LF ? 1u : 0u);
  return cpp_peek(rest, c.skip_next_lf, c.closed, c.skip_null, line);
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }
static Ptr apps(Ptr f, std::initializer_list<Ptr> xs) {
  for (const auto& x : xs) f = app(f, x);
  return f;
}
static Ptr nat(unsigned n) { Ptr t=sym("zero"); while(n--) t=app(sym("succ"),t); return t; }
static const char* code_name(Unit u) { return u==Unit::B ? "codeB" : "codeA"; }
static Ptr raw_unit(Unit u) {
  switch(u) {
    case Unit::CR: return sym("rawCR");
    case Unit::LF: return sym("rawLF");
    case Unit::Nul: return sym("rawNul");
    default: return app(sym("rawOrdinary"),sym(code_name(u)));
  }
}
static Ptr output_unit(Unit u) {
  switch(u) {
    case Unit::LF: return sym("outLF");
    case Unit::Nul: return sym("outNul");
    case Unit::Replacement: return sym("replacement");
    case Unit::Eof: return sym("outEof");
    default: return app(sym("outOrdinary"),sym(code_name(u)));
  }
}
static Ptr raw_stream(const Stream& xs) {
  Ptr t=sym("nilRaw");
  for(auto i=xs.rbegin();i!=xs.rend();++i) t=apps(sym("consRaw"),{raw_unit(*i),t});
  return t;
}
static Ptr result_term(const ReadResult& r) {
  return apps(sym("readResult"),{sym(r.has_character?"yes":"no"),output_unit(r.character),
      raw_stream(r.remaining),sym(r.skip_next_lf?"yes":"no"),nat(r.line)});
}
static Ptr query(const Case& c) {
  const char* fn_name=c.op==Op::Peek?"peek":"advance";
  return apps(sym(fn_name),{sym(c.skip_next_lf?"yes":"no"),sym(c.closed?"yes":"no"),
      sym(c.skip_null?"yes":"no"),nat(c.line),raw_stream(c.input)});
}

int main() {
  const Ptr B=sym("Bool"), N=sym("Nat"), In=sym("InputUnit"), Raw=sym("RawUnit");
  const Ptr Out=sym("OutputUnit"), RI=sym("RawStream"), R=sym("ReadResult");
  Signature sig; std::vector<Rule> rules;
  sig["Bool"]=type_sort(); sig["yes"]=B; sig["no"]=B;
  sig["Nat"]=type_sort(); sig["zero"]=N; sig["succ"]=fn(N,N);
  sig["InputUnit"]=type_sort(); sig["codeA"]=In; sig["codeB"]=In;
  sig["RawUnit"]=type_sort(); sig["rawCR"]=Raw; sig["rawLF"]=Raw; sig["rawNul"]=Raw; sig["rawOrdinary"]=fn(In,Raw);
  sig["OutputUnit"]=type_sort(); sig["outLF"]=Out; sig["outNul"]=Out; sig["replacement"]=Out; sig["outEof"]=Out; sig["outOrdinary"]=fn(In,Out);
  sig["RawStream"]=type_sort(); sig["nilRaw"]=RI; sig["consRaw"]=fn(Raw,fn(RI,RI));
  sig["ReadResult"]=type_sort();
  sig["readResult"]=fn(B,fn(Out,fn(RI,fn(B,fn(N,R)))));
  sig["peekProcess"]=fn(B,fn(B,fn(B,fn(N,fn(RI,R)))));
  sig["peek"]=fn(B,fn(B,fn(B,fn(N,fn(RI,R)))));
  sig["advance"]=fn(B,fn(B,fn(B,fn(N,fn(RI,R)))));

  auto p=[](int i){return pvar(i);};
  // Empty source makes Peek fail. It preserves the pending-CRLF bit.
  rules.push_back(rule(apps(sym("peekProcess"),{p(0),p(1),p(2),p(3),sym("nilRaw")}),
      apps(sym("readResult"),{sym("no"),sym("outEof"),sym("nilRaw"),p(0),p(3)})));
  // CR is exposed as LF without moving the source cursor, then marks the
  // following raw LF for suppression on the next operation.
  rules.push_back(rule(apps(sym("peekProcess"),{p(0),p(1),p(2),p(3),apps(sym("consRaw"),{sym("rawCR"),p(4)})}),
      apps(sym("readResult"),{sym("yes"),sym("outLF"),apps(sym("consRaw"),{sym("rawCR"),p(4)}),sym("yes"),p(3)})));
  rules.push_back(rule(apps(sym("peekProcess"),{sym("no"),p(1),p(2),p(3),apps(sym("consRaw"),{sym("rawLF"),p(4)})}),
      apps(sym("readResult"),{sym("yes"),sym("outLF"),apps(sym("consRaw"),{sym("rawLF"),p(4)}),sym("no"),p(3)})));
  // A LF suppressed after CR is consumed and updates the line before peeking
  // again. This can return false at stream exhaustion.
  rules.push_back(rule(apps(sym("peekProcess"),{sym("yes"),p(1),p(2),p(3),apps(sym("consRaw"),{sym("rawLF"),p(4)})}),
      apps(sym("peekProcess"),{sym("no"),p(1),p(2),app(sym("succ"),p(3)),p(4)})));
  // A closed final NUL is left at the cursor and exposed as the EOF marker.
  rules.push_back(rule(apps(sym("peekProcess"),{p(0),sym("yes"),p(2),p(3),apps(sym("consRaw"),{sym("rawNul"),sym("nilRaw")})}),
      apps(sym("readResult"),{sym("yes"),sym("outNul"),apps(sym("consRaw"),{sym("rawNul"),sym("nilRaw")}),sym("no"),p(3)})));
  rules.push_back(rule(apps(sym("peekProcess"),{p(0),p(1),sym("no"),p(3),apps(sym("consRaw"),{sym("rawNul"),p(4)})}),
      apps(sym("readResult"),{sym("yes"),sym("replacement"),apps(sym("consRaw"),{sym("rawNul"),p(4)}),sym("no"),p(3)})));
  rules.push_back(rule(apps(sym("peekProcess"),{p(0),p(1),sym("yes"),p(3),apps(sym("consRaw"),{sym("rawNul"),p(4)})}),
      apps(sym("peekProcess"),{sym("no"),p(1),sym("yes"),p(3),p(4)})));
  // Ordinary units clear the CRLF suppression bit and do not consume on Peek.
  rules.push_back(rule(apps(sym("peekProcess"),{p(0),p(1),p(2),p(3),apps(sym("consRaw"),{app(sym("rawOrdinary"),p(4)),p(5)})}),
      apps(sym("readResult"),{sym("yes"),app(sym("outOrdinary"),p(4)),apps(sym("consRaw"),{app(sym("rawOrdinary"),p(4)),p(5)}),sym("no"),p(3)})));
  rules.push_back(rule(apps(sym("peek"),{p(0),p(1),p(2),p(3),p(4)}),apps(sym("peekProcess"),{p(0),p(1),p(2),p(3),p(4)})));
  // Advance consumes exactly one raw code unit; the consumed LF increments
  // the source line. The next-input result then uses the same Peek relation.
  rules.push_back(rule(apps(sym("advance"),{p(0),p(1),p(2),p(3),apps(sym("consRaw"),{sym("rawLF"),p(4)})}),
      apps(sym("peekProcess"),{p(0),p(1),p(2),app(sym("succ"),p(3)),p(4)})));
  for(const char* c:{"rawCR","rawNul"})
    rules.push_back(rule(apps(sym("advance"),{p(0),p(1),p(2),p(3),apps(sym("consRaw"),{sym(c),p(4)})}),
        apps(sym("peekProcess"),{p(0),p(1),p(2),p(3),p(4)})));
  rules.push_back(rule(apps(sym("advance"),{p(0),p(1),p(2),p(3),apps(sym("consRaw"),{app(sym("rawOrdinary"),p(4)),p(5)})}),
      apps(sym("peekProcess"),{p(0),p(1),p(2),p(3),p(5)})));

  const std::vector<Case> cases={
      {Op::Peek,{Unit::CR,Unit::LF,Unit::A},false,false,false,0},
      {Op::Advance,{Unit::CR,Unit::LF,Unit::A},true,false,false,0},
      {Op::Peek,{Unit::LF,Unit::A},true,false,false,0},
      {Op::Peek,{Unit::CR,Unit::A},false,false,false,4},
      {Op::Peek,{Unit::Nul,Unit::Nul,Unit::B},false,false,true,1},
      {Op::Peek,{Unit::Nul,Unit::B},false,false,false,0},
      {Op::Peek,{Unit::Nul},true,true,true,2},
      {Op::Advance,{Unit::LF,Unit::CR,Unit::LF,Unit::B},false,false,false,3},
      {Op::Advance,{Unit::A},false,false,false,5},
  };
  for(const auto& c:cases){
    const Ptr actual=nf(query(c),rules);
    const Ptr expected=result_term(cpp_read(c));
    assert(conv(actual,expected,rules));
  }
  std::cout << "Blink preprocessor cursor model: Peek and Advance rules encoded over "
            << cases.size() << " finite cursor observations; comparisons not run\n";
}
