#include "../src/lambda_pi.hpp"
#include <cassert>
#include <optional>
#include <string>
#include <vector>

using namespace lp;

// Composes a bounded ConsumeHTMLEntity transcription with the attribute-value
// handoff at html_tokenizer.cc:1049-1078. Values are abstract Unicode scalars.
enum class Additional { DoubleQuote, SingleQuote, Greater };
enum class Status { Success, Failure, NeedMore };
struct Ref { Status status; std::vector<unsigned> output; };
struct TokenResult { const char* state; const char* action; bool has_entity; std::vector<unsigned> value; };
struct Entity { const char* name; unsigned first, second; };
static const std::vector<Entity> kEntities={
  // Values are from the pinned html_entity_names.csv; the decoder fixture is
  // intentionally bounded to the names exercised below.
  {"amp;",0x26,0},{"amp",0x26,0},{"not;",0xAC,0},{"not",0xAC,0},
  {"notin;",0x2209,0},{"NotEqualTilde;",0x2242,0x338}
};
static bool alnum(char c){return(c>='0'&&c<='9')||(c>='A'&&c<='Z')||(c>='a'&&c<='z');}
static char additional_allowed_character(Additional a){switch(a){case Additional::DoubleQuote:return '"';case Additional::SingleQuote:return '\'';case Additional::Greater:return '>';}return '\0';}
static Ref named_ref(const std::string& in,Additional add,bool eof){
  const Entity* best=nullptr;
  for(const auto&e:kEntities)if(in.size()>=std::string(e.name).size()&&in.compare(0,std::string(e.name).size(),e.name)==0&&(!best||std::string(e.name).size()>=std::string(best->name).size()))best=&e;
  if(eof&&!in.empty()&&in.back()!=';'&&std::any_of(kEntities.begin(),kEntities.end(),[&](const Entity&e){return std::string(e.name).rfind(in,0)==0;}))return{Status::NeedMore,{}};
  if(!best)return{Status::Failure,{}};
  const std::string name=best->name;const char next=in.size()>name.size()?in[name.size()]:'\0';
  if(name.back()!=';'&&additional_allowed_character(add)&&(alnum(next)||next=='='))return{Status::Failure,{}};
  std::vector<unsigned> out={best->first};if(best->second)out.push_back(best->second);return{Status::Success,out};
}
static Ref numeric_ref(const std::string& in,bool eof){
  if(in.empty())return{eof?Status::NeedMore:Status::Failure,{}};
  if(in[0]!='#')return{Status::Failure,{}};
  size_t i=1;unsigned base=10;if(i<in.size()&&(in[i]=='x'||in[i]=='X')){base=16;++i;}
  const size_t digits=i;unsigned value=0;
  auto digit=[&](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
  while(i<in.size()){int d=digit(in[i]);if(d<0||static_cast<unsigned>(d)>=base)break;value=value*base+static_cast<unsigned>(d);++i;}
  if(i==digits)return{eof?Status::NeedMore:Status::Failure,{}};
  if(i==in.size()&&eof)return{Status::NeedMore,{}};
  return{Status::Success,{value}}; // A semicolon or other terminator is left to the tokenizer.
}
static const char* restored(Additional a){switch(a){case Additional::DoubleQuote:return"doubleQuotedValue";case Additional::SingleQuote:return"singleQuotedValue";case Additional::Greater:return"unquotedValue";}return"unquotedValue";}
static TokenResult cpp_apply(Additional a,std::vector<unsigned> value,bool has_entity,const Ref& r){
  if(r.status==Status::NeedMore)return{"characterReference","suspended",has_entity,value};
  if(r.status==Status::Failure){value.push_back('&');return{restored(a),"fallbackAmpersand",has_entity,value};}
  value.insert(value.end(),r.output.begin(),r.output.end());return{restored(a),"appendedEntity",true,value};
}

static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned x){Ptr t=sym("z");while(x--)t=app(sym("s"),t);return t;}
static Ptr cp(unsigned x){return sym("cp"+std::to_string(x));}
static Ptr val(const std::vector<unsigned>& xs){Ptr t=sym("nilValue");for(auto i=xs.rbegin();i!=xs.rend();++i)t=apps(sym("consValue"),cp(*i),t);return t;}
static Ptr output_term(const std::vector<unsigned>& xs){Ptr t=sym("nilOutput");for(auto i=xs.rbegin();i!=xs.rend();++i)t=apps(sym("consOutput"),cp(*i),t);return t;}
static Ptr ref_term(const Ref&r){if(r.status==Status::NeedMore)return sym("needMore");if(r.status==Status::Failure)return sym("refFailure");Ptr out=sym("nilOutput");for(auto i=r.output.rbegin();i!=r.output.rend();++i)out=apps(sym("consOutput"),cp(*i),out);return app(sym("refSuccess"),out);}
static Ptr result_term(const TokenResult&r){return apps(sym("tokenResult"),sym(r.state),sym(r.action),sym(r.has_entity?"true":"false"),val(r.value));}
static Ptr add_term(Additional a){switch(a){case Additional::DoubleQuote:return sym("addDouble");case Additional::SingleQuote:return sym("addSingle");case Additional::Greater:return sym("addGreater");}return sym("addGreater");}
static const char* decoder_case(const std::string& input,bool numeric){
  if(numeric){if(input=="#65;")return"numeric65Semi";if(input=="#65x")return"numeric65Term";if(input=="#x;")return"numericBadHex";}
  else{if(input=="amp;")return"namedAmpSemi";if(input=="ampx")return"namedAmpX";if(input=="not")return"namedNotEof";if(input=="amp!")return"namedAmpBang";if(input=="amp=")return"namedAmpEquals";if(input=="NotEqualTilde;")return"namedTwoCodepoints";if(input=="notin;")return"namedNotin";}
  return"unknownDecoderCase";
}

