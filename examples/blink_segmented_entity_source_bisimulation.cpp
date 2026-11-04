#include "../src/lambda_pi.hpp"
#include <cassert>
#include <deque>
#include <optional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace lp;
struct Segment { std::string text; size_t pos=0; };
struct Source { std::deque<Segment> chunks; unsigned consumed=0; };
enum class TokenContext { Data, Attribute };
enum class Phase { CharacterReference, Suspended, Data, AttributeValue };
struct Token { std::vector<std::string> output; bool has_entity=false; };
struct Parser { TokenContext context; Phase phase; Source source; Token token; };

static char current_char(const Source& s){assert(!s.chunks.empty()&&s.chunks.front().pos<s.chunks.front().text.size());return s.chunks.front().text[s.chunks.front().pos];}
static void advance_expecting(Source& s,char expected){assert(current_char(s)==expected);auto& seg=s.chunks.front();++seg.pos;++s.consumed;if(seg.pos==seg.text.size())s.chunks.pop_front();}
static void push(Source& s,char c){assert(s.consumed);if(!s.chunks.empty()){auto& seg=s.chunks.front();if(seg.pos&&seg.text[seg.pos-1]==c){--seg.pos;--s.consumed;return;}}s.chunks.push_front({std::string(1,c),0});--s.consumed;}
static void prepend_unconsume(Source& s,std::string text){assert(text.size()<=s.consumed);if(text.empty())return;s.chunks.push_front({std::move(text),0});s.consumed-=s.chunks.front().text.size();}
static void append(Source& s,std::string text){if(text.empty())return;if(s.chunks.empty())s.chunks.push_front({std::move(text),0});else s.chunks.push_back({std::move(text),0});}
static void suspend(Parser& p){assert(p.phase==Phase::CharacterReference);p.phase=Phase::Suspended;}
static void resume(Parser& p){assert(p.phase==Phase::Suspended);p.phase=Phase::CharacterReference;}
static void complete_named(Parser& p,const char* cp){assert(p.phase==Phase::CharacterReference);p.token.output.emplace_back(cp);p.token.has_entity=true;p.phase=p.context==TokenContext::Data?Phase::Data:Phase::AttributeValue;}

static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr t=sym("zero");while(n--)t=app(sym("succ"),t);return t;}
static Ptr chr(char c){return sym("ch"+std::to_string(static_cast<unsigned char>(c)));}
static Ptr text_term(std::string_view s){Ptr r=sym("nilChars");for(auto i=s.rbegin();i!=s.rend();++i)r=apps(sym("consChar"),chr(*i),r);return r;}
// Segment::before is a stack: its head is the character immediately before the cursor.
static Ptr reversed_prefix(std::string_view s,size_t n){Ptr r=sym("nilChars");for(size_t i=0;i<n;++i)r=apps(sym("consChar"),chr(s[i]),r);return r;}
static Ptr segment_term(const Segment& s){return apps(sym("segment"),reversed_prefix(s.text,s.pos),text_term(std::string_view(s.text).substr(s.pos)));}
static Ptr source_term(const Source& s){Ptr segs=sym("nilSegments");for(auto it=s.chunks.rbegin();it!=s.chunks.rend();++it)segs=apps(sym("consSegment"),segment_term(*it),segs);return apps(sym("source"),segs,nat(s.consumed));}
static const char* context_name(TokenContext c){return c==TokenContext::Data?"dataContext":"attributeContext";}
static const char* phase_name(Phase p){switch(p){case Phase::CharacterReference:return"characterReference";case Phase::Suspended:return"suspended";case Phase::Data:return"dataState";case Phase::AttributeValue:return"attributeValueState";}return"suspended";}
static Ptr token_term(const Token& t){Ptr xs=sym("nilOutput");for(auto i=t.output.rbegin();i!=t.output.rend();++i)xs=apps(sym("consOutput"),sym(i->c_str()),xs);return apps(sym("token"),xs,sym(t.has_entity?"yes":"no"));}
static Ptr parser_term(const Parser& p){return apps(sym("parser"),sym(context_name(p.context)),sym(phase_name(p.phase)),source_term(p.source),token_term(p.token));}
static Ptr char_list(std::string_view s){return text_term(s);}

