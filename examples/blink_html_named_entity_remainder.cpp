#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
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
static std::string walk_name(size_t id){return"walk_"+std::to_string(id);}
static std::string prefix_name(size_t id){return"prefix_"+std::to_string(id);}
static Ptr entry_term(size_t i){const auto&e=table[i];return apps(sym("entity"),string_term(e.name),cp(e.first),e.second?app(sym("someSecond"),cp(*e.second)):sym("noSecond"),sym(e.name.back()==';'?"true":"false"));}
struct Dec{bool need_more=false,found=false,guard_reject=false;unsigned first=0,second=0;bool has_second=false;std::string consumed,remainder;size_t cursor=0;};
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
  struct Case{std::string input;bool attr,eof;};
  const std::vector<Case>cases={{"amp;",true,true},{"notin;",false,true},{"notit;",false,true},{"notit;",true,true},{"amp!",true,false},{"amp=",true,true},{"NotEqualTilde;",false,true},{"not",true,true},{"noti",false,true},{"xyz",false,true},{"CounterClockwiseContourIntegral;",false,true},{"copyx",false,true}};
  using Clock=std::chrono::steady_clock;auto started=Clock::now();Trie trie=build_trie();
  Signature sig;std::vector<Rule>rs;auto p=[](size_t i){return pvar(i);};
  Ptr B=sym("Bool"),C=sym("Char"),CL=sym("CharList"),E=sym("Entity"),O=sym("OptionCp"),RT=sym("Result"),N=sym("Nat");
  sig["Nat"]=type_sort();sig["z"]=N;sig["s"]=fn(N,N);sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;sig["Char"]=type_sort();sig["emptyChar"]=C;for(unsigned i=0;i<128;++i)sig["ch"+std::to_string(i)]=C;
  sig["CharList"]=type_sort();sig["nilChar"]=CL;sig["consChar"]=fn(C,fn(CL,CL));sig["Codepoint"]=type_sort();sig["OptionCp"]=type_sort();sig["noSecond"]=O;sig["someSecond"]=fn(sym("Codepoint"),O);
  for(const auto&e:table){sig["cp"+std::to_string(e.first)]=sym("Codepoint");if(e.second)sig["cp"+std::to_string(*e.second)]=sym("Codepoint");}
  sig["Entity"]=type_sort();sig["noEntry"]=E;sig["entity"]=fn(CL,fn(sym("Codepoint"),fn(O,fn(B,E))));sig["Result"]=type_sort();sig["noEntity"]=fn(N,fn(CL,RT));sig["guardReject"]=fn(N,fn(CL,RT));sig["needMore"]=fn(N,fn(CL,RT));sig["decoded"]=fn(sym("Codepoint"),fn(O,fn(N,fn(CL,fn(CL,RT)))));
  sig["prefer"]=fn(E,fn(E,E));sig["walkStart"]=fn(CL,fn(E,E));sig["prefixStart"]=fn(CL,B);
  sig["lastChar"]=fn(CL,C);sig["head"]=fn(CL,C);sig["after"]=fn(CL,fn(CL,CL));sig["length"]=fn(CL,N);sig["eqChar"]=fn(C,fn(C,B));sig["andB"]=fn(B,fn(B,B));sig["orB"]=fn(B,fn(B,B));sig["notB"]=fn(B,B);sig["isAlnum"]=fn(C,B);
  sig["accept"]=fn(E,fn(CL,fn(B,B)));sig["acceptName"]=fn(CL,fn(CL,fn(B,B)));sig["acceptNext"]=fn(C,B);sig["resultOf"]=fn(E,fn(N,fn(CL,fn(B,RT))));sig["resultIf"]=fn(B,fn(sym("Codepoint"),fn(O,fn(CL,fn(CL,fn(CL,fn(N,RT)))))));sig["notEnough"]=fn(B,fn(CL,fn(B,B)));sig["finishLookup"]=fn(B,fn(E,fn(N,fn(CL,fn(B,RT)))));sig["lookup"]=fn(N,fn(CL,fn(B,fn(B,RT))));
  sig["inputAt"]=fn(N,CL);sig["attrAt"]=fn(N,B);sig["eofAt"]=fn(N,B);sig["observe"]=fn(N,RT);sig["append"]=fn(CL,fn(CL,CL));sig["resume"]=fn(RT,fn(CL,fn(N,fn(B,fn(B,RT)))));
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
  rs.push_back(R(apps(sym("lookup"),p(0),p(1),p(2),p(3)),apps(sym("finishLookup"),apps(sym("notEnough"),app(sym(prefix_name(trie.root->id)),p(1)),p(1),p(3)),apps(sym(walk_name(trie.root->id)),p(1),sym("noEntry")),p(0),p(1),p(2))));
  rs.push_back(R(apps(sym("append"),sym("nilChar"),p(0)),p(0)));rs.push_back(R(apps(sym("append"),apps(sym("consChar"),p(0),p(1)),p(2)),apps(sym("consChar"),p(0),apps(sym("append"),p(1),p(2)))));
  rs.push_back(R(apps(sym("resume"),apps(sym("needMore"),p(0),p(1)),p(2),p(3),p(4),p(5)),apps(sym("lookup"),p(3),apps(sym("append"),p(1),p(2)),p(4),p(5))));rs.push_back(R(apps(sym("resume"),p(0),p(1),p(2),p(3),p(4)),p(0)));
  for(size_t i=0;i<cases.size();++i)rs.push_back(R(app(sym("observe"),nat(i)),apps(sym("lookup"),nat(i),string_term(cases[i].input),cases[i].attr?yes:no,cases[i].eof?yes:no)));
  const auto rules_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-started).count();std::println("CSV {} rows; trie {} states; generated {} per-state rules in {} ms",table.size(),trie.nodes.size(),rs.size(),rules_ms);
  auto index_start=Clock::now();RuleIndex indexed(rs);const auto index_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-index_start).count();
  auto ty=infer({},app(sym("observe"),nat(0)),sig,indexed);assert(ty&&conv(*ty,RT,indexed));auto cty=infer({},apps(sym("resume"),apps(sym("needMore"),nat(0),string_term("not")),string_term("in;"),nat(0),no,yes),sig,indexed);assert(cty&&conv(*cty,RT,indexed));std::println("RuleIndex built in {} ms; observers typechecked at {} ms total",index_ms,std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-started).count());
  for(size_t i=0;i<cases.size();++i){auto case_start=Clock::now();auto actual=nf(app(sym("observe"),nat(i)),indexed);const auto reduce_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-case_start).count();auto expected=dec_term(cpp_lookup(cases[i].input,cases[i].attr,cases[i].eof));auto actual_type=infer({},actual,sig,indexed);if(!actual_type||!conv(*actual_type,RT,indexed)||!conv(actual,expected,indexed)){std::println("case {} mismatch/type error: actual={} expected={}",i,to_string(actual),to_string(expected));return 2;}std::println("case {} ({}) agrees in {} ms",i,cases[i].input,reduce_ms);}
  auto cont_start=Clock::now();auto first=cpp_lookup("not",true,true);assert(first.need_more&&first.cursor==0&&first.remainder=="not");auto first_term=nf(app(sym("observe"),nat(7)),indexed);auto first_expected=dec_term(first);if(!conv(first_term,first_expected,indexed)){std::println("first-chunk mismatch actual={} expected={}",to_string(first_term),to_string(first_expected));return 3;}auto cont_actual=nf(apps(sym("resume"),first_term,string_term("in;"),nat(0),yes,yes),indexed);auto cont_expected=dec_term(cpp_lookup(first.remainder+"in;",true,true));auto cont_type=infer({},cont_actual,sig,indexed);if(!cont_type||!conv(*cont_type,RT,indexed)||!conv(cont_actual,cont_expected,indexed)){std::println("continuation mismatch/type error actual={} expected={}",to_string(cont_actual),to_string(cont_expected));return 3;}std::println("two-chunk continuation (not + in;) agrees in {} ms",std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-cont_start).count());
  std::println("Per-state named-entity model agrees with the full-table C++ reference on {} finite cases plus one chunk continuation",cases.size());
  std::println("Scope: generic list-prefix consumption computes accepted remainder; rejection and needMore return the unchanged list. Chunk continuation restarts lookup on the returned list plus the next chunk; SegmentedString segment/mutation behavior and tokenizer state are unmodeled.");
}
