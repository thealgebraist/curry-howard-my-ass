#include "../src/lambda_pi.hpp"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace lp;
enum class Mode { Initial, Number, HexPrefix, Decimal, Hex, Done, Await };
struct Scan { Mode mode=Mode::Initial; int64_t value=0; bool accepted=false, semicolon=false; unsigned cursor=0; };
struct Arithmetic { bool hex; int64_t before; unsigned digit; int64_t after; };
static constexpr uint32_t kWindowsLatin1[32]={0x20ac,0x81,0x201a,0x192,0x201e,0x2026,0x2020,0x2021,0x2c6,0x2030,0x160,0x2039,0x152,0x8d,0x17d,0x8f,0x90,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x2dc,0x2122,0x161,0x203a,0x153,0x9d,0x17e,0x178};
// Independent transcription for the λΠ mapping rules; compared against the
// C++ helper above on all source table inputs.
static constexpr uint32_t kTypedWindowsLatin1[32]={0x20ac,0x81,0x201a,0x192,0x201e,0x2026,0x2020,0x2021,0x2c6,0x2030,0x160,0x2039,0x152,0x8d,0x17d,0x8f,0x90,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x2dc,0x2122,0x161,0x203a,0x153,0x9d,0x17e,0x178};
static int dec(char c){return c>='0'&&c<='9'?c-'0':-1;}
static int hexv(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
static uint32_t append_legal(int64_t c){
 if(c<=0||c>0x10ffff||(c>=0xd800&&c<=0xdfff))return 0xfffd;
 if(c>=0x80&&c<=0x9f)return kWindowsLatin1[c-0x80];return static_cast<uint32_t>(c);
}
static Scan cpp_step(Scan s,char c){
 auto add=[&](bool hex,unsigned d){if(s.value<0)return;int64_t next=s.value*(hex?16:10)+d;s.value=next>0x10ffff?-1:next;};
 switch(s.mode){
 case Mode::Initial:if(c=='#'){s.mode=Mode::Number;++s.cursor;}else{s.mode=Mode::Done;}return s;
 case Mode::Number:if(c=='x'||c=='X'){s.mode=Mode::HexPrefix;++s.cursor;}else if(dec(c)>=0){s.mode=Mode::Decimal;add(false,dec(c));++s.cursor;}else{s.mode=Mode::Done;s.value=0;s.cursor=0;}return s;
 case Mode::HexPrefix:if(hexv(c)>=0){s.mode=Mode::Hex;add(true,hexv(c));++s.cursor;}else{s.mode=Mode::Done;s.value=0;s.cursor=0;}return s;
 case Mode::Decimal:if(dec(c)>=0){add(false,dec(c));++s.cursor;}else{s.mode=Mode::Done;s.value=append_legal(s.value);s.accepted=true;if(c==';'){s.semicolon=true;++s.cursor;}}return s;
 case Mode::Hex:if(hexv(c)>=0){add(true,hexv(c));++s.cursor;}else{s.mode=Mode::Done;s.value=append_legal(s.value);s.accepted=true;if(c==';'){s.semicolon=true;++s.cursor;}}return s;
 case Mode::Done:case Mode::Await:return s;
 }return s;
}
static Scan cpp_finish(Scan s){if(s.mode!=Mode::Done){s.mode=Mode::Await;s.value=0;s.accepted=false;s.semicolon=false;s.cursor=0;}return s;}

// Explicit finite transition table, independent of the C++ scanner's execution.
static const std::vector<Arithmetic> arithmetic=[] {
 std::vector<Arithmetic> result={
 {false,0,0,0},{false,0,1,1},{false,0,9,9},{false,1,2,12},{false,12,8,128},{false,0,6,6},{false,6,5,65},
 {false,0,5,5},{false,5,5,55},{false,55,2,552},{false,552,9,5529},{false,5529,6,55296},
 {false,1,1,11},{false,11,1,111},{false,111,4,1114},{false,1114,1,11141},{false,11141,1,111411},
 {false,111411,1,1114111},{false,111411,2,-1},{false,1114111,0,-1},{false,-1,7,-1},{false,-1,5,-1},
 {true,0,8,8},{true,0,9,9},{true,0,4,4},{true,4,1,65},
 {true,0,1,1},{true,1,0,16},{true,16,15,271},{true,271,15,4351},{true,4351,15,69631},
 {true,69631,15,1114111},{true,1,1,17},{true,17,0,272},{true,272,0,4352},{true,4352,0,69632},{true,69632,0,-1},{true,-1,15,-1}
 };
 // Explicit finite two-digit hexadecimal transitions for every U+0080-U+009F
 // input, independent of the C++ scanner's traces.
 for(unsigned d=0;d<16;++d){result.push_back({true,8,d,128+d});result.push_back({true,9,d,144+d});}
 result.push_back({true,0,7,7});result.push_back({true,7,15,127}); // U+007F pass-through
 result.push_back({true,0,10,10});result.push_back({true,10,0,160}); // U+00A0 pass-through
 return result;
}();
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr r=sym("z");while(n--)r=app(sym("s"),r);return r;}
static std::string value_name(int64_t v){return v<0?"invalid":"v"+std::to_string(v);}
static Ptr value(int64_t v){return sym(value_name(v));}
static const char* mode_name(Mode m){static constexpr const char* names[]={"initial","number","hexPrefix","decimal","hexadecimal","done","await"};return names[static_cast<unsigned>(m)];}
static std::string digit_name(char c){if(dec(c)>=0)return "d"+std::string(1,c);if(hexv(c)>=10)return "h"+std::string(1,char(c>='a'?c-'a'+'A':c));if(c=='#')return"hash";if(c=='x')return"lowerX";if(c=='X')return"upperX";if(c==';')return"semi";return"other";}
static Ptr cp_term(uint32_t cp){if(cp==0xfffd)return sym("replacement");if(cp==0x20ac)return sym("euro");if(cp==0x178)return sym("yDiaeresis");return value(cp);}
static Ptr scan_term(Scan s){Ptr v=s.mode==Mode::Done&&s.accepted?cp_term(static_cast<uint32_t>(s.value)):value(s.mode==Mode::Done?0:s.value);return apps(sym("scanState"),sym(mode_name(s.mode)),v,sym(s.accepted?"yes":"no"),sym(s.semicolon?"yes":"no"),nat(s.cursor));}
static Ptr digit_term(char c){return sym(digit_name(c));}
int main(){
 std::vector<std::string> cases={"#0;","#128;","#65;","#55296;","#1114112;","#11141120;","#x110000;","#x110000F;","#1114111;","#x10FFFF;","#12z","#xZ","#","#x","#9","#x7F;","#xA0;"};
 for(unsigned cp=0x80;cp<=0x9f;++cp){char s[16];std::snprintf(s,sizeof(s),"#x%02X;",cp);cases.emplace_back(s);}
 Signature sig;std::vector<Rule> rs;auto p=[](int i){return pvar(i);};
 Ptr natty=sym("Nat"),dig=sym("Digit"),state=sym("State"),bit=sym("Bool"),vty=sym("Value");
 sig["Nat"]=type_sort();sig["z"]=natty;sig["s"]=fn(natty,natty);sig["Bool"]=type_sort();sig["yes"]=bit;sig["no"]=bit;sig["Mode"]=type_sort();
 for(const char* m:{"initial","number","hexPrefix","decimal","hexadecimal","done","await"})sig[m]=sym("Mode");
 sig["Digit"]=type_sort();for(const char* d:{"hash","lowerX","upperX","semi","other"})sig[d]=dig;for(unsigned i=0;i<10;++i)sig["d"+std::to_string(i)]=dig;for(const char* d:{"hA","hB","hC","hD","hE","hF"})sig[d]=dig;
 sig["Value"]=type_sort();std::set<int64_t> vals={0,1,4,5,6,7,8,9,10,11,12,16,17,55,65,111,127,128,159,160,256,271,272,552,1114,4096,4351,4352,5529,43520,65536,69631,69632,111411,1114110,1114111,1114112,-1};
 for(unsigned i=0;i<32;++i){vals.insert(0x80+i);vals.insert(kWindowsLatin1[i]);vals.insert(kTypedWindowsLatin1[i]);}
 for(const auto& a:arithmetic){vals.insert(a.before);vals.insert(a.after);}for(auto x:vals)sig[value_name(x)]=vty;for(const char* c:{"replacement","euro","yDiaeresis"})sig[c]=vty;
 sig["scanState"]=fn(sym("Mode"),fn(vty,fn(bit,fn(bit,fn(natty,state)))));sig["acc10"]=fn(vty,fn(dig,vty));sig["acc16"]=fn(vty,fn(dig,vty));sig["legal"]=fn(vty,vty);sig["step"]=fn(state,fn(dig,state));sig["finish"]=fn(state,state);
 auto yes=sym("yes"),no=sym("no");
 for(const auto& a:arithmetic){Ptr d=sym(a.hex?(a.digit<10?"d"+std::to_string(a.digit):"h"+std::string(1,char('A'+a.digit-10))):"d"+std::to_string(a.digit));rs.push_back(rule(apps(sym(a.hex?"acc16":"acc10"),value(a.before),d),value(a.after)));}
 // Once invalid, all later digits preserve the sentinel.
 for(unsigned d=0;d<10;++d){Ptr x=sym(("d"+std::to_string(d)).c_str());rs.push_back(rule(apps(sym("acc10"),sym("invalid"),x),sym("invalid")));}
 for(unsigned d=0;d<16;++d){Ptr x=sym((d<10?"d"+std::to_string(d):"h"+std::string(1,char('A'+d-10))).c_str());rs.push_back(rule(apps(sym("acc16"),sym("invalid"),x),sym("invalid")));}
 rs.push_back(rule(app(sym("legal"),sym("invalid")),sym("replacement")));rs.push_back(rule(app(sym("legal"),sym("v0")),sym("replacement")));rs.push_back(rule(app(sym("legal"),sym("v55296")),sym("replacement")));
 for(unsigned i=0;i<32;++i)rs.push_back(rule(app(sym("legal"),value(0x80+i)),cp_term(kTypedWindowsLatin1[i])));
 rs.push_back(rule(app(sym("legal"),p(0)),p(0)));
 rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("initial"),p(0),p(1),p(2),p(3)),sym("hash")),apps(sym("scanState"),sym("number"),p(0),no,no,app(sym("s"),p(3)))));
 rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("number"),p(0),p(1),p(2),p(3)),sym("lowerX")),apps(sym("scanState"),sym("hexPrefix"),p(0),no,no,app(sym("s"),p(3)))));rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("number"),p(0),p(1),p(2),p(3)),sym("upperX")),apps(sym("scanState"),sym("hexPrefix"),p(0),no,no,app(sym("s"),p(3)))));
 for(unsigned d=0;d<10;++d){Ptr x=sym(("d"+std::to_string(d)).c_str());rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("number"),p(0),p(1),p(2),p(3)),x),apps(sym("scanState"),sym("decimal"),apps(sym("acc10"),sym("v0"),x),no,no,app(sym("s"),p(3)))));}
 for(unsigned d=0;d<16;++d){Ptr x=sym((d<10?"d"+std::to_string(d):"h"+std::string(1,char('A'+d-10))).c_str());rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("hexPrefix"),p(0),p(1),p(2),p(3)),x),apps(sym("scanState"),sym("hexadecimal"),apps(sym("acc16"),sym("v0"),x),no,no,app(sym("s"),p(3)))));rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("decimal"),p(0),p(1),p(2),p(3)),x),apps(sym("scanState"),sym("decimal"),apps(sym("acc10"),p(0),x),no,no,app(sym("s"),p(3)))));rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("hexadecimal"),p(0),p(1),p(2),p(3)),x),apps(sym("scanState"),sym("hexadecimal"),apps(sym("acc16"),p(0),x),no,no,app(sym("s"),p(3)))));}
 rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("decimal"),p(0),p(1),p(2),p(3)),sym("semi")),apps(sym("scanState"),sym("done"),app(sym("legal"),p(0)),yes,yes,app(sym("s"),p(3)))));rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("hexadecimal"),p(0),p(1),p(2),p(3)),sym("semi")),apps(sym("scanState"),sym("done"),app(sym("legal"),p(0)),yes,yes,app(sym("s"),p(3)))));
 for(const char* m:{"decimal","hexadecimal"})rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym(m),p(0),p(1),p(2),p(3)),p(4)),apps(sym("scanState"),sym("done"),app(sym("legal"),p(0)),yes,no,p(3))));
 for(const char* bad:{"other","semi","hash","lowerX","upperX","hA","hB","hC","hD","hE","hF"}){rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("number"),p(0),p(1),p(2),p(3)),sym(bad)),apps(sym("scanState"),sym("done"),sym("v0"),no,no,sym("z"))));rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("hexPrefix"),p(0),p(1),p(2),p(3)),sym(bad)),apps(sym("scanState"),sym("done"),sym("v0"),no,no,sym("z"))));}
 rs.push_back(rule(apps(sym("step"),apps(sym("scanState"),sym("done"),p(0),p(1),p(2),p(3)),p(4)),apps(sym("scanState"),sym("done"),p(0),p(1),p(2),p(3))));
 rs.push_back(rule(app(sym("finish"),apps(sym("scanState"),sym("done"),p(0),p(1),p(2),p(3))),apps(sym("scanState"),sym("done"),p(0),p(1),p(2),p(3))));rs.push_back(rule(app(sym("finish"),p(0)),apps(sym("scanState"),sym("await"),sym("v0"),no,no,sym("z"))));
 Ptr start=apps(sym("scanState"),sym("initial"),sym("v0"),no,no,sym("z"));
 auto check_typed=[&](Ptr term){auto ty=infer({},term,sig,rs);assert(ty&&conv(*ty,state,rs));};check_typed(apps(sym("step"),start,sym("hash")));
 for(auto input:cases){Scan cpp;Ptr typed=start;for(char c:input){cpp=cpp_step(cpp,c);typed=nf(apps(sym("step"),typed,digit_term(c)),rs);if(!conv(typed,scan_term(cpp),rs)){std::println("mismatch {} char {} cpp {}",input,c,to_string(scan_term(cpp)));std::println("got {}",to_string(typed));return 1;}if(cpp.mode==Mode::Done)break;}cpp=cpp_finish(cpp);typed=nf(app(sym("finish"),typed),rs);assert(conv(typed,scan_term(cpp),rs));}
 std::println("Blink numeric entity model: {} independent C++/typed λΠ step traces agree across exact U+10FFFF latch and final mapping",cases.size());
}
