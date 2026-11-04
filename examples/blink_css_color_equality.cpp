#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <iostream>
#include <vector>

using namespace lp;

struct ColorValue {
  unsigned color_space;
  std::array<bool,4> none_flags;
  std::array<float,4> components;
};

static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr lhs,Ptr rhs){return{std::move(lhs),std::move(rhs)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bt(bool value){return sym(value?"true":"false");}
static Ptr normalize(Ptr term,const RuleIndex& rules){for(unsigned i=0;i<12;++i)term=nf(term,rules);return term;}

static Ptr color_term(const ColorValue& color){
  std::vector<Ptr> fields={sym(color.color_space==0?"spaceSRGBLegacy":"spaceDisplayP3")};
  for(bool flag:color.none_flags)fields.push_back(bt(flag));
  for(float component:color.components)fields.push_back(sym(component==0.f?"component0":"component1"));
  return apps(sym("cssColor"),fields);
}

int main(){
  const Ptr B=sym("Bool"),S=sym("ColorSpace"),F=sym("ColorComponent"),C=sym("CSSColorValue");
  Signature signature;std::vector<Rule> rules;
  signature["Bool"]=type_sort();signature["true"]=B;signature["false"]=B;
  signature["ColorSpace"]=type_sort();signature["spaceSRGBLegacy"]=S;signature["spaceDisplayP3"]=S;
  signature["ColorComponent"]=type_sort();signature["component0"]=F;signature["component1"]=F;
  signature["CSSColorValue"]=type_sort();
  Ptr color_constructor=C;
  for(unsigned i=0;i<4;++i)color_constructor=fn(F,color_constructor);
  for(unsigned i=0;i<4;++i)color_constructor=fn(B,color_constructor);
  color_constructor=fn(S,color_constructor);
  signature["cssColor"]=color_constructor;
  signature["andB"]=fn(B,fn(B,B));
  signature["boolEqual"]=fn(B,fn(B,B));
  signature["colorSpaceEqual"]=fn(S,fn(S,B));
  signature["componentEqual"]=fn(F,fn(F,B));
  signature["colorValueEqual"]=fn(C,fn(C,B));
  signature["cssColorEquals"]=fn(C,fn(C,B));
  for(bool a:{false,true})for(bool b:{false,true})
    rules.push_back(rule(apps(sym("andB"),{bt(a),bt(b)}),bt(a&&b)));
  for(const char* value:{"true","false"})
    rules.push_back(rule(apps(sym("boolEqual"),{sym(value),sym(value)}),sym("true")));
  rules.push_back(rule(apps(sym("boolEqual"),{pvar(0),pvar(1)}),sym("false")));
  for(const char* value:{"spaceSRGBLegacy","spaceDisplayP3"})
    rules.push_back(rule(apps(sym("colorSpaceEqual"),{sym(value),sym(value)}),sym("true")));
  rules.push_back(rule(apps(sym("colorSpaceEqual"),{pvar(2),pvar(3)}),sym("false")));
  for(const char* value:{"component0","component1"})
    rules.push_back(rule(apps(sym("componentEqual"),{sym(value),sym(value)}),sym("true")));
  rules.push_back(rule(apps(sym("componentEqual"),{pvar(4),pvar(5)}),sym("false")));

  std::vector<Ptr> left_fields={pvar(10)};
  std::vector<Ptr> right_fields={pvar(20)};
  std::vector<Ptr> equalities={apps(sym("colorSpaceEqual"),{pvar(10),pvar(20)})};
  for(unsigned i=0;i<4;++i){
    left_fields.push_back(pvar(11+i));right_fields.push_back(pvar(21+i));
    equalities.push_back(apps(sym("boolEqual"),{pvar(11+i),pvar(21+i)}));
  }
  for(unsigned i=0;i<4;++i){
    left_fields.push_back(pvar(15+i));right_fields.push_back(pvar(25+i));
    equalities.push_back(apps(sym("componentEqual"),{pvar(15+i),pvar(25+i)}));
  }
  Ptr conjunction=equalities.back();
  for(int i=static_cast<int>(equalities.size())-2;i>=0;--i)
    conjunction=apps(sym("andB"),{equalities[i],conjunction});
  rules.push_back(rule(apps(sym("colorValueEqual"),
    {apps(sym("cssColor"),left_fields),apps(sym("cssColor"),right_fields)}),conjunction));
  rules.push_back(rule(apps(sym("cssColorEquals"),{pvar(30),pvar(31)}),
                       apps(sym("colorValueEqual"),{pvar(30),pvar(31)})));

  RuleIndex indexed(rules);
  auto sample=infer({},apps(sym("cssColorEquals"),
    {color_term({0,{false,false,false,false},{0.f,0.f,0.f,0.f}}),
     color_term({0,{false,false,false,false},{0.f,0.f,0.f,0.f}})}),signature,rules);
  assert(sample&&conv(*sample,B,indexed));

  std::vector<ColorValue> colors;
  for(unsigned space=0;space<2;++space)
    for(unsigned flags=0;flags<16;++flags)
      for(unsigned components=0;components<16;++components){
        ColorValue value{};value.color_space=space;
        for(unsigned i=0;i<4;++i){
          value.none_flags[i]=((flags>>i)&1u)!=0;
          value.components[i]=((components>>i)&1u)?1.f:0.f;
        }
        colors.push_back(value);
      }

  unsigned comparisons=0;
  for(const auto& left:colors)for(const auto& right:colors){
    const bool expected=left.color_space==right.color_space&&
      left.none_flags==right.none_flags&&left.components==right.components;
    Ptr term=apps(sym("cssColorEquals"),{color_term(left),color_term(right)});
    if(!conv(normalize(term,indexed),bt(expected),indexed)){
      std::cerr<<"CSSColor equality mismatch\n";return 2;
    }
    ++comparisons;
  }
  std::cout<<"CSSColor::Equals and Color::operator== compare "<<comparisons
           <<" finite structural color pairs.\n";
  std::cout<<"Domain: two color spaces, four Boolean none-flags, and four exact components in {0,1}.\n";
}
