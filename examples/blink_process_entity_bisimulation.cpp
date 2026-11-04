#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <string_view>
#include <vector>

using namespace lp;
enum class Ret { Data, Rcdata };
enum class Status { Resume, Suspend };
enum class DecKind { One, Two, Failure, Pending };
struct Decoded { DecKind kind; const char* a="none"; const char* b="none"; };
struct Token { std::vector<const char*> chars; bool has_entity=false; };
struct Outcome { Status status; Ret ret; Token token; };
struct Case { Ret ret; Decoded decoded; Token before; };
static Decoded decode_numeric(std::string_view s){if(s=="#10;"||s=="#xA;")return{DecKind::One,"cp10"};if(s=="#")return{DecKind::Pending};if(s=="#xZ")return{DecKind::Failure};return{DecKind::Failure};}
static Decoded decode_named(std::string_view s){if(s=="amp;")return{DecKind::One,"cpAmp"};if(s=="NotEqualTilde;")return{DecKind::Two,"cp2242","cp338"};if(s=="NotEqu")return{DecKind::Pending};return{DecKind::Failure};}
static Outcome cpp_process(Ret ret,Decoded d,Token token){switch(d.kind){case DecKind::One:token.chars.push_back(d.a);token.has_entity=true;return{Status::Resume,ret,token};case DecKind::Two:token.chars.push_back(d.a);token.chars.push_back(d.b);token.has_entity=true;return{Status::Resume,ret,token};case DecKind::Failure:token.chars.push_back("ampersand");return{Status::Resume,ret,token};case DecKind::Pending:return{Status::Suspend,ret,token};}std::abort();}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned x){Ptr r=sym("z");while(x--)r=app(sym("s"),r);return r;}
static Ptr chars_term(const std::vector<const char*>& xs){Ptr r=sym("nilChars");for(auto i=xs.rbegin();i!=xs.rend();++i)r=apps(sym("consChar"),sym(*i),r);return r;}
static Ptr token_term(const Token& t){return apps(sym("token"),chars_term(t.chars),sym(t.has_entity?"yes":"no"));}
static Ptr decoded_term(Decoded d){switch(d.kind){case DecKind::One:return app(sym("one"),sym(d.a));case DecKind::Two:return apps(sym("two"),sym(d.a),sym(d.b));case DecKind::Failure:return sym("failure");case DecKind::Pending:return sym("pending");}return sym("failure");}
static Ptr outcome_term(Outcome o){return apps(sym("outcome"),sym(o.status==Status::Resume?"resume":"suspend"),sym(o.ret==Ret::Data?"data":"rcdata"),token_term(o.token));}
int main(){
 const std::array<Case,8> cs={
  Case{Ret::Data,decode_numeric("#10;"),{}},Case{Ret::Rcdata,decode_numeric("#xA;"),{{"prior"},false}},
  Case{Ret::Data,decode_numeric("#xZ"),{}},Case{Ret::Rcdata,decode_numeric("#"),{{"prior"},false}},
  Case{Ret::Data,decode_named("amp;"),{}},Case{Ret::Rcdata,decode_named("NotEqualTilde;"),{}},
  Case{Ret::Data,decode_named("xyz"),{{"prefix"},false}},Case{Ret::Rcdata,decode_named("NotEqu"),{{"prefix"},true}}};
 Signature sig;std::vector<Rule>rs;auto p=[](int i){return pvar(i);};
 Ptr B=sym("Bool"),C=sym("Chars"),ch=sym("Char"),T=sym("Token"),D=sym("Decoded"),Rstate=sym("ReturnState"),St=sym("Status"),O=sym("Outcome");
 sig["Bool"]=type_sort();sig["yes"]=B;sig["no"]=B;sig["Char"]=type_sort();
 for(const char* x:{"cp10","cpAmp","cp2242","cp338","ampersand","prior","prefix"})sig[x]=ch;
 sig["Chars"]=type_sort();sig["nilChars"]=C;sig["consChar"]=fn(ch,fn(C,C));sig["Token"]=type_sort();sig["token"]=fn(C,fn(B,T));
 sig["Decoded"]=type_sort();sig["failure"]=D;sig["pending"]=D;sig["one"]=fn(ch,D);sig["two"]=fn(ch,fn(ch,D));
 sig["ReturnState"]=type_sort();sig["data"]=Rstate;sig["rcdata"]=Rstate;sig["Status"]=type_sort();sig["resume"]=St;sig["suspend"]=St;
 sig["Outcome"]=type_sort();sig["outcome"]=fn(St,fn(Rstate,fn(T,O)));sig["append"]=fn(C,fn(ch,C));sig["process"]=fn(D,fn(Rstate,fn(T,O)));
 sig["decodeAt"]=fn(sym("Nat"),D);sig["tokenAt"]=fn(sym("Nat"),T);sig["returnAt"]=fn(sym("Nat"),Rstate);sig["Nat"]=type_sort();sig["z"]=sym("Nat");sig["s"]=fn(sym("Nat"),sym("Nat"));
 rs.push_back(R(apps(sym("append"),sym("nilChars"),p(0)),apps(sym("consChar"),p(0),sym("nilChars"))));rs.push_back(R(apps(sym("append"),apps(sym("consChar"),p(0),p(1)),p(2)),apps(sym("consChar"),p(0),apps(sym("append"),p(1),p(2)))));
 Ptr one_lhs=apps(sym("process"),app(sym("one"),p(0)),p(1),apps(sym("token"),p(2),p(3)));
 Ptr one_rhs=apps(sym("outcome"),sym("resume"),p(1),apps(sym("token"),apps(sym("append"),p(2),p(0)),sym("yes")));rs.push_back(R(one_lhs,one_rhs));
 Ptr two_lhs=apps(sym("process"),apps(sym("two"),p(0),p(1)),p(2),apps(sym("token"),p(3),p(4)));
 Ptr two_chars=apps(sym("append"),apps(sym("append"),p(3),p(0)),p(1));Ptr two_rhs=apps(sym("outcome"),sym("resume"),p(2),apps(sym("token"),two_chars,sym("yes")));rs.push_back(R(two_lhs,two_rhs));
 Ptr fail_lhs=apps(sym("process"),sym("failure"),p(0),apps(sym("token"),p(1),p(2)));
 Ptr fail_rhs=apps(sym("outcome"),sym("resume"),p(0),apps(sym("token"),apps(sym("append"),p(1),sym("ampersand")),p(2)));rs.push_back(R(fail_lhs,fail_rhs));
 rs.push_back(R(apps(sym("process"),sym("pending"),p(0),p(1)),apps(sym("outcome"),sym("suspend"),p(0),p(1))));
 for(size_t i=0;i<cs.size();++i){const auto& c=cs[i];Ret ret=c.ret;Decoded d=c.decoded;Token before=c.before;Ptr id=nat(static_cast<unsigned>(i));rs.push_back(R(app(sym("decodeAt"),id),decoded_term(d)));rs.push_back(R(app(sym("tokenAt"),id),token_term(before)));rs.push_back(R(app(sym("returnAt"),id),sym(ret==Ret::Data?"data":"rcdata")));Outcome expected=cpp_process(ret,d,before);Ptr term=apps(sym("process"),app(sym("decodeAt"),id),app(sym("returnAt"),id),app(sym("tokenAt"),id));auto ty=infer({},term,sig,rs);assert(ty&&conv(*ty,O,rs));assert(conv(nf(term,rs),outcome_term(expected),rs));}
 std::println("Blink ProcessEntity composition: {} numeric/named success, failure, and pending cases agree in typed λΠ",cs.size());
}
