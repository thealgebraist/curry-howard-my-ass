#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>

// CSS-wide defaulting projection for Blink's inherited `color` longhand.
// Values are abstract color IDs; zero is the configured initial black value.
using namespace lp;
enum class Keyword { Missing, Inherit, Initial, Unset };
static unsigned cpp_color_default(Keyword keyword, bool has_parent,
                                  unsigned parent_color,
                                  unsigned initial_color) {
  switch (keyword) {
    case Keyword::Initial: return initial_color;
    case Keyword::Missing:
      return has_parent ? parent_color : initial_color;
    case Keyword::Inherit:
    case Keyword::Unset:
      // color is inherited; at the root, StyleBuilder converts inherit/unset
      // to initial before applying the longhand.
      return has_parent ? parent_color : initial_color;
  }
  return initial_color;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr t=sym("z");while(n--)t=app(sym("s"),t);return t;}
static unsigned nat_value(Ptr t){unsigned n=0;while(auto a=std::get_if<App>(&t->v)){auto h=std::get_if<Sym>(&a->fn->v);if(!h||h->name!="s")break;++n;t=a->arg;}assert(std::get_if<Sym>(&t->v)&&std::get<Sym>(t->v).name=="z");return n;}
static Ptr ap(Ptr f,std::initializer_list<Ptr> xs){for(const auto& x:xs)f=app(f,x);return f;}

int main(){
  Signature sig;std::vector<Rule> rules;auto p=[](size_t i){return pvar(i);};
  auto value=sym("ColorValue"), boolean=sym("Bool"), keyword=sym("Keyword");
  sig["ColorValue"]=type_sort();sig["z"]=value;sig["s"]=fn(value,value);
  sig["Bool"]=type_sort();sig["yes"]=boolean;sig["no"]=boolean;
  sig["Keyword"]=type_sort();sig["missing"]=keyword;sig["inheritK"]=keyword;
  sig["initialK"]=keyword;sig["unsetK"]=keyword;
  sig["colorDefault"]=fn(keyword,fn(boolean,fn(value,fn(value,value))));
  sig["parentOrInitial"]=fn(boolean,fn(value,fn(value,value)));
  rules.push_back(rule(ap(sym("parentOrInitial"),{sym("yes"),p(0),p(1)}),p(0)));
  rules.push_back(rule(ap(sym("parentOrInitial"),{sym("no"),p(0),p(1)}),p(1)));
  rules.push_back(rule(ap(sym("colorDefault"),{sym("initialK"),p(0),p(1),p(2)}),p(2)));
  rules.push_back(rule(ap(sym("colorDefault"),{sym("missing"),p(0),p(1),p(2)}),ap(sym("parentOrInitial"),{p(0),p(1),p(2)})));
  rules.push_back(rule(ap(sym("colorDefault"),{sym("inheritK"),p(0),p(1),p(2)}),ap(sym("parentOrInitial"),{p(0),p(1),p(2)})));
  rules.push_back(rule(ap(sym("colorDefault"),{sym("unsetK"),p(0),p(1),p(2)}),ap(sym("parentOrInitial"),{p(0),p(1),p(2)})));

  const Keyword keywords[]={Keyword::Missing,Keyword::Inherit,Keyword::Initial,Keyword::Unset};
  const char* names[]={"missing","inheritK","initialK","unsetK"};
  unsigned count=0;
  for(bool has_parent:{false,true})for(unsigned parent=0;parent<3;++parent)for(size_t k=0;k<4;++k){
    Ptr term=ap(sym("colorDefault"),{sym(names[k]),sym(has_parent?"yes":"no"),nat(parent),nat(0)});
    auto ty=infer({},term,sig,rules);assert(ty&&conv(*ty,value,rules));
    const unsigned actual=nat_value(nf(term,rules));
    const unsigned expected=cpp_color_default(keywords[k],has_parent,parent,0);
    assert(actual==expected);++count;
  }
  std::cout<<"Blink color defaulting projection agrees on "<<count<<" finite inputs\n";
  std::cout<<"Scope: inherited color only; initial value black (0); parent and computed colors are abstract IDs.\n";
}