int main(){
  struct Case{Additional add;std::vector<unsigned> prefix;bool prior_entity;bool numeric;std::string input;bool eof;};
  const std::vector<Case> cases={
    {Additional::DoubleQuote,{0x61},false,false,"amp;",true},
    {Additional::DoubleQuote,{0x62},false,false,"ampx",true},
    {Additional::SingleQuote,{0x63},false,false,"not",true},
    {Additional::Greater,{0x64},true,false,"amp!",false},
    {Additional::SingleQuote,{0x65},false,false,"amp=",true},
    {Additional::DoubleQuote,{0x66},false,true,"#65;",true},
    {Additional::SingleQuote,{0x67},false,true,"#65x",false},
    {Additional::Greater,{0x68},false,true,"#x;",false},
    {Additional::DoubleQuote,{0x69},false,false,"NotEqualTilde;",true},
    {Additional::Greater,{0x6A},false,false,"notin;",true},
  };
  Signature sig;std::vector<Rule>rs;auto p=[](size_t i){return pvar(i);};
  Ptr B=sym("Bool"),CP=sym("Codepoint"),V=sym("Value"),O=sym("Output"),RefT=sym("RefOutcome"),Tok=sym("TokenizerResult");
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;sig["Codepoint"]=type_sort();sig["Value"]=type_sort();sig["nilValue"]=V;sig["consValue"]=fn(CP,fn(V,V));sig["snocAmp"]=fn(V,V);
  sig["Output"]=type_sort();sig["nilOutput"]=O;sig["consOutput"]=fn(CP,fn(O,O));sig["appendOutput"]=fn(O,fn(V,V));sig["concat"]=fn(V,fn(O,V));
  sig["RefOutcome"]=type_sort();sig["needMore"]=RefT;sig["refFailure"]=RefT;sig["refSuccess"]=fn(O,RefT);
  sig["DecoderCase"]=type_sort();for(const char*x:{"namedAmpSemi","namedAmpX","namedNotEof","namedAmpBang","namedAmpEquals","numeric65Semi","numeric65Term","numericBadHex","namedTwoCodepoints","namedNotin"})sig[x]=sym("DecoderCase");
  sig["Additional"]=type_sort();for(const char*x:{"addDouble","addSingle","addGreater"})sig[x]=sym("Additional");
  sig["TokenizerState"]=type_sort();for(const char*x:{"doubleQuotedValue","singleQuotedValue","unquotedValue","characterReference"})sig[x]=sym("TokenizerState");
  sig["Disposition"]=type_sort();for(const char*x:{"suspended","fallbackAmpersand","appendedEntity"})sig[x]=sym("Disposition");
  sig["TokenizerResult"]=type_sort();sig["tokenResult"]=fn(sym("TokenizerState"),fn(sym("Disposition"),fn(B,fn(V,Tok))));
  sig["appendRef"]=fn(O,fn(V,V));sig["returnState"]=fn(sym("Additional"),sym("TokenizerState"));sig["applyRef"]=fn(sym("Additional"),fn(V,fn(B,fn(RefT,Tok))));
  sig["decodeRef"]=fn(sym("DecoderCase"),RefT);sig["addAt"]=fn(sym("Nat"),sym("Additional"));sig["valueAt"]=fn(sym("Nat"),V);sig["entityAt"]=fn(sym("Nat"),B);sig["decoderCaseAt"]=fn(sym("Nat"),sym("DecoderCase"));sig["observe"]=fn(sym("Nat"),Tok);sig["Nat"]=type_sort();sig["z"]=sym("Nat");sig["s"]=fn(sym("Nat"),sym("Nat"));
  std::vector<unsigned> cps={0x26,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,65,0xAC,0x2209,0x2242,0x338};for(unsigned x:cps)sig["cp"+std::to_string(x)]=CP;
  rs.push_back(R(apps(sym("concat"),p(0),sym("nilOutput")),p(0)));
  rs.push_back(R(apps(sym("concat"),sym("nilValue"),apps(sym("consOutput"),p(0),p(1))),apps(sym("consValue"),p(0),apps(sym("concat"),sym("nilValue"),p(1)))));
  rs.push_back(R(apps(sym("concat"),apps(sym("consValue"),p(0),p(1)),p(2)),apps(sym("consValue"),p(0),apps(sym("concat"),p(1),p(2)))));
  rs.push_back(R(app(sym("snocAmp"),p(0)),apps(sym("concat"),p(0),apps(sym("consOutput"),cp(0x26),sym("nilOutput")))));
  rs.push_back(R(app(sym("returnState"),sym("addDouble")),sym("doubleQuotedValue")));rs.push_back(R(app(sym("returnState"),sym("addSingle")),sym("singleQuotedValue")));rs.push_back(R(app(sym("returnState"),sym("addGreater")),sym("unquotedValue")));
  for(const char* add:{"addDouble","addSingle","addGreater"}){
    rs.push_back(R(apps(sym("applyRef"),sym(add),p(0),p(1),sym("needMore")),apps(sym("tokenResult"),sym("characterReference"),sym("suspended"),p(1),p(0))));
    rs.push_back(R(apps(sym("applyRef"),sym(add),p(0),p(1),sym("refFailure")),apps(sym("tokenResult"),app(sym("returnState"),sym(add)),sym("fallbackAmpersand"),p(1),app(sym("snocAmp"),p(0)))));
    rs.push_back(R(apps(sym("applyRef"),sym(add),p(0),p(1),app(sym("refSuccess"),p(2))),apps(sym("tokenResult"),app(sym("returnState"),sym(add)),sym("appendedEntity"),sym("true"),apps(sym("concat"),p(0),p(2)))));
  }
  rs.push_back(R(apps(sym("decodeRef"),sym("namedAmpSemi")),app(sym("refSuccess"),output_term({0x26}))));
  rs.push_back(R(apps(sym("decodeRef"),sym("namedAmpX")),sym("refFailure")));rs.push_back(R(apps(sym("decodeRef"),sym("namedNotEof")),sym("needMore")));
  rs.push_back(R(apps(sym("decodeRef"),sym("namedAmpBang")),app(sym("refSuccess"),output_term({0x26}))));rs.push_back(R(apps(sym("decodeRef"),sym("namedAmpEquals")),sym("refFailure")));
  rs.push_back(R(apps(sym("decodeRef"),sym("numeric65Semi")),app(sym("refSuccess"),output_term({65}))));rs.push_back(R(apps(sym("decodeRef"),sym("numeric65Term")),app(sym("refSuccess"),output_term({65}))));rs.push_back(R(apps(sym("decodeRef"),sym("numericBadHex")),sym("refFailure")));
  rs.push_back(R(apps(sym("decodeRef"),sym("namedTwoCodepoints")),app(sym("refSuccess"),output_term({0x2242,0x338}))));
  rs.push_back(R(apps(sym("decodeRef"),sym("namedNotin")),app(sym("refSuccess"),output_term({0x2209}))));
  rs.push_back(R(apps(sym("observe"),p(0)),apps(sym("applyRef"),app(sym("addAt"),p(0)),app(sym("valueAt"),p(0)),app(sym("entityAt"),p(0)),apps(sym("decodeRef"),app(sym("decoderCaseAt"),p(0))))));
  for(size_t i=0;i<cases.size();++i){const auto&c=cases[i];Ref ref=c.numeric?numeric_ref(c.input,c.eof):named_ref(c.input,c.add,c.eof);TokenResult out=cpp_apply(c.add,c.prefix,c.prior_entity,ref);Ptr decoder=sym(decoder_case(c.input,c.numeric));rs.push_back(R(app(sym("addAt"),nat(i)),add_term(c.add)));rs.push_back(R(app(sym("valueAt"),nat(i)),val(c.prefix)));rs.push_back(R(app(sym("entityAt"),nat(i)),sym(c.prior_entity?"true":"false")));rs.push_back(R(app(sym("decoderCaseAt"),nat(i)),decoder));if(!conv(nf(app(sym("decodeRef"),decoder),rs),ref_term(ref),rs)){std::println("decoder mismatch case {}",i);return 2;}if(!conv(nf(app(sym("observe"),nat(i)),rs),result_term(out),rs)){std::println("tokenizer composition mismatch case {}",i);return 2;}}
  auto ty=infer({},app(sym("observe"),nat(0)),sig,rs);assert(ty&&conv(*ty,Tok,rs));
  std::println("Attribute character-reference composition: C++ and typed λΠ decoder outcomes plus tokenizer results agree on {} finite cases",cases.size());
  std::println("Scope: semicolonless names use the ASCII alphanumeric/'=' guard only with a nonzero additional-allowed character; numeric cases bypass that named-reference guard.");
  std::println("Tokenizer slice: success append/entity flag, '&' fallback, not-enough suspension, and restoration from double/single/'>' context; entity parsing remains a finite C++ transcription plus typed case table.");
}
