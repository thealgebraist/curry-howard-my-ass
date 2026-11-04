#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
using namespace lp;

enum class ContentKind { Text, AltText, Quote, None, Image, Counter, AltCounter };
struct ContentNode {
  ContentKind kind;
  unsigned value[6]{};
  std::string constructor;
};

static std::vector<ContentNode> make_nodes() {
  std::vector<ContentNode> nodes;
  auto add=[&](ContentKind kind,std::string name,std::initializer_list<unsigned> values){
    ContentNode node{kind,{},std::move(name)};unsigned i=0;for(unsigned value:values)node.value[i++]=value;nodes.push_back(std::move(node));
  };
  for(unsigned i=0;i<2;++i)add(ContentKind::Text,"text"+std::to_string(i),{i});
  for(unsigned i=0;i<2;++i)add(ContentKind::AltText,"altText"+std::to_string(i),{i});
  for(unsigned i=0;i<4;++i)add(ContentKind::Quote,"quote"+std::to_string(i),{i});
  add(ContentKind::None,"noneContent",{});
  for(unsigned i=0;i<2;++i)add(ContentKind::Image,"image"+std::to_string(i),{i});
  for(unsigned key=0;key<32;++key){unsigned bits[5];for(unsigned j=0;j<5;++j)bits[j]=(key>>j)&1;add(ContentKind::Counter,"counter"+std::to_string(key),{bits[0],bits[1],bits[2],bits[3],bits[4]});}
  for(unsigned key=0;key<32;++key){unsigned bits[5];for(unsigned j=0;j<5;++j)bits[j]=(key>>j)&1;for(unsigned text=0;text<2;++text)add(ContentKind::AltCounter,"altCounter"+std::to_string(key*2+text),{bits[0],bits[1],bits[2],bits[3],bits[4],text});}
  return nodes;
}

// Directed equality follows ContentData::Equals: base CounterContentData
// accepts AltCounterContentData, while AltCounterContentData requires that the
// other node is also an AltCounterContentData.
static bool cpp_node_equals(const ContentNode& a,const ContentNode& b) {
  if(a.kind==ContentKind::Counter&&b.kind==ContentKind::AltCounter){
    for(unsigned i=0;i<5;++i)if(a.value[i]!=b.value[i])return false;
    return true;
  }
  if(a.kind!=b.kind)return false;
  switch(a.kind){
    case ContentKind::Text:case ContentKind::AltText:return a.value[0]==b.value[0];
    case ContentKind::Quote:return a.value[0]==b.value[0];
    case ContentKind::None:return true;
    case ContentKind::Image:return a.value[0]==b.value[0]; // StyleImage equality is an abstract atom.
    case ContentKind::Counter:for(unsigned i=0;i<5;++i)if(a.value[i]!=b.value[i])return false;return true;
    case ContentKind::AltCounter:for(unsigned i=0;i<6;++i)if(a.value[i]!=b.value[i])return false;return true;
  }
  return false;
}
static bool cpp_content_equals(const std::vector<ContentNode>& a,const std::vector<ContentNode>& b){
  size_t i=0;while(i<a.size()&&i<b.size()&&cpp_node_equals(a[i],b[i]))++i;return i==a.size()&&i==b.size();
}

