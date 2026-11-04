#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <deque>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace lp;
struct Entity { std::string name; unsigned first; std::optional<unsigned> second; };
static std::vector<Entity> load_table(){
  std::ifstream in("vendor/chromium-src/third_party/blink/renderer/core/html/parser/html_entity_names.csv");assert(in);
  std::vector<Entity> rows;std::string line;
  while(std::getline(in,line)){
    auto comma=line.find(',');assert(comma!=std::string::npos);Entity e{};e.name=line.substr(1,comma-2);
    std::string vals=line.substr(comma+2);if(!vals.empty()&&vals.back()=='"')vals.pop_back();size_t pos=0;
    while(pos<vals.size()){auto end=vals.find(' ',pos);auto tok=vals.substr(pos,end==std::string::npos?end:end-pos);assert(tok.rfind("U+",0)==0);unsigned cp=std::stoul(tok.substr(2),nullptr,16);if(!e.first)e.first=cp;else e.second=cp;if(end==std::string::npos)break;pos=end+1;}
    rows.push_back(std::move(e));
  }
  assert(rows.size()==2231);return rows;
}
static const std::vector<Entity> table=load_table();
struct Node{size_t id;std::optional<size_t> terminal;std::map<unsigned char,Node*> edges;};
struct Trie{std::vector<std::unique_ptr<Node>> nodes;Node* root=nullptr;};
static Trie build_trie(){Trie trie;auto add=[&](){size_t id=trie.nodes.size();trie.nodes.push_back(std::make_unique<Node>(Node{id,std::nullopt,{}}));return trie.nodes.back().get();};trie.root=add();for(size_t i=0;i<table.size();++i){Node*n=trie.root;for(unsigned char c:table[i].name){auto it=n->edges.find(c);if(it==n->edges.end()){Node*child=add();n->edges.emplace(c,child);n=child;}else n=it->second;}n->terminal=i;}return trie;}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(size_t x){Ptr t=sym("z");while(x--)t=app(sym("s"),t);return t;}
static Ptr ch(unsigned char c){return sym("ch"+std::to_string(c));}
static Ptr string_term(const std::string&s){Ptr t=sym("nilChar");for(auto i=s.rbegin();i!=s.rend();++i)t=apps(sym("consChar"),ch((unsigned char)*i),t);return t;}
static Ptr cp(unsigned x){return sym("cp"+std::to_string(x));}
static Ptr output_term(const std::vector<unsigned>& output){Ptr r=sym("nilOutput");for(auto i=output.rbegin();i!=output.rend();++i)r=apps(sym("consOutput"),cp(*i),r);return r;}
static std::string walk_name(size_t id){return"walk_"+std::to_string(id);}
static std::string prefix_name(size_t id){return"prefix_"+std::to_string(id);}
static Ptr chunks_term(const std::vector<std::string>& chunks){Ptr r=sym("nilChunks");for(auto i=chunks.rbegin();i!=chunks.rend();++i)r=apps(sym("consChunk"),string_term(*i),r);return r;}
static Ptr source_term(const std::vector<std::string>& chunks,size_t offset=0){return apps(sym("source"),chunks_term(chunks),nat(offset));}
static Ptr entry_term(size_t i){const auto&e=table[i];return apps(sym("entity"),string_term(e.name),cp(e.first),e.second?app(sym("someSecond"),cp(*e.second)):sym("noSecond"),sym(e.name.back()==';'?"true":"false"));}
struct Dec{bool need_more=false,found=false,guard_reject=false;unsigned first=0,second=0;bool has_second=false;std::string consumed,remainder;size_t cursor=0;};
enum class TokContext{Data,Attribute};enum class TokPhase{CharacterReference,Suspended,Data,AttributeValue};
struct TokState{TokContext context;TokPhase phase;std::vector<unsigned> output;bool has_entity=false;std::string remaining;size_t offset=0;};
static const char* ctx_name(TokContext c){return c==TokContext::Data?"dataContext":"attributeContext";}
static const char* phase_name(TokPhase p){switch(p){case TokPhase::CharacterReference:return"characterReference";case TokPhase::Suspended:return"suspended";case TokPhase::Data:return"dataState";case TokPhase::AttributeValue:return"attributeValueState";}return"suspended";}
struct SegCursor{std::string text;size_t pos=0;};struct SegSource{std::deque<SegCursor> chunks;size_t offset=0;};
static char current(const SegSource&s){assert(!s.chunks.empty()&&s.chunks.front().pos<s.chunks.front().text.size());return s.chunks.front().text[s.chunks.front().pos];}
static void advance_expecting(SegSource&s,char c){assert(current(s)==c);auto&h=s.chunks.front();++h.pos;++s.offset;if(h.pos==h.text.size())s.chunks.pop_front();}
static void push_char(SegSource&s,char c){assert(s.offset);if(!s.chunks.empty()&&s.chunks.front().pos&&s.chunks.front().text[s.chunks.front().pos-1]==c){--s.chunks.front().pos;--s.offset;}else{s.chunks.push_front({std::string(1,c),0});--s.offset;}}
static void prepend_unconsume(SegSource&s,const std::string&v){assert(v.size()<=s.offset);if(!v.empty()){s.chunks.push_front({v,0});s.offset-=v.size();}}
static std::string logical_source(const SegSource&s){std::string out;for(const auto&seg:s.chunks)out.append(seg.text,seg.pos,std::string::npos);return out;}
static bool starts(const std::string&a,const std::string&b);
static size_t probe_length(const std::string&input){size_t n=0;for(size_t k=1;k<=input.size();++k){const auto prefix=input.substr(0,k);bool any=std::any_of(table.begin(),table.end(),[&](const Entity&e){return starts(prefix,e.name);});if(!any)break;n=k;}return n;}
struct ProbeResult{std::string logical;size_t offset=0,probed=0,pushes=0;bool prepended=false;};
static ProbeResult cpp_consume_named_entity(const std::vector<std::string>&chunks,const Dec&d){SegSource source;for(const auto&chunk:chunks)if(!chunk.empty())source.chunks.push_back({chunk,0});std::string all;for(const auto&chunk:chunks)all+=chunk;size_t probed=probe_length(all);std::string speculative=all.substr(0,probed);for(char c:speculative)advance_expecting(source,c);size_t match_length=d.found?d.consumed.size():0;const bool correction=probed>match_length;if(correction){if(probed<=2){for(auto i=speculative.rbegin();i!=speculative.rend();++i)push_char(source,*i);}else prepend_unconsume(source,speculative);}if(d.found&&correction)for(char c:d.consumed)advance_expecting(source,c);return{logical_source(source),source.offset,probed,correction&&probed<=2?probed:0,correction&&probed>2};}
static TokState cpp_apply_entity(TokContext context,const Dec&d,const ProbeResult&probe,std::vector<unsigned> prior={}){TokState s{context,TokPhase::CharacterReference,std::move(prior),false,probe.logical,probe.offset};if(d.need_more){s.phase=TokPhase::Suspended;return s;}if(d.found){s.output.push_back(d.first);if(d.has_second)s.output.push_back(d.second);s.has_entity=true;}else s.output.push_back(38);s.phase=context==TokContext::Data?TokPhase::Data:TokPhase::AttributeValue;return s;}
static bool starts(const std::string&a,const std::string&b){return b.size()>=a.size()&&b.compare(0,a.size(),a)==0;}
static bool ascii_alnum(char c){return(c>='0'&&c<='9')||(c>='A'&&c<='Z')||(c>='a'&&c<='z');}
static Dec cpp_lookup(const std::string&in,bool attr,bool eof){
  const Entity*best=nullptr;for(const auto&e:table)if(starts(e.name,in)&&(!best||e.name.size()>=best->name.size()))best=&e;
  if(eof&&!in.empty()&&in.back()!=';'&&std::any_of(table.begin(),table.end(),[&](const Entity&e){return starts(in,e.name);}))return{true,false,false,0,0,false,"",in};
  if(!best)return{false,false,false,0,0,false,"",in};char next=in.size()>best->name.size()?in[best->name.size()]:'\0';
  if(best->name.back()!=';'&&attr&&(ascii_alnum(next)||next=='='))return{false,false,true,0,0,false,"",in};
  return{false,true,false,best->first,best->second.value_or(0),best->second.has_value(),best->name,in.substr(best->name.size()),best->name.size()};
}
static Ptr dec_term(const Dec&d){auto rem=string_term(d.remainder),offset=nat(d.cursor);if(d.need_more)return apps(sym("needMore"),offset,rem);if(d.guard_reject)return apps(sym("guardReject"),offset,rem);if(!d.found)return apps(sym("noEntity"),offset,rem);return apps(sym("decoded"),cp(d.first),d.has_second?app(sym("someSecond"),cp(d.second)):sym("noSecond"),offset,string_term(d.consumed),rem);}

