#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace lp;

struct Entity { std::string name; unsigned first; std::optional<unsigned> second; };
struct Decoded { unsigned first=0, second=0; bool found=false, has_second=false; };
static std::vector<Entity> load_entities() {
  std::ifstream in("vendor/chromium-src/third_party/blink/renderer/core/html/parser/html_entity_names.csv");
  assert(in && "run from repository root: pinned Blink CSV is required");
  std::vector<Entity> out; std::string line;
  while (std::getline(in,line)) {
    const auto comma=line.find(','); assert(comma!=std::string::npos);
    std::string name=line.substr(1,comma-2), values=line.substr(comma+2);
    if (!values.empty() && values.back()=='"') values.pop_back();
    Entity e; e.name=name;
    size_t pos=0;
    while(pos<values.size()) {
      const auto end=values.find(' ',pos); const auto token=values.substr(pos,end==std::string::npos?end:end-pos);
      assert(token.rfind("U+",0)==0); unsigned cp=std::stoul(token.substr(2),nullptr,16);
      if (!e.first) e.first=cp; else e.second=cp;
      if(end==std::string::npos) break;
      pos=end+1;
    }
    out.push_back(e);
  }
  assert(out.size()==2231); return out;
}
static const std::vector<Entity> csv_entities=load_entities();
static const std::vector<Entity> entities=[] {
  const std::vector<std::string> selected={"amp;","amp","not;","not","notin;","NotEqualTilde;","copy;","copy","AElig;","AElig","notindot;"};
  std::vector<Entity> out;
  for(const auto& name:selected){auto it=std::find_if(csv_entities.begin(),csv_entities.end(),[&](const Entity&e){return e.name==name;});assert(it!=csv_entities.end());out.push_back(*it);}
  return out;
}();
struct TrieNode { std::optional<size_t> value; std::map<unsigned char,std::unique_ptr<TrieNode>> edges; };
static TrieNode build_trie() {
  TrieNode root;
  for(size_t i=0;i<entities.size();++i) {
    TrieNode* n=&root;
    for(unsigned char c:entities[i].name) { auto& child=n->edges[c]; if(!child) child=std::make_unique<TrieNode>(); n=child.get(); }
    n->value=i;
  }
  return root;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule R(Ptr a,Ptr b){return {std::move(a),std::move(b)};}
static Ptr nat(size_t x){Ptr t=sym("z");while(x--)t=app(sym("s"),t);return t;}
static Ptr chr(unsigned char c){return sym("ch"+std::to_string(c));}
static Ptr string_term(const std::string& s){Ptr t=sym("nilC");for(auto i=s.rbegin();i!=s.rend();++i)t=apps(sym("consC"),chr((unsigned char)*i),t);return t;}
static Ptr cp(unsigned x){return sym("cp"+std::to_string(x));}
static Ptr entity_term(size_t i){const auto&e=entities[i];return apps(sym("entity"),string_term(e.name),cp(e.first),e.second?app(sym("someSecond"),cp(*e.second)):sym("noSecond"),e.name.back()==';'?sym("true"):sym("false"));}
static Ptr trie_term(const TrieNode& n){Ptr edges=sym("nilEdge");for(auto i=n.edges.rbegin();i!=n.edges.rend();++i)edges=apps(sym("edge"),chr(i->first),trie_term(*i->second),edges);Ptr value=n.value?entity_term(*n.value):sym("noEntry");return apps(sym("node"),value,edges);}
static Ptr result_term(const Decoded& d){if(!d.found)return sym("noEntity");return apps(sym("decoded"),cp(d.first),d.has_second?app(sym("someSecond"),cp(d.second)):sym("noSecond"));}
static bool ascii_alnum(char c){return(c>='0'&&c<='9')||(c>='A'&&c<='Z')||(c>='a'&&c<='z');}
static Decoded cpp_lookup(const std::string& input,bool attribute,bool eof){
  const Entity* best=nullptr;
  for(const auto&e:entities)if(input.size()>=e.name.size()&&input.compare(0,e.name.size(),e.name)==0&&(!best||e.name.size()>=best->name.size()))best=&e;
  if(eof&&!input.empty()&&input.back()!=';'&&std::any_of(entities.begin(),entities.end(),[&](const Entity&e){return e.name.rfind(input,0)==0;}))return{};
  if(!best)return{};
  const char next=input.size()>best->name.size()?input[best->name.size()]:'\0';
  if(best->name.back()!=';'&&attribute&&(ascii_alnum(next)||next=='='))return{};
  return{best->first,best->second.value_or(0),true,best->second.has_value()};
}

int main(){
  struct Case{std::string input;bool attribute,eof;};
  const std::vector<Case> cases={{"notin;",false,true},{"notit;",false,true},{"notit;",true,true},{"ampx",false,true},{"ampx",true,true},{"amp;",true,true},{"NotEqualTilde;",false,true},{"xyz",false,true},{"not=",true,true},{"not",false,true},{"not ",true,true},{"NotEqualTilde",false,true}};
  const TrieNode trie=build_trie();
  Signature sig;std::vector<Rule>rs;auto p=[](size_t i){return pvar(i);};
  Ptr B=sym("Bool"),N=sym("Nat"),C=sym("Chars"),CH=sym("Char"),E=sym("Entity"),EL=sym("EdgeList"),T=sym("Trie"),O=sym("OptionCp"),RType=sym("Result");
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;sig["Nat"]=type_sort();sig["z"]=N;sig["s"]=fn(N,N);
  sig["Char"]=type_sort();sig["emptyChar"]=CH;for(unsigned c=0;c<128;c++)sig["ch"+std::to_string(c)]=CH;
  sig["Chars"]=type_sort();sig["nilC"]=C;sig["consC"]=fn(CH,fn(C,C));
  sig["Codepoint"]=type_sort();sig["noSecond"]=O;sig["OptionCp"]=type_sort();sig["someSecond"]=fn(sym("Codepoint"),O);
  for(const auto&e:entities){sig["cp"+std::to_string(e.first)]=sym("Codepoint");if(e.second)sig["cp"+std::to_string(*e.second)]=sym("Codepoint");}
  sig["Entity"]=type_sort();sig["noEntry"]=E;sig["entity"]=fn(C,fn(sym("Codepoint"),fn(O,fn(B,E))));
  sig["EdgeList"]=type_sort();sig["nilEdge"]=EL;sig["edge"]=fn(CH,fn(T,fn(EL,EL)));
  sig["Trie"]=type_sort();sig["node"]=fn(E,fn(EL,T));sig["dead"]=T;
  sig["Result"]=type_sort();sig["noEntity"]=RType;sig["decoded"]=fn(sym("Codepoint"),fn(O,RType));
  auto decls=std::vector<std::pair<const char*,Ptr>>{{"eqNat",fn(N,fn(N,B))},{"eqChar",fn(CH,fn(CH,B))},{"and",fn(B,fn(B,B))},{"or",fn(B,fn(B,B))},{"notB",fn(B,B)},{"length",fn(C,N)},{"ge",fn(N,fn(N,B))},{"findEdge",fn(EL,fn(CH,T))},{"step",fn(T,fn(CH,T))},{"terminal",fn(T,E)},{"prefer",fn(E,fn(E,E))},{"update",fn(T,fn(E,E))},{"walk",fn(T,fn(C,fn(E,E)))},{"walkNext",fn(T,fn(C,fn(E,E)))},{"prefix",fn(T,fn(C,B))},{"prefixNext",fn(T,fn(C,B))},{"lastChar",fn(C,CH)},{"head",fn(C,CH)},{"after",fn(C,fn(C,C))},{"isAlnum",fn(CH,B)},{"accept",fn(E,fn(C,fn(B,B)))},{"acceptName",fn(C,fn(C,fn(B,B)))},{"acceptNoSemi",fn(C,fn(B,B))},{"resultOf",fn(E,fn(C,fn(B,RType)))},{"lookup",fn(T,fn(C,fn(B,fn(B,RType))))},{"notEnough",fn(T,fn(C,fn(B,B)))},{"lookupDone",fn(B,fn(T,fn(C,fn(B,RType))))},{"tableAt",fn(N,T)},{"inputAt",fn(N,C)},{"attrAt",fn(N,B)},{"eofAt",fn(N,B)},{"observe",fn(N,RType)}};for(auto&[n,t]:decls)sig[n]=t;
  auto yes=sym("true"),no=sym("false");
  rs.push_back(R(apps(sym("eqChar"),p(0),p(0)),yes));rs.push_back(R(apps(sym("eqChar"),p(0),p(1)),no));
  rs.push_back(R(apps(sym("eqNat"),sym("z"),sym("z")),yes));rs.push_back(R(apps(sym("eqNat"),sym("z"),app(sym("s"),p(0))),no));rs.push_back(R(apps(sym("eqNat"),app(sym("s"),p(0)),sym("z")),no));rs.push_back(R(apps(sym("eqNat"),app(sym("s"),p(0)),app(sym("s"),p(1))),apps(sym("eqNat"),p(0),p(1))));
  rs.push_back(R(apps(sym("and"),yes,p(0)),p(0)));rs.push_back(R(apps(sym("and"),no,p(0)),no));rs.push_back(R(apps(sym("or"),yes,p(0)),yes));rs.push_back(R(apps(sym("or"),no,p(0)),p(0)));rs.push_back(R(app(sym("notB"),yes),no));rs.push_back(R(app(sym("notB"),no),yes));
  rs.push_back(R(app(sym("length"),sym("nilC")),sym("z")));rs.push_back(R(app(sym("length"),apps(sym("consC"),p(0),p(1))),app(sym("s"),app(sym("length"),p(1)))));
  rs.push_back(R(apps(sym("ge"),sym("z"),sym("z")),yes));rs.push_back(R(apps(sym("ge"),sym("z"),app(sym("s"),p(0))),no));rs.push_back(R(apps(sym("ge"),app(sym("s"),p(0)),sym("z")),yes));rs.push_back(R(apps(sym("ge"),app(sym("s"),p(0)),app(sym("s"),p(1))),apps(sym("ge"),p(0),p(1))));
  rs.push_back(R(apps(sym("findEdge"),sym("nilEdge"),p(0)),sym("dead")));rs.push_back(R(apps(sym("findEdge"),apps(sym("edge"),p(0),p(1),p(2)),p(3)),apps(sym("findEdgeIf"),apps(sym("eqChar"),p(0),p(3)),p(1),p(2),p(3))));sig["findEdgeIf"]=fn(B,fn(T,fn(EL,fn(CH,T))));rs.push_back(R(apps(sym("findEdgeIf"),yes,p(0),p(1),p(2)),p(0)));rs.push_back(R(apps(sym("findEdgeIf"),no,p(0),p(1),p(2)),apps(sym("findEdge"),p(1),p(2))));
  rs.push_back(R(apps(sym("step"),apps(sym("node"),p(0),p(1)),p(2)),apps(sym("findEdge"),p(1),p(2))));
  rs.push_back(R(app(sym("terminal"),apps(sym("node"),p(0),p(1))),p(0)));rs.push_back(R(app(sym("terminal"),sym("dead")),sym("noEntry")));
  rs.push_back(R(apps(sym("prefer"),sym("noEntry"),p(0)),p(0)));rs.push_back(R(apps(sym("prefer"),p(0),p(1)),p(0)));rs.push_back(R(apps(sym("update"),p(0),p(1)),apps(sym("prefer"),app(sym("terminal"),p(0)),p(1))));
  rs.push_back(R(apps(sym("walk"),p(0),sym("nilC"),p(1)),apps(sym("update"),p(0),p(1))));rs.push_back(R(apps(sym("walk"),p(0),apps(sym("consC"),p(1),p(2)),p(3)),apps(sym("walkNext"),apps(sym("step"),p(0),p(1)),p(2),apps(sym("update"),p(0),p(3)))));
  rs.push_back(R(apps(sym("walkNext"),sym("dead"),p(0),p(1)),p(1)));rs.push_back(R(apps(sym("walkNext"),p(0),p(1),p(2)),apps(sym("walk"),p(0),p(1),p(2))));
  rs.push_back(R(apps(sym("prefix"),p(0),sym("nilC")),yes));rs.push_back(R(apps(sym("prefix"),p(0),apps(sym("consC"),p(1),p(2))),apps(sym("prefixNext"),apps(sym("step"),p(0),p(1)),p(2))));rs.push_back(R(apps(sym("prefixNext"),sym("dead"),p(0)),no));rs.push_back(R(apps(sym("prefixNext"),p(0),p(1)),apps(sym("prefix"),p(0),p(1))));
  rs.push_back(R(app(sym("head"),sym("nilC")),sym("emptyChar")));rs.push_back(R(app(sym("head"),apps(sym("consC"),p(0),p(1))),p(0)));
  rs.push_back(R(app(sym("lastChar"),sym("nilC")),sym("emptyChar")));rs.push_back(R(app(sym("lastChar"),apps(sym("consC"),p(0),sym("nilC"))),p(0)));rs.push_back(R(app(sym("lastChar"),apps(sym("consC"),p(0),apps(sym("consC"),p(1),p(2)))),app(sym("lastChar"),apps(sym("consC"),p(1),p(2)))));
  rs.push_back(R(apps(sym("after"),sym("nilC"),p(0)),p(0)));rs.push_back(R(apps(sym("after"),apps(sym("consC"),p(0),p(1)),sym("nilC")),sym("nilC")));rs.push_back(R(apps(sym("after"),apps(sym("consC"),p(0),p(1)),apps(sym("consC"),p(2),p(3))),apps(sym("after"),p(1),p(3))));
  for(unsigned c=0;c<128;c++) if(ascii_alnum((char)c)) rs.push_back(R(app(sym("isAlnum"),chr(c)),yes));
  rs.push_back(R(app(sym("isAlnum"),p(0)),no));
  rs.push_back(R(apps(sym("acceptName"),p(0),p(1),no),yes));rs.push_back(R(apps(sym("acceptName"),p(0),p(1),yes),apps(sym("acceptNoSemi"),app(sym("head"),apps(sym("after"),p(0),p(1))),yes)));
  rs.push_back(R(apps(sym("acceptNoSemi"),p(0),no),yes));rs.push_back(R(apps(sym("acceptNoSemi"),p(0),yes),app(sym("notB"),apps(sym("or"),app(sym("isAlnum"),p(0)),apps(sym("eqChar"),p(0),chr('='))))));
  rs.push_back(R(apps(sym("accept"),sym("noEntry"),p(0),p(1)),no));rs.push_back(R(apps(sym("accept"),apps(sym("entity"),p(0),p(1),p(2),yes),p(3),p(4)),yes));rs.push_back(R(apps(sym("accept"),apps(sym("entity"),p(0),p(1),p(2),no),p(3),p(4)),apps(sym("acceptName"),p(0),p(3),p(4))));
  rs.push_back(R(apps(sym("resultOf"),sym("noEntry"),p(0),p(1)),sym("noEntity")));rs.push_back(R(apps(sym("resultOf"),apps(sym("entity"),p(0),p(1),p(2),p(3)),p(4),p(5)),apps(sym("resultIf"),apps(sym("accept"),apps(sym("entity"),p(0),p(1),p(2),p(3)),p(4),p(5)),p(1),p(2))));sig["resultIf"]=fn(B,fn(sym("Codepoint"),fn(O,RType)));rs.push_back(R(apps(sym("resultIf"),yes,p(0),p(1)),apps(sym("decoded"),p(0),p(1))));rs.push_back(R(apps(sym("resultIf"),no,p(0),p(1)),sym("noEntity")));
  rs.push_back(R(apps(sym("notEnough"),p(0),p(1),no),no));rs.push_back(R(apps(sym("notEnough"),p(0),sym("nilC"),yes),no));rs.push_back(R(apps(sym("notEnough"),p(0),p(1),yes),apps(sym("and"),apps(sym("prefix"),p(0),p(1)),app(sym("notB"),apps(sym("eqChar"),app(sym("lastChar"),p(1)),chr(';'))))));
  rs.push_back(R(apps(sym("lookupDone"),yes,p(0),p(1),p(2)),sym("noEntity")));rs.push_back(R(apps(sym("lookupDone"),no,p(0),p(1),p(2)),apps(sym("resultOf"),apps(sym("walk"),p(0),p(1),sym("noEntry")),p(1),p(2))));
  rs.push_back(R(apps(sym("lookup"),p(0),p(1),p(2),p(3)),apps(sym("lookupDone"),apps(sym("notEnough"),p(0),p(1),p(3)),p(0),p(1),p(2))));
  rs.push_back(R(app(sym("observe"),p(0)),apps(sym("lookup"),app(sym("tableAt"),p(0)),app(sym("inputAt"),p(0)),app(sym("attrAt"),p(0)),app(sym("eofAt"),p(0)))));
  const Ptr root=trie_term(trie);
  for(size_t i=0;i<cases.size();++i){rs.push_back(R(app(sym("tableAt"),nat(i)),root));rs.push_back(R(app(sym("inputAt"),nat(i)),string_term(cases[i].input)));rs.push_back(R(app(sym("attrAt"),nat(i)),cases[i].attribute?yes:no));rs.push_back(R(app(sym("eofAt"),nat(i)),cases[i].eof?yes:no));}
  std::println("CSV parser validated {} rows; embedded λΠ trie has {} selected entries",csv_entities.size(),entities.size());
  auto ty=infer({},app(sym("observe"),nat(0)),sig,rs);assert(ty&&conv(*ty,RType,rs));std::println("λΠ observer typechecked");
  for(size_t i=0;i<cases.size();++i){auto got=nf(app(sym("observe"),nat(i)),rs);auto expected=result_term(cpp_lookup(cases[i].input,cases[i].attribute,cases[i].eof));if(!conv(got,expected,rs)){std::println("case {} mismatch",i);return 2;}}
  std::println("Named HTML entity trie: C++ and typed λΠ agree on {} finite cases over {} selected CSV rows",cases.size(),entities.size());
  std::println("Scope: selected CSV-backed names cover longest prefix, attribute guard, two-codepoint output, and incomplete-prefix EOF rollback; full-trie normalization stalls, and stream rewind is not modeled.");
}