static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr ap(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr cons(const std::string& node,Ptr tail){return ap(sym("cons"),{sym(node),std::move(tail)});}
static Ptr list_term(const std::vector<ContentNode>& nodes,size_t start=0){Ptr result=sym("nil");for(size_t i=nodes.size();i>start;--i)result=cons(nodes[i-1].constructor,result);return result;}
static Ptr bt(bool b){return sym(b?"true":"false");}
static Ptr normalize(Ptr term,const RuleIndex& ix){for(unsigned i=0;i<8;++i)term=nf(term,ix);return term;}

int main(){
  const auto nodes=make_nodes();
  Signature sig;std::vector<Rule> rules;Ptr B=sym("Bool"),N=sym("ContentNode"),L=sym("ContentList");
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;sig["ContentNode"]=type_sort();sig["ContentList"]=type_sort();
  for(const auto& node:nodes)sig[node.constructor]=N;
  sig["nil"]=L;sig["cons"]=fn(N,fn(L,L));sig["andB"]=fn(B,fn(B,B));sig["notB"]=fn(B,B);
  sig["nodeEqual"]=fn(N,fn(N,B));sig["contentListEqual"]=fn(L,fn(L,B));sig["contentChangedRequiresReattach"]=fn(L,fn(L,B));
  for(bool a:{false,true})for(bool b:{false,true})rules.push_back(R(ap(sym("andB"),{bt(a),bt(b)}),bt(a&&b)));
  rules.push_back(R(app(sym("notB"),sym("true")),sym("false")));
  rules.push_back(R(app(sym("notB"),sym("false")),sym("true")));

  for(const auto& node:nodes)rules.push_back(R(ap(sym("nodeEqual"),{sym(node.constructor),sym(node.constructor)}),sym("true")));
  // CounterContentData::Equals accepts an AltCounterContentData after checking
  // the five CounterData fields; the reverse dynamic cast fails.
  for(unsigned key=0;key<32;++key)for(unsigned text=0;text<2;++text)
    rules.push_back(R(ap(sym("nodeEqual"),{sym("counter"+std::to_string(key)),sym("altCounter"+std::to_string(key*2+text))}),sym("true")));
  rules.push_back(R(ap(sym("nodeEqual"),{pvar(0),pvar(1)}),sym("false")));

  rules.push_back(R(ap(sym("contentListEqual"),{sym("nil"),sym("nil")}),sym("true")));
  rules.push_back(R(ap(sym("contentListEqual"),{sym("nil"),ap(sym("cons"),{pvar(0),pvar(1)})}),sym("false")));
  rules.push_back(R(ap(sym("contentListEqual"),{ap(sym("cons"),{pvar(0),pvar(1)}),sym("nil")}),sym("false")));
  rules.push_back(R(ap(sym("contentListEqual"),{ap(sym("cons"),{pvar(0),pvar(1)}),ap(sym("cons"),{pvar(2),pvar(3)})}),ap(sym("andB"),{ap(sym("nodeEqual"),{pvar(0),pvar(2)}),ap(sym("contentListEqual"),{pvar(1),pvar(3)})})));
  rules.push_back(R(ap(sym("contentChangedRequiresReattach"),{pvar(0),pvar(1)}),app(sym("notB"),ap(sym("contentListEqual"),{pvar(0),pvar(1)}))));

  RuleIndex ix(rules);
  Ptr node_sample=ap(sym("nodeEqual"),{sym(nodes.front().constructor),sym(nodes.front().constructor)});
  auto node_type=infer({},node_sample,sig,rules);assert(node_type&&conv(*node_type,B,ix));
  Ptr list_sample=ap(sym("contentListEqual"),{sym("nil"),sym("nil")});
  auto list_type=infer({},list_sample,sig,rules);assert(list_type&&conv(*list_type,B,ix));
  Ptr reattach_sample=ap(sym("contentChangedRequiresReattach"),{sym("nil"),sym("nil")});
  auto reattach_type=infer({},reattach_sample,sig,rules);assert(reattach_type&&conv(*reattach_type,B,ix));

  unsigned node_pairs=0;
  for(const auto& a:nodes)for(const auto& b:nodes){
    Ptr term=ap(sym("nodeEqual"),{sym(a.constructor),sym(b.constructor)});
    if(!conv(nf(term,ix),bt(cpp_node_equals(a,b)),ix)){std::cerr<<"content node equality mismatch: "<<a.constructor<<" / "<<b.constructor<<"\n";return 2;}++node_pairs;
  }
  std::vector<std::vector<ContentNode>> singles(1);
  for(const auto& node:nodes)singles.push_back({node});
  unsigned list_pairs=0,reattach_pairs=0;
  auto compare_lists=[&](const std::vector<ContentNode>& a,const std::vector<ContentNode>& b)->bool{
    Ptr left=list_term(a),right=list_term(b);
    bool equal=cpp_content_equals(a,b);
    if(!conv(normalize(ap(sym("contentListEqual"),{left,right}),ix),bt(equal),ix))return false;
    if(!conv(normalize(ap(sym("contentChangedRequiresReattach"),{left,right}),ix),bt(!equal),ix))return false;
    ++list_pairs;++reattach_pairs;return true;
  };
  for(const auto& a:singles)for(const auto& b:singles)if(!compare_lists(a,b)){std::cerr<<"single-node content list mismatch\n";return 3;}
  // Two-element lists cover head comparison with a shared tail and tail
  // comparison with a shared head. NoneContentData cannot have a Next node.
  for(const auto& a:nodes)if(a.kind!=ContentKind::None)for(const auto& b:nodes)if(b.kind!=ContentKind::None){
    if(!compare_lists({a,nodes.front()},{b,nodes.front()})){std::cerr<<"content list head mismatch\n";return 4;}
    if(!compare_lists({nodes.front(),a},{nodes.front(),b})){std::cerr<<"content list tail mismatch\n";return 5;}
  }
  for(const auto& a:nodes)if(a.kind!=ContentKind::None){
    if(!compare_lists({a,nodes.front()},{a})){std::cerr<<"content list length mismatch\n";return 6;}
    if(!compare_lists({a,nodes.front()},{})){std::cerr<<"content list nil mismatch\n";return 7;}
  }
  std::cout<<"ContentData node equality agrees on "<<node_pairs<<" directed tag/payload pairs.\n";
  std::cout<<"ContentData list equality and content-triggered reattach agree on "<<list_pairs<<" finite list pairs ("<<reattach_pairs<<" reattach decisions).\n";
  std::cout<<"Scope: strings, StyleImage equality, counter-style values, and tree-scope identities are finite atoms; lists are tested through length two.\n";
}