static void check_source_op(Ptr& typed,Source& cpp,const char* op,char c,const std::string& data,const std::vector<Rule>& rules){
 if(std::string_view(op)=="advance"){const char got=current_char(cpp);assert(got==c);advance_expecting(cpp,c);typed=nf(apps(sym("advanceExpecting"),typed,chr(c)),rules);}
 else if(std::string_view(op)=="push"){push(cpp,c);typed=nf(apps(sym("push"),typed,chr(c)),rules);}
 else if(std::string_view(op)=="prepend"){prepend_unconsume(cpp,data);typed=nf(apps(sym("prependUnconsume"),typed,char_list(data)),rules);}
 else {append(cpp,data);typed=nf(apps(sym("appendChunk"),typed,char_list(data)),rules);}
 if(!conv(typed,source_term(cpp),rules)){std::cerr<<"failed source op: "<<op<<" char="<<c<<" data="<<data<<" consumed="<<cpp.consumed<<"\nT="<<to_string(typed)<<"\nC="<<to_string(source_term(cpp))<<"\n";std::abort();}
 const Ptr current=app(sym("currentChar"),typed);
 const Ptr expected=cpp.chunks.empty()?sym("eofChar"):chr(current_char(cpp));
 assert(conv(nf(current,rules),expected,rules));
}

