#include "../src/lambda_pi.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace lp;

enum class Scalar { Zero, One, Two, OneOver96, TwoOver96, DpcmOne,
                    DpcmTwo, NaN, PositiveInfinity, NegativeInfinity,
                    MaxDouble, MinDouble };
enum class Unit { X, Dppx, Dpi, Dpcm };
struct ResolutionInput { bool calculated; Scalar value; Unit unit; };

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }
static Ptr app2(const char* f, Ptr a, Ptr b) { return app(app(sym(f),a),b); }
static const char* scalar_name(Scalar s) {
  switch(s) {
    case Scalar::Zero:return "zero"; case Scalar::One:return "one";
    case Scalar::Two:return "two"; case Scalar::OneOver96:return "oneOver96";
    case Scalar::TwoOver96:return "twoOver96"; case Scalar::DpcmOne:return "dpcmOne";
    case Scalar::DpcmTwo:return "dpcmTwo"; case Scalar::NaN:return "nan";
    case Scalar::PositiveInfinity:return "positiveInfinity";
    case Scalar::NegativeInfinity:return "negativeInfinity";
    case Scalar::MaxDouble:return "maxDouble"; case Scalar::MinDouble:return "minDouble";
  }
  return "zero";
}
static Ptr scalar_term(Scalar s) { return sym(scalar_name(s)); }
static Ptr unit_term(Unit u) {
  switch(u) { case Unit::X:return sym("unitX"); case Unit::Dppx:return sym("unitDppx");
    case Unit::Dpi:return sym("unitDpi"); case Unit::Dpcm:return sym("unitDpcm"); }
  return sym("unitX");
}
static Ptr input_term(ResolutionInput in) {
  return in.calculated ? app(sym("calculatedResolution"),scalar_term(in.value))
                       : app2("literalResolution",scalar_term(in.value),unit_term(in.unit));
}
static Scalar clamp(Scalar s) {
  if(s==Scalar::NaN)return Scalar::Zero;
  if(s==Scalar::PositiveInfinity)return Scalar::MaxDouble;
  if(s==Scalar::NegativeInfinity)return Scalar::MinDouble;
  return s;
}
static Scalar cpp_compute(ResolutionInput in) {
  if(in.calculated)return clamp(in.value);
  if(in.value==Scalar::NaN)return Scalar::Zero;
  if(in.value==Scalar::PositiveInfinity)return Scalar::MaxDouble;
  if(in.value==Scalar::NegativeInfinity)return Scalar::MinDouble;
  if(in.value==Scalar::Zero)return Scalar::Zero;
  if(in.unit==Unit::X||in.unit==Unit::Dppx)return in.value;
  if(in.unit==Unit::Dpi)return in.value==Scalar::One?Scalar::OneOver96:Scalar::TwoOver96;
  return in.value==Scalar::One?Scalar::DpcmOne:Scalar::DpcmTwo;
}

int main() {
  const Ptr S=sym("Scalar"), U=sym("ResolutionUnit"), V=sym("ResolutionValue");
  Signature sig; std::vector<Rule> rules;
  sig["Scalar"]=type_sort();
  for(Scalar s:{Scalar::Zero,Scalar::One,Scalar::Two,Scalar::OneOver96,Scalar::TwoOver96,
      Scalar::DpcmOne,Scalar::DpcmTwo,Scalar::NaN,Scalar::PositiveInfinity,
      Scalar::NegativeInfinity,Scalar::MaxDouble,Scalar::MinDouble}) sig[scalar_name(s)]=S;
  sig["ResolutionUnit"]=type_sort();
  for(const char* u:{"unitX","unitDppx","unitDpi","unitDpcm"})sig[u]=U;
  sig["ResolutionValue"]=type_sort();
  sig["literalResolution"]=fn(S,fn(U,V)); sig["calculatedResolution"]=fn(S,V);
  sig["computeDotsPerPixel"]=fn(V,S); sig["clampDouble"]=fn(S,S);
  for(Scalar s:{Scalar::Zero,Scalar::One,Scalar::Two,Scalar::OneOver96,Scalar::TwoOver96,
      Scalar::DpcmOne,Scalar::DpcmTwo,Scalar::NaN,Scalar::PositiveInfinity,
      Scalar::NegativeInfinity,Scalar::MaxDouble,Scalar::MinDouble}) {
    Scalar out=s==Scalar::NaN?Scalar::Zero:s==Scalar::PositiveInfinity?Scalar::MaxDouble:
        s==Scalar::NegativeInfinity?Scalar::MinDouble:s;
    rules.push_back(rule(app(sym("clampDouble"),scalar_term(s)),scalar_term(out)));
  }
  rules.push_back(rule(app(sym("computeDotsPerPixel"),app(sym("calculatedResolution"),pvar(0))),
      app(sym("clampDouble"),pvar(0))));
  for(Scalar value:{Scalar::Zero,Scalar::One,Scalar::Two,Scalar::NaN,
      Scalar::PositiveInfinity,Scalar::NegativeInfinity}) for(Unit unit:{Unit::X,Unit::Dppx,Unit::Dpi,Unit::Dpcm}) {
    Scalar product=value;
    if(value==Scalar::NaN)product=Scalar::NaN;
    else if(value==Scalar::Zero)product=Scalar::Zero;
    else if(value==Scalar::PositiveInfinity)product=Scalar::PositiveInfinity;
    else if(value==Scalar::NegativeInfinity)product=Scalar::NegativeInfinity;
    else if(unit==Unit::Dpi)product=value==Scalar::One?Scalar::OneOver96:Scalar::TwoOver96;
    else if(unit==Unit::Dpcm)product=value==Scalar::One?Scalar::DpcmOne:Scalar::DpcmTwo;
    rules.push_back(rule(app(sym("computeDotsPerPixel"),app2("literalResolution",scalar_term(value),unit_term(unit))),
        app(sym("clampDouble"),scalar_term(product))));
  }
  RuleIndex index(rules);
  std::vector<ResolutionInput> inputs;
  for(Scalar s:{Scalar::Zero,Scalar::One,Scalar::Two,Scalar::NaN,
      Scalar::PositiveInfinity,Scalar::NegativeInfinity}) for(Unit u:{Unit::X,Unit::Dppx,Unit::Dpi,Unit::Dpcm}) inputs.push_back({false,s,u});
  for(Scalar s:{Scalar::Zero,Scalar::One,Scalar::NaN,Scalar::PositiveInfinity,Scalar::NegativeInfinity})inputs.push_back({true,s,Unit::X});
  for(const auto& in:inputs) {
    Ptr term=app(sym("computeDotsPerPixel"),input_term(in));
    Ptr wanted=scalar_term(cpp_compute(in));
    if(!conv(nf(term,index),wanted,index)) { std::cerr<<"ComputedResolution projection mismatch\n"; return 2; }
  }
  std::cout<<"CSS image-set computed-resolution dispatch and ClampDouble projection cover "<<inputs.size()<<" finite observations.\n";
  std::cout<<"Calculated math evaluation and unit multiplication are finite observations; this is not a CSS math evaluator.\n";
  return 0;
}
