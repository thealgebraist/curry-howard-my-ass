#include "../src/lambda_pi.hpp"
#include <cassert>
#include <cstdint>
#include <string_view>
#include <vector>

using namespace lp;
enum class Mode { Initial, Number, HexPrefix, Dec, Hex, Done, Await };
struct Result { Mode mode; uint32_t value; bool consumed_terminator; size_t pos; };
static int dec(char c){return c>='0'&&c<='9'?c-'0':-1;}
static int hexv(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
static uint32_t legal(uint32_t v){static constexpr uint32_t win[32]={0x20ac,0x81,0x201a,0x192,0x201e,0x2026,0x2020,0x2021,0x2c6,0x2030,0x160,0x2039,0x152,0x8d,0x17d,0x8f,0x90,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x2dc,0x2122,0x161,0x203a,0x153,0x9d,0x17e,0x178};if(v==0||v>0x10ffff||(v>=0xd800&&v<=0xdfff))return 0xfffd;if(v>=0x80&&v<=0x9f)return win[v-0x80];return v;}
enum class LegalClass { Zero, Surrogate, AboveMax, Win80, Win9f, Ordinary };
static const char* lc(LegalClass x){switch(x){case LegalClass::Zero:return"zeroPoint";case LegalClass::Surrogate:return"surrogatePoint";case LegalClass::AboveMax:return"aboveMaxPoint";case LegalClass::Win80:return"win80Point";case LegalClass::Win9f:return"win9fPoint";default:return"ordinaryPoint";}}
static const char* mapped(LegalClass x){switch(x){case LegalClass::Zero:case LegalClass::Surrogate:case LegalClass::AboveMax:return"replacementPoint";case LegalClass::Win80:return"euroPoint";case LegalClass::Win9f:return"yDiaeresisPoint";default:return"ordinaryResult";}}
static Result cpp_scan(std::string_view in){Mode m=Mode::Initial;uint32_t v=0;size_t i=0;
 while(i<in.size()) {char c=in[i];switch(m){
  case Mode::Initial: if(c=='#'){m=Mode::Number;++i;continue;} return {Mode::Done,0,false,i};
  case Mode::Number: if(c=='x'||c=='X'){m=Mode::HexPrefix;++i;continue;} if(dec(c)>=0){m=Mode::Dec;continue;} return {Mode::Done,0,false,0};
  case Mode::HexPrefix: if(hexv(c)>=0){m=Mode::Hex;continue;} return {Mode::Done,0,false,0};
  case Mode::Dec: if(dec(c)>=0){v=v*10+static_cast<unsigned>(dec(c));++i;continue;} if(c==';'){++i;return {Mode::Done,legal(v),true,i};} return {Mode::Done,legal(v),false,i};
  case Mode::Hex: if(hexv(c)>=0){v=v*16+static_cast<unsigned>(hexv(c));++i;continue;} if(c==';'){++i;return {Mode::Done,legal(v),true,i};} return {Mode::Done,legal(v),false,i};
  default: return {m,v,false,i};
 }} if(m==Mode::Number||m==Mode::HexPrefix||m==Mode::Dec||m==Mode::Hex)return {Mode::Await,0,false,0};return {Mode::Await,v,false,i};
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));} static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr r=sym("z");while(n--)r=app(sym("s"),r);return r;}
static Ptr mode(Mode m){static const char* n[]={"initial","number","hexPrefix","decimal","hexadecimal","done","await"};return sym(n[static_cast<unsigned>(m)]);}
static Ptr digit(char c){if(c>='0'&&c<='9')return sym((std::string("d")+c).c_str());if(c>='a'&&c<='f')return sym((std::string("h")+char(c-'a'+'A')).c_str());if(c>='A'&&c<='F')return sym((std::string("h")+c).c_str());if(c=='#')return sym("hash");if(c=='x')return sym("lowerX");if(c=='X')return sym("upperX");if(c==';')return sym("semi");return sym("other");}
static Ptr input_term(std::string_view s){Ptr r=sym("end");for(auto i=s.rbegin();i!=s.rend();++i)r=apps(sym("cons"),digit(*i),r);return r;}
static Ptr output(const Result& r){return apps(sym("result"),mode(r.mode),nat(r.value),sym(r.consumed_terminator?"true":"false"),nat(static_cast<unsigned>(r.pos)));}
int main(){
 Signature sig;std::vector<Rule> rs;auto p=[](int i){return pvar(i);};Ptr n=sym("Nat"),d=sym("Digit"),inp=sym("Input"),state=sym("ScanState"),bit=sym("Bool");
 sig["Nat"]=type_sort();sig["z"]=n;sig["s"]=fn(n,n);sig["Bool"]=type_sort();sig["true"]=bit;sig["false"]=bit;
 for(const char* x:{"initial","number","hexPrefix","decimal","hexadecimal","done","await"})sig[x]=sym("Mode");sig["Mode"]=type_sort();
 sig["Digit"]=type_sort();sig["Input"]=type_sort();sig["end"]=inp;sig["cons"]=fn(d,fn(inp,inp));
 std::vector<std::pair<const char*,unsigned>> ds={{"d0",0},{"d1",1},{"d2",2},{"d3",3},{"d4",4},{"d5",5},{"d6",6},{"d7",7},{"d8",8},{"d9",9},{"hA",10},{"hB",11},{"hC",12},{"hD",13},{"hE",14},{"hF",15}};
 for(auto [name,v]:ds){sig[name]=d;sig[(std::string("val")+name).c_str()]=n;}
 for(const char* x:{"hash","lowerX","upperX","semi","other"})sig[x]=d;
 sig["ScanState"]=type_sort();sig["scan"]=fn(state,fn(n,fn(inp,state))); // encoded result state
 sig["st"]=fn(sym("Mode"),fn(n,fn(bit,fn(n,state))));sig["result"]=fn(sym("Mode"),fn(n,fn(bit,fn(n,state))));
 sig["addDigit10"]=fn(n,fn(n,n));sig["addDigit16"]=fn(n,fn(n,n));sig["arith10"]=fn(n,fn(n,n));sig["arith16"]=fn(n,fn(n,n));sig["legal"]=fn(n,n);
 sig["step"]=fn(state,fn(d,state));sig["run"]=fn(state,fn(inp,state));sig["inputAt"]=fn(n,inp);sig["expectedAt"]=fn(n,state);sig["valueOf"]=fn(state,n);sig["posOf"]=fn(state,n);
 sig["dval"]=fn(d,n);sig["hexval"]=fn(d,n);sig["isDec"]=fn(d,bit);sig["isHex"]=fn(d,bit);
 sig["LegalClass"]=type_sort();for(auto x:{LegalClass::Zero,LegalClass::Surrogate,LegalClass::AboveMax,LegalClass::Win80,LegalClass::Win9f,LegalClass::Ordinary})sig[lc(x)]=sym("LegalClass");sig["LegalResult"]=type_sort();for(const char* x:{"replacementPoint","euroPoint","yDiaeresisPoint","ordinaryResult"})sig[x]=sym("LegalResult");sig["classifyLegal"]=fn(sym("LegalClass"),sym("LegalResult"));
 Ptr yes=sym("true"),no=sym("false");
 rs.push_back(rule(apps(sym("addDigit10"),p(0),p(1)),apps(sym("arith10"),p(0),p(1))));rs.push_back(rule(apps(sym("addDigit16"),p(0),p(1)),apps(sym("arith16"),p(0),p(1))));
 for(auto [a,digitv]:std::vector<std::pair<unsigned,unsigned>>{{0,0},{0,1},{1,2},{12,8},{0,9},{0,6},{6,5}})rs.push_back(rule(apps(sym("arith10"),nat(a),nat(digitv)),nat(a*10+digitv)));
 for(auto [a,digitv]:std::vector<std::pair<unsigned,unsigned>>{{0,4},{0,8},{8,0},{0,10}})rs.push_back(rule(apps(sym("arith16"),nat(a),nat(digitv)),nat(a*16+digitv)));

 for(auto [name,v]:ds){Ptr x=sym(name);rs.push_back(rule(app(sym("dval"),x),nat(v)));rs.push_back(rule(app(sym("hexval"),x),nat(v)));rs.push_back(rule(app(sym("isDec"),x),v<10?yes:no));rs.push_back(rule(app(sym("isHex"),x),yes));}
 for(const char* x:{"hash","lowerX","upperX","semi","other"}){rs.push_back(rule(app(sym("isDec"),sym(x)),no));rs.push_back(rule(app(sym("isHex"),sym(x)),no));}
 for(auto x:{LegalClass::Zero,LegalClass::Surrogate,LegalClass::AboveMax,LegalClass::Win80,LegalClass::Win9f,LegalClass::Ordinary})rs.push_back(rule(app(sym("classifyLegal"),sym(lc(x))),sym(mapped(x))));
 // State is result(mode, value, consumed-semicolon, cursor). Unconsumed terminators
 // return the cursor before that character; EOF becomes Await (streaming boundary).
 rs.push_back(rule(apps(sym("step"),apps(sym("result"),sym("initial"),p(0),p(1),p(2)),sym("hash")),apps(sym("result"),sym("number"),p(0),p(1),app(sym("s"),p(2)))));
 rs.push_back(rule(apps(sym("step"),apps(sym("result"),sym("number"),p(0),p(1),p(2)),sym("lowerX")),apps(sym("result"),sym("hexPrefix"),p(0),p(1),app(sym("s"),p(2)))));rs.push_back(rule(apps(sym("step"),apps(sym("result"),sym("number"),p(0),p(1),p(2)),sym("upperX")),apps(sym("result"),sym("hexPrefix"),p(0),p(1),app(sym("s"),p(2)))));
 for(auto [name,v]:ds)if(v<10)rs.push_back(rule(apps(sym("step"),apps(sym("result"),sym("number"),p(0),p(1),p(2)),sym(name)),apps(sym("result"),sym("decimal"),apps(sym("addDigit10"),p(0),app(sym("dval"),sym(name))),no,app(sym("s"),p(2)))));
 for(const char* m:{"decimal","hexadecimal","hexPrefix"}){for(auto [name,v]:ds){Ptr statex=sym(m);if(std::string_view(m)=="hexPrefix")statex=sym("hexPrefix");rs.push_back(rule(apps(sym("step"),apps(sym("result"),statex,p(0),p(1),p(2)),sym(name)),apps(sym("result"),std::string_view(m)=="decimal"?sym("decimal"):sym("hexadecimal"),apps(sym(std::string_view(m)=="decimal"?"addDigit10":"addDigit16"),p(0),app(sym(std::string_view(m)=="decimal"?"dval":"hexval"),sym(name))),no,app(sym("s"),p(2)))));}}
 rs.push_back(rule(apps(sym("step"),apps(sym("result"),sym("decimal"),p(0),p(1),p(2)),sym("semi")),apps(sym("result"),sym("done"),p(0),yes,app(sym("s"),p(2)))));rs.push_back(rule(apps(sym("step"),apps(sym("result"),sym("hexadecimal"),p(0),p(1),p(2)),sym("semi")),apps(sym("result"),sym("done"),p(0),yes,app(sym("s"),p(2)))));
 for(const char* m:{"decimal","hexadecimal"})rs.push_back(rule(apps(sym("step"),apps(sym("result"),sym(m),p(0),p(1),p(2)),p(3)),apps(sym("result"),sym("done"),p(0),no,p(2))));
 for(const char* bad:{"other","semi","hash","lowerX","upperX"}){rs.push_back(rule(apps(sym("step"),apps(sym("result"),sym("number"),p(0),p(1),p(2)),sym(bad)),apps(sym("result"),sym("done"),sym("z"),no,sym("z"))));rs.push_back(rule(apps(sym("step"),apps(sym("result"),sym("hexPrefix"),p(0),p(1),p(2)),sym(bad)),apps(sym("result"),sym("done"),sym("z"),no,sym("z"))));}
 rs.push_back(rule(apps(sym("valueOf"),apps(sym("result"),p(0),p(1),p(2),p(3))),p(1)));rs.push_back(rule(apps(sym("posOf"),apps(sym("result"),p(0),p(1),p(2),p(3))),p(3)));
 rs.push_back(rule(apps(sym("run"),apps(sym("result"),sym("done"),p(0),p(1),p(2)),sym("end")),apps(sym("result"),sym("done"),p(0),p(1),p(2))));
 rs.push_back(rule(apps(sym("run"),p(0),sym("end")),apps(sym("result"),sym("await"),sym("z"),no,sym("z"))));
 rs.push_back(rule(apps(sym("run"),p(0),apps(sym("cons"),p(1),p(2))),apps(sym("run"),apps(sym("step"),p(0),p(1)),p(2))));
 const std::vector<std::string_view> cases={"#12;","#xA;","#12z","#9","#","#x","#xZ","#xA!"};
 for(size_t i=0;i<cases.size();++i){Result c=cpp_scan(cases[i]);rs.push_back(rule(app(sym("inputAt"),nat(i)),input_term(cases[i])));rs.push_back(rule(app(sym("expectedAt"),nat(i)),output(c)));Ptr start=apps(sym("result"),sym("initial"),sym("z"),no,sym("z"));Ptr term=apps(sym("run"),start,app(sym("inputAt"),nat(i)));auto ty=infer({},term,sig,rs);assert(ty&&conv(*ty,state,rs));assert(conv(nf(term,rs),output(c),rs));}
 for(auto x:{LegalClass::Zero,LegalClass::Surrogate,LegalClass::AboveMax,LegalClass::Win80,LegalClass::Win9f,LegalClass::Ordinary}){Ptr term=app(sym("classifyLegal"),sym(lc(x)));assert(conv(nf(term,rs),sym(mapped(x)),rs));}
 assert(legal(0)==0xfffd&&legal(0xd800)==0xfffd&&legal(0x110000)==0xfffd&&legal(0x80)==0x20ac&&legal(0x9f)==0x178);
 std::println("Blink numeric HTML entity scanner: {} finite decimal/hex traces and 6 legal-codepoint classes agree with typed λΠ",cases.size());
}
