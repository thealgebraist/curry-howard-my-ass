#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <string>
#include <vector>

using namespace lp;

// Finite semantic projection of HTMLConstructionSite::PendingText and
// FlushPendingText. Text chunks are opaque atoms; mode ordering follows the
// source's min(whitespace_mode, new_whitespace_mode) operation.
enum class Mode : unsigned { Unknown = 0, NotAll = 1, All = 2, Newline = 3 };
enum class Sibling : unsigned { None, Text, Other };
struct Pending { unsigned parent, next; std::string data; Mode mode; };
struct Effect { enum Kind { Discard, Insert, Append } kind; unsigned parent; std::string data; };
struct Location { unsigned parent, next; };
struct Step { Effect flushed; Pending pending; bool did_flush; };

static Effect cpp_flush(const Pending& p, Sibling sibling, bool valid) {
  if (p.data.empty() || !valid) return {Effect::Discard, p.parent, {}};
  return {sibling == Sibling::Text ? Effect::Append : Effect::Insert, p.parent, p.data};
}
static Step cpp_append(Pending old, Location loc, const std::string& text, Mode mode,
                       Sibling old_sibling, bool old_valid) {
  if (old.data.empty()) return {{Effect::Discard, 0, {}}, {loc.parent,loc.next,text,mode}, false};
  if (old.parent == loc.parent && old.next == loc.next) {
    old.data += text;
    if (static_cast<unsigned>(mode) < static_cast<unsigned>(old.mode)) old.mode=mode;
    return {{Effect::Discard,0,{}},old,false};
  }
  Effect effect=cpp_flush(old,old_sibling,old_valid);
  return {effect,{loc.parent,loc.next,text,mode},true};
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr n(unsigned x){Ptr r=sym("z");while(x--)r=app(sym("s"),r);return r;}
static Ptr text(const std::string& s){return sym(s.empty()?"emptyText":s);}
static Ptr mode(Mode m){return sym(m==Mode::Unknown?"unknown":m==Mode::NotAll?"notAll":m==Mode::All?"allWhitespace":"newlineWhitespace");}
static Ptr sibling(Sibling s){return sym(s==Sibling::Text?"textSibling":s==Sibling::Other?"otherSibling":"noSibling");}
static Ptr pending(const Pending& p){return apps(sym("pending"),n(p.parent),n(p.next),text(p.data),mode(p.mode));}
static Ptr effect(const Effect& e){return apps(sym(e.kind==Effect::Discard?"discard":e.kind==Effect::Insert?"insertText":"appendText"),n(e.parent),text(e.data));}
static Ptr transition(const Step& s){return apps(sym(s.did_flush?"flushedThen":"retained"),effect(s.flushed),pending(s.pending));}
static Ptr app4(Ptr f,Ptr a,Ptr b,Ptr c,Ptr d){return apps(std::move(f),std::move(a),std::move(b),std::move(c),std::move(d));}
int main(){
  Signature sig; std::vector<Rule> rs; auto p=[](int i){return pvar(i);};
  Ptr Nat=sym("Nat"),Bool=sym("Bool"),Text=sym("Text"),ModeT=sym("Mode"),Sib=sym("Sibling"),PendingT=sym("Pending"),EffectT=sym("Effect"),StepT=sym("Step");
  sig["Nat"]=type_sort();sig["z"]=Nat;sig["s"]=fn(Nat,Nat);
  sig["Bool"]=type_sort();sig["yes"]=Bool;sig["no"]=Bool;
  sig["Text"]=type_sort();sig["emptyText"]=Text;for(auto x:{"a","b","ab","c","abc"})sig[x]=Text;sig["cat"]=fn(Text,fn(Text,Text));
  sig["Mode"]=type_sort();for(auto x:{"unknown","notAll","allWhitespace","newlineWhitespace"})sig[x]=ModeT;sig["minMode"]=fn(ModeT,fn(ModeT,ModeT));
  sig["Sibling"]=type_sort();for(auto x:{"noSibling","textSibling","otherSibling"})sig[x]=Sib;
  sig["Pending"]=type_sort();sig["pending"]=fn(Nat,fn(Nat,fn(Text,fn(ModeT,PendingT))));
  sig["Effect"]=type_sort();for(auto x:{"discard","insertText","appendText"})sig[x]=fn(Nat,fn(Text,EffectT));
  sig["Step"]=type_sort();sig["retained"]=fn(EffectT,fn(PendingT,StepT));sig["flushedThen"]=fn(EffectT,fn(PendingT,StepT));
  sig["isEmpty"]=fn(Text,Bool);sig["valid"]=fn(Bool,Bool);sig["isSameLocation"]=fn(Nat,fn(Nat,fn(Nat,fn(Nat,Bool))));
  sig["flush"]=fn(PendingT,fn(Sib,fn(Bool,EffectT)));
  sig["appendAt"]=fn(PendingT,fn(Nat,fn(Nat,fn(Text,fn(ModeT,fn(Sib,fn(Bool,StepT)))))));
  sig["flushEffect"]=fn(PendingT,fn(Sib,fn(Bool,EffectT)));
  // FlushPendingText: empty or invalid locations are discarded; otherwise the
  // previous Text sibling receives ParserAppendData, or a new Text is inserted.
  rs.push_back(rule(app(sym("isEmpty"),sym("emptyText")),sym("yes")));
  for(auto x:{"a","b","ab","c","abc"})rs.push_back(rule(app(sym("isEmpty"),sym(x)),sym("no")));
  rs.push_back(rule(app(sym("valid"),sym("yes")),sym("yes")));rs.push_back(rule(app(sym("valid"),sym("no")),sym("no")));
  rs.push_back(rule(apps(sym("flush"),p(0),p(1),p(2)),apps(sym("flushEffect"),p(0),p(1),p(2))));
  rs.push_back(rule(apps(sym("flushEffect"),app4(sym("pending"),p(0),p(1),sym("emptyText"),p(2)),p(3),p(4)),apps(sym("discard"),p(0),sym("emptyText"))));
  rs.push_back(rule(apps(sym("flushEffect"),p(0),p(1),sym("no")),apps(sym("discard"),n(0),sym("emptyText"))));
  rs.push_back(rule(apps(sym("flushEffect"),app4(sym("pending"),p(0),p(1),p(2),p(3)),sym("textSibling"),sym("yes")),apps(sym("appendText"),p(0),p(2))));
  rs.push_back(rule(apps(sym("flushEffect"),app4(sym("pending"),p(0),p(1),p(2),p(3)),p(4),sym("yes")),apps(sym("insertText"),p(0),p(2))));
  rs.push_back(rule(apps(sym("minMode"),sym("unknown"),p(0)),p(0)));
  rs.push_back(rule(apps(sym("minMode"),p(0),sym("unknown")),p(0)));
  for(Mode a:{Mode::NotAll,Mode::All,Mode::Newline})for(Mode b:{Mode::NotAll,Mode::All,Mode::Newline})
    rs.push_back(rule(apps(sym("minMode"),mode(a),mode(b)),mode(static_cast<unsigned>(a)<static_cast<unsigned>(b)?a:b)));
  // Explicit finite location equality supports parent and next-child identities.
  for(unsigned a=0;a<2;++a)for(unsigned b=0;b<2;++b)for(unsigned c=0;c<2;++c)for(unsigned d=0;d<2;++d)
    rs.push_back(rule(apps(sym("isSameLocation"),n(a),n(b),n(c),n(d)),sym(a==c&&b==d?"yes":"no")));
  Ptr emptyPending=app4(sym("pending"),p(0),p(1),sym("emptyText"),p(2));
  Ptr appendPattern=apps(sym("appendAt"),emptyPending,p(3),p(4),p(5),p(6),p(7),p(8));
  rs.push_back(rule(appendPattern,apps(sym("retained"),apps(sym("discard"),n(0),sym("emptyText")),app4(sym("pending"),p(3),p(4),p(5),p(6)))));
  Ptr old=app4(sym("pending"),p(0),p(1),p(2),p(3));
  Ptr same=apps(sym("isSameLocation"),p(0),p(1),p(4),p(5));
  rs.push_back(rule(apps(sym("appendAt"),old,p(4),p(5),p(6),p(7),p(8),p(9)),
    apps(sym("chooseSame"),same,old,p(4),p(5),p(6),p(7),p(8),p(9))));
  sig["chooseSame"]=fn(Bool,fn(PendingT,fn(Nat,fn(Nat,fn(Text,fn(ModeT,fn(Sib,fn(Bool,StepT))))))));
  rs.push_back(rule(apps(sym("chooseSame"),sym("yes"),old,p(4),p(5),p(6),p(7),p(8),p(9)),
    apps(sym("retained"),apps(sym("discard"),n(0),sym("emptyText")),app4(sym("pending"),p(4),p(5),apps(sym("cat"),p(2),p(6)),apps(sym("minMode"),p(3),p(7))))));
  rs.push_back(rule(apps(sym("chooseSame"),sym("no"),old,p(4),p(5),p(6),p(7),p(8),p(9)),
    apps(sym("flushedThen"),apps(sym("flush"),old,p(8),p(9)),app4(sym("pending"),p(4),p(5),p(6),p(7)))));
  rs.push_back(rule(apps(sym("cat"),sym("emptyText"),p(0)),p(0)));
  rs.push_back(rule(apps(sym("cat"),sym("a"),sym("b")),sym("ab")));
  rs.push_back(rule(apps(sym("cat"),sym("ab"),sym("c")),sym("abc")));

  RuleIndex ix(rs);
  struct Case { Pending old; Location loc; std::string chunk; Mode mode; Sibling sibling; bool valid; };
  const std::array<Case,7> cases={{{{0,0,"",Mode::Unknown},{0,0},"a",Mode::NotAll,Sibling::None,true},
    {{0,0,"a",Mode::NotAll},{0,0},"b",Mode::All,Sibling::None,true},
    {{0,0,"a",Mode::All},{1,0},"b",Mode::NotAll,Sibling::Text,true},
    {{0,0,"a",Mode::All},{0,1},"b",Mode::NotAll,Sibling::Other,true},
    {{0,0,"a",Mode::All},{1,0},"b",Mode::NotAll,Sibling::None,false},
    {{0,0,"",Mode::Unknown},{1,0},"c",Mode::Newline,Sibling::Text,true},
    {{0,0,"ab",Mode::NotAll},{1,0},"c",Mode::All,Sibling::Other,true}}};
  for(const auto& x:cases){
    Step native=cpp_append(x.old,x.loc,x.chunk,x.mode,x.sibling,x.valid);
    Ptr input=apps(sym("appendAt"),pending(x.old),n(x.loc.parent),n(x.loc.next),text(x.chunk),mode(x.mode),sibling(x.sibling),sym(x.valid?"yes":"no"));
    Ptr got=nf(input,ix);Ptr expected=transition(native);
    auto ty=infer({},input,sig,ix);assert(ty&&conv(*ty,StepT,ix));if(!conv(got,expected,ix)){std::println("mismatch expected={} actual={}",to_string(expected),to_string(got));return 2;}
  }
  std::println("Blink PendingText append/flush projection: {} finite location, sibling, validity, and whitespace-mode transitions agree in typed λΠ",cases.size());
  std::println("Scope: source-level projection of PendingText::Append and FlushPendingText. String chunks are finite opaque atoms; template/foster insertion adjustment, String canonicalization, actual Node identity, and task-queue ordering are not modeled.");
}