int main(){
  struct Case{std::string input;bool attr,eof;std::vector<std::string> chunks;};
  const std::vector<Case>cases={{"amp;",true,true,{"am","p;"}},{"notin;",false,true,{"not","in;"}},{"notit;",false,true,{"not","it;"}},{"notit;",true,true,{"no","tit;"}},{"amp!",true,false,{"a","mp!"}},{"amp=",true,true,{"am","p="}},{"NotEqualTilde;",false,true,{"NotEqual","Tilde;"}},{"not",true,true,{"n","ot"}},{"noti",false,true,{"no","ti"}},{"xyz",false,true,{"x","yz"}},{"CounterClockwiseContourIntegral;",false,true,{"CounterClockwise","ContourIntegral;"}},{"copyx",false,true,{"cop","yx"}},{"a",false,true,{"a"}},{"am",false,true,{"a","m"}}};
  using Clock=std::chrono::steady_clock;auto started=Clock::now();Trie trie=build_trie();
  Signature sig;std::vector<Rule>rs;auto p=[](size_t i){return pvar(i);};
  Ptr B=sym("Bool"),C=sym("Char"),CL=sym("CharList"),E=sym("Entity"),O=sym("OptionCp"),RT=sym("Result"),N=sym("Nat"),CHUNKS=sym("Chunks"),SRC=sym("Source"),SO=sym("SourceOutcome");
  sig["Nat"]=type_sort();sig["z"]=N;sig["s"]=fn(N,N);sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;sig["Char"]=type_sort();sig["emptyChar"]=C;for(unsigned i=0;i<128;++i)sig["ch"+std::to_string(i)]=C;
  sig["CharList"]=type_sort();sig["nilChar"]=CL;sig["Chunks"]=type_sort();sig["nilChunks"]=CHUNKS;sig["consChunk"]=fn(CL,fn(CHUNKS,CHUNKS));sig["Source"]=type_sort();sig["source"]=fn(CHUNKS,fn(N,SRC));sig["flattenSource"]=fn(SRC,CL);sig["flattenChunks"]=fn(CHUNKS,CL);sig["append"]=fn(CL,fn(CL,CL));sig["consChar"]=fn(C,fn(CL,CL));sig["Codepoint"]=type_sort();sig["cp38"]=sym("Codepoint");sig["OptionCp"]=type_sort();sig["noSecond"]=O;sig["someSecond"]=fn(sym("Codepoint"),O);
  sig["cp65"]=sym("Codepoint");for(const auto&e:table){sig["cp"+std::to_string(e.first)]=sym("Codepoint");if(e.second)sig["cp"+std::to_string(*e.second)]=sym("Codepoint");}
  sig["Entity"]=type_sort();sig["noEntry"]=E;sig["entity"]=fn(CL,fn(sym("Codepoint"),fn(O,fn(B,E))));sig["Result"]=type_sort();sig["noEntity"]=fn(N,fn(CL,RT));sig["guardReject"]=fn(N,fn(CL,RT));sig["needMore"]=fn(N,fn(CL,RT));sig["decoded"]=fn(sym("Codepoint"),fn(O,fn(N,fn(CL,fn(CL,RT)))));
  sig["prefer"]=fn(E,fn(E,E));sig["walkStart"]=fn(CL,fn(E,E));sig["prefixStart"]=fn(CL,B);
  sig["lastChar"]=fn(CL,C);sig["head"]=fn(CL,C);sig["after"]=fn(CL,fn(CL,CL));sig["length"]=fn(CL,N);sig["eqChar"]=fn(C,fn(C,B));sig["andB"]=fn(B,fn(B,B));sig["orB"]=fn(B,fn(B,B));sig["notB"]=fn(B,B);sig["isAlnum"]=fn(C,B);
  sig["accept"]=fn(E,fn(CL,fn(B,B)));sig["acceptName"]=fn(CL,fn(CL,fn(B,B)));sig["acceptNext"]=fn(C,B);sig["resultOf"]=fn(E,fn(N,fn(CL,fn(B,RT))));sig["resultIf"]=fn(B,fn(sym("Codepoint"),fn(O,fn(CL,fn(CL,fn(CL,fn(N,RT)))))));sig["notEnough"]=fn(B,fn(CL,fn(B,B)));sig["finishLookup"]=fn(B,fn(E,fn(N,fn(CL,fn(B,RT)))));sig["lookup"]=fn(N,fn(CL,fn(B,fn(B,RT))));
  sig["inputAt"]=fn(N,SRC);sig["attrAt"]=fn(N,B);sig["eofAt"]=fn(N,B);sig["observe"]=fn(N,RT);sig["append"]=fn(CL,fn(CL,CL));sig["resume"]=fn(RT,fn(CL,fn(N,fn(B,fn(B,RT)))));
  sig["SourceOutcome"]=type_sort();sig["decodedAt"]=fn(sym("Codepoint"),fn(O,fn(SRC,SO)));sig["noEntityAt"]=fn(SRC,SO);sig["needMoreAt"]=fn(SRC,SO);sig["guardRejectAt"]=fn(SRC,SO);sig["compose"]=fn(RT,fn(SRC,SO));sig["consumeN"]=fn(N,fn(SRC,SRC));sig["sourceLogical"]=fn(SRC,CL);sig["sourceOffset"]=fn(SRC,N);sig["outcomeLogical"]=fn(SO,CL);sig["outcomeOffset"]=fn(SO,N);
  Ptr OUT=sym("Output"),CTX=sym("Context"),PH=sym("Phase"),TS=sym("TokenizerState");sig["Output"]=type_sort();sig["nilOutput"]=OUT;sig["consOutput"]=fn(sym("Codepoint"),fn(OUT,OUT));sig["Context"]=type_sort();sig["dataContext"]=CTX;sig["attributeContext"]=CTX;sig["Phase"]=type_sort();for(const char*x:{"characterReference","suspended","dataState","attributeValueState"})sig[x]=PH;sig["TokenizerState"]=type_sort();sig["tokenizer"]=fn(CTX,fn(PH,fn(OUT,fn(B,fn(SRC,TS)))));sig["appendOutput"]=fn(OUT,fn(sym("Codepoint"),OUT));sig["applyEntity"]=fn(SO,fn(CTX,fn(OUT,fn(B,TS))));sig["appendSourceChunks"]=fn(CHUNKS,fn(CL,CHUNKS));sig["appendSourceChunk"]=fn(SRC,fn(CL,SRC));sig["tokenPhase"]=fn(TS,PH);sig["tokenOutput"]=fn(TS,OUT);sig["tokenEntity"]=fn(TS,B);sig["tokenSourceLogical"]=fn(TS,CL);sig["tokenSourceOffset"]=fn(TS,N);
  for(const auto&node:trie.nodes){sig[walk_name(node->id)]=fn(CL,fn(E,E));sig[prefix_name(node->id)]=fn(CL,B);}
  auto yes=sym("true"),no=sym("false");
  rs.push_back(R(apps(sym("eqChar"),p(0),p(0)),yes));rs.push_back(R(apps(sym("eqChar"),p(0),p(1)),no));rs.push_back(R(apps(sym("andB"),yes,p(0)),p(0)));rs.push_back(R(apps(sym("andB"),no,p(0)),no));rs.push_back(R(apps(sym("orB"),yes,p(0)),yes));rs.push_back(R(apps(sym("orB"),no,p(0)),p(0)));rs.push_back(R(app(sym("notB"),yes),no));rs.push_back(R(app(sym("notB"),no),yes));
  rs.push_back(R(apps(sym("prefer"),sym("noEntry"),p(0)),p(0)));rs.push_back(R(apps(sym("prefer"),p(0),p(1)),p(0)));
  // Each state has a distinct rewrite head, so RuleIndex dispatches to only
  // that node's outgoing character rules plus its wildcard fallback.
  for(const auto&holder:trie.nodes){const Node&node=*holder;const auto w=sym(walk_name(node.id)),q=sym(prefix_name(node.id));
    Ptr terminal=node.terminal?entry_term(*node.terminal):sym("noEntry");
    rs.push_back(R(apps(w,sym("nilChar"),p(0)),apps(sym("prefer"),terminal,p(0))));
    rs.push_back(R(app(q,sym("nilChar")),yes));
    for(const auto&[chv,child]:node.edges){
      Ptr next_terminal=child->terminal?entry_term(*child->terminal):sym("noEntry");
      rs.push_back(R(apps(w,apps(sym("consChar"),ch(chv),p(0)),p(1)),apps(sym(walk_name(child->id)),p(0),apps(sym("prefer"),next_terminal,p(1)))));
      rs.push_back(R(app(q,apps(sym("consChar"),ch(chv),p(0))),app(sym(prefix_name(child->id)),p(0))));
    }
    rs.push_back(R(apps(w,apps(sym("consChar"),p(0),p(1)),p(2)),p(2)));
    rs.push_back(R(app(q,apps(sym("consChar"),p(0),p(1))),no));
  }
  rs.push_back(R(app(sym("head"),sym("nilChar")),sym("emptyChar")));rs.push_back(R(app(sym("head"),apps(sym("consChar"),p(0),p(1))),p(0)));rs.push_back(R(app(sym("lastChar"),sym("nilChar")),sym("emptyChar")));rs.push_back(R(app(sym("lastChar"),apps(sym("consChar"),p(0),sym("nilChar"))),p(0)));rs.push_back(R(app(sym("lastChar"),apps(sym("consChar"),p(0),apps(sym("consChar"),p(1),p(2)))),app(sym("lastChar"),apps(sym("consChar"),p(1),p(2)))));rs.push_back(R(apps(sym("after"),sym("nilChar"),p(0)),p(0)));rs.push_back(R(apps(sym("after"),p(0),sym("nilChar")),sym("nilChar")));rs.push_back(R(apps(sym("after"),apps(sym("consChar"),p(0),p(1)),apps(sym("consChar"),p(2),p(3))),apps(sym("after"),p(1),p(3))));
  for(unsigned c=0;c<128;++c)if(ascii_alnum((char)c))rs.push_back(R(app(sym("isAlnum"),ch(c)),yes));rs.push_back(R(app(sym("isAlnum"),p(0)),no));
  rs.push_back(R(apps(sym("acceptName"),p(0),p(1),no),yes));rs.push_back(R(apps(sym("acceptName"),p(0),p(1),yes),apps(sym("acceptNext"),app(sym("head"),apps(sym("after"),p(0),p(1))),yes)));rs.push_back(R(apps(sym("acceptNext"),p(0),no),yes));rs.push_back(R(apps(sym("acceptNext"),p(0),yes),app(sym("notB"),apps(sym("orB"),app(sym("isAlnum"),p(0)),apps(sym("eqChar"),p(0),ch('='))))));rs.push_back(R(apps(sym("accept"),sym("noEntry"),p(0),p(1)),no));rs.push_back(R(apps(sym("accept"),apps(sym("entity"),p(0),p(1),p(2),yes),p(3),p(4)),yes));rs.push_back(R(apps(sym("accept"),apps(sym("entity"),p(0),p(1),p(2),no),p(3),p(4)),apps(sym("acceptName"),p(0),p(3),p(4))));
  rs.push_back(R(apps(sym("resultOf"),sym("noEntry"),p(0),p(1),p(2)),apps(sym("noEntity"),nat(0),p(1))));rs.push_back(R(apps(sym("resultOf"),apps(sym("entity"),p(0),p(1),p(2),p(3)),p(4),p(5),p(6)),apps(sym("resultIf"),apps(sym("accept"),apps(sym("entity"),p(0),p(1),p(2),p(3)),p(5),p(6)),p(1),p(2),p(0),apps(sym("after"),p(0),p(5)),p(5),app(sym("length"),p(0)))));rs.push_back(R(apps(sym("resultIf"),yes,p(0),p(1),p(2),p(3),p(4),p(5)),apps(sym("decoded"),p(0),p(1),p(5),p(2),p(3))));rs.push_back(R(apps(sym("resultIf"),no,p(0),p(1),p(2),p(3),p(4),p(5)),apps(sym("guardReject"),nat(0),p(4))));
  rs.push_back(R(apps(sym("notEnough"),p(0),p(1),no),no));rs.push_back(R(apps(sym("notEnough"),no,p(0),yes),no));rs.push_back(R(apps(sym("notEnough"),yes,sym("nilChar"),yes),no));rs.push_back(R(apps(sym("notEnough"),yes,p(0),yes),app(sym("notB"),apps(sym("eqChar"),app(sym("lastChar"),p(0)),ch(';')))));
  rs.push_back(R(apps(sym("length"),sym("nilChar")),nat(0)));rs.push_back(R(apps(sym("length"),apps(sym("consChar"),p(0),p(1))),app(sym("s"),app(sym("length"),p(1)))));
  rs.push_back(R(apps(sym("finishLookup"),yes,p(0),p(1),p(2),p(3)),apps(sym("needMore"),nat(0),p(2))));rs.push_back(R(apps(sym("finishLookup"),no,p(0),p(1),p(2),p(3)),apps(sym("resultOf"),p(0),p(1),p(2),p(3))));
  rs.push_back(R(apps(sym("append"),sym("nilChar"),p(0)),p(0)));rs.push_back(R(apps(sym("append"),apps(sym("consChar"),p(0),p(1)),p(2)),apps(sym("consChar"),p(0),apps(sym("append"),p(1),p(2)))));rs.push_back(R(apps(sym("flattenChunks"),sym("nilChunks")),sym("nilChar")));rs.push_back(R(apps(sym("flattenChunks"),apps(sym("consChunk"),p(0),p(1))),apps(sym("append"),p(0),app(sym("flattenChunks"),p(1)))));rs.push_back(R(app(sym("flattenSource"),apps(sym("source"),p(0),p(1))),app(sym("flattenChunks"),p(0))));
  rs.push_back(R(apps(sym("consumeN"),sym("z"),p(0)),p(0)));
  rs.push_back(R(apps(sym("consumeN"),app(sym("s"),p(0)),apps(sym("source"),sym("nilChunks"),p(1))),apps(sym("source"),sym("nilChunks"),p(1))));
  rs.push_back(R(apps(sym("consumeN"),p(0),apps(sym("source"),apps(sym("consChunk"),sym("nilChar"),p(1)),p(2))),apps(sym("consumeN"),p(0),apps(sym("source"),p(1),p(2)))));
  rs.push_back(R(apps(sym("consumeN"),app(sym("s"),p(0)),apps(sym("source"),apps(sym("consChunk"),apps(sym("consChar"),p(1),sym("nilChar")),p(2)),p(3))),apps(sym("consumeN"),p(0),apps(sym("source"),p(2),app(sym("s"),p(3))))));
  rs.push_back(R(apps(sym("consumeN"),app(sym("s"),p(0)),apps(sym("source"),apps(sym("consChunk"),apps(sym("consChar"),p(1),p(2)),p(3)),p(4))),apps(sym("consumeN"),p(0),apps(sym("source"),apps(sym("consChunk"),p(2),p(3)),app(sym("s"),p(4))))));
  rs.push_back(R(apps(sym("compose"),apps(sym("decoded"),p(0),p(1),p(2),p(3),p(4)),p(5)),apps(sym("decodedAt"),p(0),p(1),apps(sym("consumeN"),p(2),p(5)))));
  rs.push_back(R(apps(sym("compose"),apps(sym("noEntity"),p(0),p(1)),p(2)),apps(sym("noEntityAt"),p(2))));rs.push_back(R(apps(sym("compose"),apps(sym("needMore"),p(0),p(1)),p(2)),apps(sym("needMoreAt"),p(2))));rs.push_back(R(apps(sym("compose"),apps(sym("guardReject"),p(0),p(1)),p(2)),apps(sym("guardRejectAt"),p(2))));
  rs.push_back(R(app(sym("sourceLogical"),apps(sym("source"),p(0),p(1))),app(sym("flattenChunks"),p(0))));rs.push_back(R(app(sym("sourceOffset"),apps(sym("source"),p(0),p(1))),p(1)));
  rs.push_back(R(app(sym("outcomeLogical"),apps(sym("decodedAt"),p(0),p(1),p(2))),app(sym("sourceLogical"),p(2))));rs.push_back(R(app(sym("outcomeLogical"),apps(sym("noEntityAt"),p(0))),app(sym("sourceLogical"),p(0))));rs.push_back(R(app(sym("outcomeLogical"),apps(sym("needMoreAt"),p(0))),app(sym("sourceLogical"),p(0))));rs.push_back(R(app(sym("outcomeLogical"),apps(sym("guardRejectAt"),p(0))),app(sym("sourceLogical"),p(0))));
  rs.push_back(R(app(sym("outcomeOffset"),apps(sym("decodedAt"),p(0),p(1),p(2))),app(sym("sourceOffset"),p(2))));rs.push_back(R(app(sym("outcomeOffset"),apps(sym("noEntityAt"),p(0))),app(sym("sourceOffset"),p(0))));rs.push_back(R(app(sym("outcomeOffset"),apps(sym("needMoreAt"),p(0))),app(sym("sourceOffset"),p(0))));rs.push_back(R(app(sym("outcomeOffset"),apps(sym("guardRejectAt"),p(0))),app(sym("sourceOffset"),p(0))));
  rs.push_back(R(apps(sym("appendOutput"),sym("nilOutput"),p(0)),apps(sym("consOutput"),p(0),sym("nilOutput"))));rs.push_back(R(apps(sym("appendOutput"),apps(sym("consOutput"),p(0),p(1)),p(2)),apps(sym("consOutput"),p(0),apps(sym("appendOutput"),p(1),p(2)))));
  rs.push_back(R(apps(sym("appendSourceChunks"),sym("nilChunks"),p(0)),apps(sym("consChunk"),p(0),sym("nilChunks"))));rs.push_back(R(apps(sym("appendSourceChunks"),apps(sym("consChunk"),p(0),p(1)),p(2)),apps(sym("consChunk"),p(0),apps(sym("appendSourceChunks"),p(1),p(2)))));rs.push_back(R(apps(sym("appendSourceChunk"),apps(sym("source"),p(0),p(1)),p(2)),apps(sym("source"),app(apps(sym("appendSourceChunks"),p(0)),p(2)),p(1))));
  for(const char*ctx:{"dataContext","attributeContext"}){const char*mode=ctx==std::string_view("dataContext")?"dataState":"attributeValueState";
    rs.push_back(R(apps(sym("applyEntity"),apps(sym("needMoreAt"),p(0)),sym(ctx),p(1),p(2)),apps(sym("tokenizer"),sym(ctx),sym("suspended"),p(1),p(2),p(0))));
    rs.push_back(R(apps(sym("applyEntity"),apps(sym("decodedAt"),p(0),sym("noSecond"),p(1)),sym(ctx),p(2),p(3)),apps(sym("tokenizer"),sym(ctx),sym(mode),apps(sym("appendOutput"),p(2),p(0)),sym("true"),p(1))));
    rs.push_back(R(apps(sym("applyEntity"),apps(sym("decodedAt"),p(0),apps(sym("someSecond"),p(1)),p(2)),sym(ctx),p(3),p(4)),apps(sym("tokenizer"),sym(ctx),sym(mode),apps(sym("appendOutput"),apps(sym("appendOutput"),p(3),p(0)),p(1)),sym("true"),p(2))));
    rs.push_back(R(apps(sym("applyEntity"),apps(sym("noEntityAt"),p(0)),sym(ctx),p(1),p(2)),apps(sym("tokenizer"),sym(ctx),sym(mode),apps(sym("appendOutput"),p(1),sym("cp38")),p(2),p(0))));
    rs.push_back(R(apps(sym("applyEntity"),apps(sym("guardRejectAt"),p(0)),sym(ctx),p(1),p(2)),apps(sym("tokenizer"),sym(ctx),sym(mode),apps(sym("appendOutput"),p(1),sym("cp38")),p(2),p(0))));
  }
  rs.push_back(R(app(sym("tokenPhase"),apps(sym("tokenizer"),p(0),p(1),p(2),p(3),p(4))),p(1)));rs.push_back(R(app(sym("tokenOutput"),apps(sym("tokenizer"),p(0),p(1),p(2),p(3),p(4))),p(2)));rs.push_back(R(app(sym("tokenEntity"),apps(sym("tokenizer"),p(0),p(1),p(2),p(3),p(4))),p(3)));rs.push_back(R(app(sym("tokenSourceLogical"),apps(sym("tokenizer"),p(0),p(1),p(2),p(3),p(4))),app(sym("sourceLogical"),p(4))));rs.push_back(R(app(sym("tokenSourceOffset"),apps(sym("tokenizer"),p(0),p(1),p(2),p(3),p(4))),app(sym("sourceOffset"),p(4))));
  rs.push_back(R(apps(sym("lookup"),p(0),p(1),p(2),p(3)),apps(sym("finishLookup"),apps(sym("notEnough"),app(sym(prefix_name(trie.root->id)),app(sym("flattenSource"),p(1))),app(sym("flattenSource"),p(1)),p(3)),apps(sym(walk_name(trie.root->id)),app(sym("flattenSource"),p(1)),sym("noEntry")),p(0),app(sym("flattenSource"),p(1)),p(2))));
  rs.push_back(R(apps(sym("append"),sym("nilChar"),p(0)),p(0)));rs.push_back(R(apps(sym("append"),apps(sym("consChar"),p(0),p(1)),p(2)),apps(sym("consChar"),p(0),apps(sym("append"),p(1),p(2)))));
  rs.push_back(R(apps(sym("resume"),apps(sym("needMore"),p(0),p(1)),p(2),p(3),p(4),p(5)),apps(sym("lookup"),p(3),apps(sym("source"),apps(sym("consChunk"),apps(sym("append"),p(1),p(2)),sym("nilChunks")),nat(0)),p(4),p(5))));rs.push_back(R(apps(sym("resume"),p(0),p(1),p(2),p(3),p(4)),p(0)));
  for(size_t i=0;i<cases.size();++i)rs.push_back(R(app(sym("observe"),nat(i)),apps(sym("lookup"),nat(i),source_term(cases[i].chunks),cases[i].attr?yes:no,cases[i].eof?yes:no)));
  const auto rules_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-started).count();std::println("CSV {} rows; trie {} states; generated {} per-state rules in {} ms",table.size(),trie.nodes.size(),rs.size(),rules_ms);
  auto index_start=Clock::now();RuleIndex indexed(rs);const auto index_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-index_start).count();
  auto ty=infer({},app(sym("observe"),nat(0)),sig,indexed);assert(ty&&conv(*ty,RT,indexed));auto cty=infer({},apps(sym("resume"),apps(sym("needMore"),nat(0),string_term("not")),string_term("in;"),nat(0),no,yes),sig,indexed);assert(cty&&conv(*cty,RT,indexed));std::println("RuleIndex built in {} ms; observers typechecked at {} ms total",index_ms,std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-started).count());
  for(size_t i=0;i<cases.size();++i){auto case_start=Clock::now();auto actual=nf(app(sym("observe"),nat(i)),indexed);const auto reduce_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-case_start).count();auto d=cpp_lookup(cases[i].input,cases[i].attr,cases[i].eof);auto expected=dec_term(d);auto actual_type=infer({},actual,sig,indexed);if(!actual_type||!conv(*actual_type,RT,indexed)||!conv(actual,expected,indexed)){std::println("case {} mismatch/type error: actual={} expected={}",i,to_string(actual),to_string(expected));return 2;}
    Ptr src=source_term(cases[i].chunks);Ptr composed=nf(apps(sym("compose"),actual,src),indexed);auto trace=cpp_consume_named_entity(cases[i].chunks,d);auto logical=nf(app(sym("outcomeLogical"),composed),indexed);auto offset=nf(app(sym("outcomeOffset"),composed),indexed);
    if(!conv(logical,string_term(trace.logical),indexed)||!conv(offset,nat(trace.offset),indexed)){std::println("source effect {} mismatch actual logical={} offset={} expected logical={} offset={} probe={}",i,to_string(logical),to_string(offset),trace.logical,trace.offset,trace.probed);return 4;}
    if(cases[i].input=="amp;"&&trace.probed!=d.consumed.size())return 5;
    if(cases[i].input=="a"&&trace.pushes!=1)return 5;if(cases[i].input=="am"&&trace.pushes!=2)return 5;
    if(cases[i].input=="notit;"&&!cases[i].attr&&(!trace.prepended||trace.probed!=4||trace.offset!=3))return 5;
    if(cases[i].input=="notit;"&&cases[i].attr&&(!trace.prepended||trace.probed!=4||trace.offset!=0))return 5;
    std::println("case {} ({}) source effect agrees; probe={} Pushes={} Prepend={} global-offset={} in {} ms",i,cases[i].input,trace.probed,trace.pushes,trace.prepended,trace.offset,reduce_ms);}
  auto cont_start=Clock::now();auto first=cpp_lookup("not",true,true);assert(first.need_more&&first.cursor==0&&first.remainder=="not");auto first_term=nf(app(sym("observe"),nat(7)),indexed);auto first_expected=dec_term(first);if(!conv(first_term,first_expected,indexed)){std::println("first-chunk mismatch actual={} expected={}",to_string(first_term),to_string(first_expected));return 3;}auto cont_actual=nf(apps(sym("resume"),first_term,string_term("in;"),nat(0),yes,yes),indexed);auto cont_expected=dec_term(cpp_lookup(first.remainder+"in;",true,true));auto cont_type=infer({},cont_actual,sig,indexed);if(!cont_type||!conv(*cont_type,RT,indexed)||!conv(cont_actual,cont_expected,indexed)){std::println("continuation mismatch/type error actual={} expected={}",to_string(cont_actual),to_string(cont_expected));return 3;}std::println("two-chunk continuation (not + in;) agrees in {} ms",std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-cont_start).count());
  auto check_tokenizer=[&](Ptr typed,const TokState&cpp,const char*label){auto ty=infer({},typed,sig,indexed);if(!ty){std::println("tokenizer typing failed {}: {}",label,to_string(typed));std::abort();}if(!conv(*ty,TS,indexed)){std::println("tokenizer result type {} expected {}",to_string(*ty),to_string(TS));std::abort();}auto a=nf(app(sym("tokenPhase"),typed),indexed),b=nf(app(sym("tokenOutput"),typed),indexed),c=nf(app(sym("tokenEntity"),typed),indexed),d=nf(app(sym("tokenSourceLogical"),typed),indexed),e=nf(app(sym("tokenSourceOffset"),typed),indexed);bool ok=conv(a,sym(phase_name(cpp.phase)),indexed)&&conv(b,output_term(cpp.output),indexed)&&conv(c,sym(cpp.has_entity?"true":"false"),indexed)&&conv(d,string_term(cpp.remaining),indexed)&&conv(e,nat(cpp.offset),indexed);if(!ok){std::println("tokenizer mismatch {} phase={} output={} flag={} logical={} offset={} expectedPhase={} expectedOutput={} expectedFlag={} expectedLogical={} expectedOffset={}",label,to_string(a),to_string(b),to_string(c),to_string(d),to_string(e),phase_name(cpp.phase),to_string(output_term(cpp.output)),cpp.has_entity?"true":"false",cpp.remaining,cpp.offset);std::abort();}};
  for(TokContext ctx:{TokContext::Data,TokContext::Attribute}){
    const bool attr=ctx==TokContext::Attribute;const Ptr ctxTerm=sym(ctx_name(ctx));const std::vector<unsigned> prior={65};
    auto pendingDec=cpp_lookup("not",attr,true);assert(pendingDec.need_more);auto pendingProbe=cpp_consume_named_entity({"n","ot"},pendingDec);TokState cppPending=cpp_apply_entity(ctx,pendingDec,pendingProbe,prior);
    Ptr pendingSource=source_term({"n","ot"});Ptr pendingRaw=nf(apps(sym("lookup"),nat(0),pendingSource,attr?yes:no,yes),indexed);Ptr pendingOutcome=nf(apps(sym("compose"),pendingRaw,pendingSource),indexed);Ptr pendingTyped=nf(apps(sym("applyEntity"),pendingOutcome,ctxTerm,output_term(prior),no),indexed);check_tokenizer(pendingTyped,cppPending,attr?"attribute pending":"data pending");
    std::vector<std::string> resumedChunks={"n","ot","in;"};auto resumedDec=cpp_lookup("notin;",attr,true);auto resumedProbe=cpp_consume_named_entity(resumedChunks,resumedDec);TokState cppResumed=cpp_apply_entity(ctx,resumedDec,resumedProbe,prior);
    Ptr appended=nf(apps(sym("appendSourceChunk"),pendingSource,string_term("in;")),indexed);Ptr resumedRaw=nf(apps(sym("lookup"),nat(0),appended,attr?yes:no,yes),indexed);Ptr resumedOutcome=nf(apps(sym("compose"),resumedRaw,appended),indexed);Ptr resumedTyped=nf(apps(sym("applyEntity"),resumedOutcome,ctxTerm,output_term(prior),no),indexed);check_tokenizer(resumedTyped,cppResumed,attr?"attribute resumed":"data resumed");
  }
  for(size_t i:{size_t(0),size_t(5),size_t(6),size_t(9)}){const auto&c=cases[i];const auto d=cpp_lookup(c.input,c.attr,c.eof);const auto probe=cpp_consume_named_entity(c.chunks,d);TokContext ctx=c.attr?TokContext::Attribute:TokContext::Data;TokState cpp=cpp_apply_entity(ctx,d,probe,{65});Ptr src=source_term(c.chunks);Ptr raw=nf(app(sym("observe"),nat(i)),indexed);Ptr composed=nf(apps(sym("compose"),raw,src),indexed);Ptr typed=nf(apps(sym("applyEntity"),composed,sym(ctx_name(ctx)),output_term({65}),no),indexed);check_tokenizer(typed,cpp,c.input.c_str());}
  std::println("Per-state named-entity model agrees with the full-table C++ reference on {} finite cases plus one chunk continuation",cases.size());
  std::println("Scope: the trie observer still sees concatenated typed chunks. The C++ transcript checks Blink's prefix-probe and exact Push/Prepend rollback choices; tokenizer outcomes preserve/suspend token state, append decoded one/two codepoints and entity flag, or append literal ampersand. Comparisons project source to logical remainder and offset; segment partition and intermediate cursor positions are not compared.");
}