int main(){
 Signature sig;std::vector<Rule>rs;auto p=[](int i){return pvar(i);};
 Ptr N=sym("Nat"),B=sym("Bool"),CH=sym("Char"),CL=sym("Chars"),SEG=sym("Segment"),SEGS=sym("Segments"),SRC=sym("Source"),TOK=sym("Token"),OUT=sym("OutputList"),CTX=sym("Context"),PH=sym("Phase"),PARSER=sym("Parser");
 sig["Nat"]=type_sort();sig["zero"]=N;sig["succ"]=fn(N,N);sig["Bool"]=type_sort();sig["yes"]=B;sig["no"]=B;
 sig["Char"]=type_sort();sig["eofChar"]=CH;for(unsigned c=0;c<128;++c)sig["ch"+std::to_string(c)]=CH;sig["prior"]=CH;sig["cpNotin"]=CH;
 sig["Chars"]=type_sort();sig["nilChars"]=CL;sig["consChar"]=fn(CH,fn(CL,CL));sig["Segment"]=type_sort();sig["segment"]=fn(CL,fn(CL,SEG));sig["Segments"]=type_sort();sig["nilSegments"]=SEGS;sig["consSegment"]=fn(SEG,fn(SEGS,SEGS));sig["Source"]=type_sort();sig["source"]=fn(SEGS,fn(N,SRC));
 sig["OutputList"]=type_sort();sig["nilOutput"]=OUT;sig["consOutput"]=fn(CH,fn(OUT,OUT));sig["Token"]=type_sort();sig["token"]=fn(OUT,fn(B,TOK));
 sig["Context"]=type_sort();sig["dataContext"]=CTX;sig["attributeContext"]=CTX;sig["Phase"]=type_sort();for(const char*x:{"characterReference","suspended","dataState","attributeValueState"})sig[x]=PH;sig["Parser"]=type_sort();sig["parser"]=fn(CTX,fn(PH,fn(SRC,fn(TOK,PARSER))));
 sig["currentChar"]=fn(SRC,CH);sig["advanceExpecting"]=fn(SRC,fn(CH,SRC));sig["advanceSource"]=fn(SRC,SRC);sig["appendBefore"]=fn(CL,fn(CH,CL));sig["push"]=fn(SRC,fn(CH,SRC));sig["prependUnconsume"]=fn(SRC,fn(CL,SRC));sig["appendChunk"]=fn(SRC,fn(CL,SRC));sig["appendSegments"]=fn(SEGS,fn(CL,SEGS));sig["length"]=fn(CL,N);sig["minus"]=fn(N,fn(N,N));
 sig["suspendEntity"]=fn(PARSER,PARSER);sig["resumeEntity"]=fn(PARSER,PARSER);sig["appendToParser"]=fn(PARSER,fn(CL,PARSER));sig["advanceParser"]=fn(PARSER,fn(CH,PARSER));sig["completeNotin"]=fn(PARSER,PARSER);sig["appendOutput"]=fn(OUT,fn(CH,OUT));
 auto nil=sym("nilChars");
 // Character list arithmetic and the operation log's source primitives.
 rs.push_back(R(apps(sym("length"),nil),sym("zero")));rs.push_back(R(apps(sym("length"),apps(sym("consChar"),p(0),p(1))),app(sym("succ"),apps(sym("length"),p(1)))));
 rs.push_back(R(apps(sym("minus"),p(0),sym("zero")),p(0)));rs.push_back(R(apps(sym("minus"),app(sym("succ"),p(0)),app(sym("succ"),p(1))),apps(sym("minus"),p(0),p(1))));
 rs.push_back(R(apps(sym("appendBefore"),p(0),p(1)),apps(sym("consChar"),p(1),p(0))));
 rs.push_back(R(apps(sym("currentChar"),apps(sym("source"),apps(sym("consSegment"),apps(sym("segment"),p(0),apps(sym("consChar"),p(1),p(2))),p(3)),p(4))),p(1)));
 rs.push_back(R(app(sym("currentChar"),apps(sym("source"),sym("nilSegments"),p(0))),sym("eofChar")));
 Ptr srcHead=apps(sym("source"),apps(sym("consSegment"),apps(sym("segment"),p(0),apps(sym("consChar"),p(1),p(2))),p(3)),p(4));
 Ptr srcSame=apps(sym("source"),apps(sym("consSegment"),apps(sym("segment"),apps(sym("appendBefore"),p(0),p(1)),p(2)),p(3)),app(sym("succ"),p(4)));
 Ptr tailSource=apps(sym("source"),apps(sym("consSegment"),apps(sym("segment"),p(0),apps(sym("consChar"),p(1),nil)),apps(sym("consSegment"),p(2),p(3))),p(4));
 Ptr tailResult=apps(sym("source"),apps(sym("consSegment"),p(2),p(3)),app(sym("succ"),p(4)));
 rs.push_back(R(apps(sym("advanceSource"),tailSource),tailResult));
 Ptr endSource=apps(sym("source"),apps(sym("consSegment"),apps(sym("segment"),p(0),apps(sym("consChar"),p(1),nil)),sym("nilSegments")),p(2));
 Ptr endResult=apps(sym("source"),sym("nilSegments"),app(sym("succ"),p(2)));
 rs.push_back(R(apps(sym("advanceSource"),endSource),endResult));
 rs.push_back(R(apps(sym("advanceSource"),srcHead),srcSame));
 rs.push_back(R(apps(sym("advanceExpecting"),srcHead,p(1)),apps(sym("advanceSource"),srcHead)));
 // PushIfPossible fast rewind when the immediately previous character matches.
 rs.push_back(R(apps(sym("push"),apps(sym("source"),apps(sym("consSegment"),apps(sym("segment"),apps(sym("consChar"),p(0),p(1)),p(2)),p(3)),app(sym("succ"),p(4))),p(0)),apps(sym("source"),apps(sym("consSegment"),apps(sym("segment"),p(1),apps(sym("consChar"),p(0),p(2))),p(3)),p(4))));
 // Mismatch or segment-start falls back to kUnconsume prepend.
 rs.push_back(R(apps(sym("push"),apps(sym("source"),p(0),app(sym("succ"),p(1))),p(2)),apps(sym("source"),apps(sym("consSegment"),apps(sym("segment"),nil,apps(sym("consChar"),p(2),nil)),p(0)),p(1))));
 rs.push_back(R(apps(sym("prependUnconsume"),apps(sym("source"),p(0),p(1)),p(2)),apps(sym("source"),apps(sym("consSegment"),apps(sym("segment"),nil,p(2)),p(0)),apps(sym("minus"),p(1),apps(sym("length"),p(2))))));
 rs.push_back(R(apps(sym("appendSegments"),sym("nilSegments"),p(0)),apps(sym("consSegment"),apps(sym("segment"),nil,p(0)),sym("nilSegments"))));
 rs.push_back(R(apps(sym("appendSegments"),apps(sym("consSegment"),p(0),p(1)),p(2)),apps(sym("consSegment"),p(0),apps(sym("appendSegments"),p(1),p(2)))));
 rs.push_back(R(apps(sym("appendChunk"),apps(sym("source"),p(0),p(1)),p(2)),apps(sym("source"),app(apps(sym("appendSegments"),p(0)),p(2)),p(1))));

 // Token buffering and tokenizer state transitions around need-more.
 rs.push_back(R(apps(sym("appendOutput"),sym("nilOutput"),p(0)),apps(sym("consOutput"),p(0),sym("nilOutput"))));
 rs.push_back(R(apps(sym("appendOutput"),apps(sym("consOutput"),p(0),p(1)),p(2)),apps(sym("consOutput"),p(0),apps(sym("appendOutput"),p(1),p(2)))));
 for(const char* ctx:{"dataContext","attributeContext"}){
  rs.push_back(R(apps(sym("suspendEntity"),apps(sym("parser"),sym(ctx),sym("characterReference"),p(0),p(1))),apps(sym("parser"),sym(ctx),sym("suspended"),p(0),p(1))));
  rs.push_back(R(apps(sym("resumeEntity"),apps(sym("parser"),sym(ctx),sym("suspended"),p(0),p(1))),apps(sym("parser"),sym(ctx),sym("characterReference"),p(0),p(1))));
  rs.push_back(R(apps(sym("completeNotin"),apps(sym("parser"),sym(ctx),sym("characterReference"),p(0),apps(sym("token"),p(1),p(2)))),
     apps(sym("parser"),sym(ctx),sym(ctx==std::string_view("dataContext")?"dataState":"attributeValueState"),p(0),apps(sym("token"),apps(sym("appendOutput"),p(1),sym("cpNotin")),sym("yes")))));
 }
 rs.push_back(R(apps(sym("advanceParser"),apps(sym("parser"),p(0),p(1),p(2),p(3)),p(4)),apps(sym("parser"),p(0),p(1),apps(sym("advanceExpecting"),p(2),p(4)),p(3))));
 rs.push_back(R(apps(sym("appendToParser"),apps(sym("parser"),p(0),p(1),p(2),p(3)),p(4)),apps(sym("parser"),p(0),p(1),apps(sym("appendChunk"),p(2),p(4)),p(3))));

 auto check_type=[&](Ptr t,Ptr expected){auto ty=infer({},t,sig,rs);assert(ty&&conv(*ty,expected,rs));};
 Source one{{{"n!",0}},0};Ptr t=source_term(one);check_type(apps(sym("advanceExpecting"),t,chr('n')),SRC);
 auto drive=[&](Source s,const std::vector<std::pair<std::string,std::string>>& ops){Ptr semantic=source_term(s);for(const auto& [op,arg]:ops){if(op=="advance")check_source_op(semantic,s,"advance",arg[0],{},rs);else if(op=="push")check_source_op(semantic,s,"push",arg[0],{},rs);else if(op=="prepend")check_source_op(semantic,s,"prepend",0,arg,rs);else check_source_op(semantic,s,"append",0,arg,rs);assert(conv(nat(s.consumed),nat(s.consumed),rs));}return std::pair<Source,Ptr>{std::move(s),std::move(semantic)};};
 (void)drive;
 // One-character Push fast path: previous character matches in same segment.
 {Source s{{{"n!",0}},0};Ptr q=source_term(s);check_source_op(q,s,"advance",'n',{},rs);check_source_op(q,s,"push",'n',{},rs);}
 // Two-character rollback uses Push(second), Push(first), both same segment.
 {Source s{{{"no!",0}},0};Ptr q=source_term(s);for(char c:std::string("no"))check_source_op(q,s,"advance",c,{},rs);check_source_op(q,s,"push",'o',{},rs);check_source_op(q,s,"push",'n',{},rs);}
 // At a segment start, PushIfPossible fails and Push prepends a new segment.
 {Source s{{{"n",0},{"!",0}},0};Ptr q=source_term(s);check_source_op(q,s,"advance",'n',{},rs);check_source_op(q,s,"push",'n',{},rs);}
 // A mismatch also takes the prepend fallback instead of rewinding.
 {Source s{{{"ab!",0}},0};Ptr q=source_term(s);check_source_op(q,s,"advance",'a',{},rs);check_source_op(q,s,"push",'x',{},rs);}
 // >2 chars use Prepend(kUnconsume), followed by replay of the matched "not";
 // input "notit!" leaves its unmatched 'i' at the current cursor.
 Source correction{{{"not",0},{"it!",0}},0};Ptr correction_term=source_term(correction);for(char c:std::string("noti"))check_source_op(correction_term,correction,"advance",c,{},rs);check_source_op(correction_term,correction,"prepend",0,"noti",rs);for(char c:std::string("not"))check_source_op(correction_term,correction,"advance",c,{},rs);assert(current_char(correction)=='i');
 // Compose not-enough rollback, tokenizer suspension, append, resume, source
 // replay of "notin;", and the success handoff in both Data and attribute contexts.
 for(TokenContext ctx:{TokenContext::Data,TokenContext::Attribute}){
  Source src{{{"not",0}},0};Ptr st=source_term(src);for(char c:std::string("not"))check_source_op(st,src,"advance",c,{},rs);check_source_op(st,src,"prepend",0,"not",rs);
  Token tok{{"prior"},false};Parser parser{ctx,Phase::CharacterReference,src,tok};Ptr pt=parser_term(parser);Ptr suspended=nf(app(sym("suspendEntity"),pt),rs);suspend(parser);assert(conv(suspended,parser_term(parser),rs));
  append(parser.source,"in;");pt=nf(apps(sym("appendToParser"),suspended,char_list("in;")),rs);assert(conv(pt,parser_term(parser),rs));
  resume(parser);pt=nf(app(sym("resumeEntity"),pt),rs);assert(conv(pt,parser_term(parser),rs));
  for(char c:std::string("notin;")){advance_expecting(parser.source,c);pt=nf(apps(sym("advanceParser"),pt,chr(c)),rs);assert(conv(pt,parser_term(parser),rs));}
  complete_named(parser,"cpNotin");pt=nf(app(sym("completeNotin"),pt),rs);assert(conv(pt,parser_term(parser),rs));
 }
 std::println("Blink segmented entity source: rollback, longest-prefix replay, and Data/attribute suspension traces agree after each typed operation");
}
