#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace lp;

enum class ImageKind { Fetched, Generated, Pending, ImageSet, Crossfade, MaskSource };
struct ImageRelations {
  bool css_value_equivalent;
  bool fetched_resource_same, fetched_url_data_equal, fetched_resolution_equal;
  bool generated_container_sizes_equal, generated_viewport_size_equal;
  bool generated_value_equivalent;
  bool image_set_best_fit_equivalent, image_set_css_value_equal;
  bool crossfade_original_value_same;
  bool mask_image_equivalent, mask_resource_same;
};
static bool cpp_style_image_equal(ImageKind a,ImageKind b,const ImageRelations& r){
  switch(a){
    case ImageKind::Fetched:
      if(b==ImageKind::Pending)return r.css_value_equivalent;
      return b==ImageKind::Fetched&&r.fetched_resource_same&&
             r.fetched_url_data_equal&&r.fetched_resolution_equal;
    case ImageKind::Generated:
      return b==ImageKind::Generated&&r.generated_container_sizes_equal&&
             r.generated_viewport_size_equal&&r.generated_value_equivalent;
    case ImageKind::Pending:
      return r.css_value_equivalent;
    case ImageKind::ImageSet:
      return b==ImageKind::ImageSet&&r.image_set_best_fit_equivalent&&
             r.image_set_css_value_equal;
    case ImageKind::Crossfade:
      return b==ImageKind::Crossfade&&r.crossfade_original_value_same;
    case ImageKind::MaskSource:
      if(b==ImageKind::Pending)return r.css_value_equivalent;
      return b==ImageKind::MaskSource&&r.mask_image_equivalent&&r.mask_resource_same;
  }
  return false;
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule R(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr ap(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr bt(bool b){return sym(b?"true":"false");}
static Ptr image_term(ImageKind kind){const char* names[]={"fetched","generated","pending","imageSet","crossfade","maskSource"};return sym(names[static_cast<unsigned>(kind)]);}
static Ptr normalize(Ptr term,const RuleIndex& ix){for(unsigned i=0;i<16;++i)term=nf(term,ix);return term;}
static Ptr and2(Ptr a,Ptr b){return ap(sym("andB"),{std::move(a),std::move(b)});}
int main(){
  Signature sig;std::vector<Rule> rules;Ptr B=sym("Bool"),I=sym("StyleImageKind");
  sig["Bool"]=type_sort();sig["true"]=B;sig["false"]=B;sig["notB"]=fn(B,B);sig["andB"]=fn(B,fn(B,B));
  sig["StyleImageKind"]=type_sort();
  const char* names[]={"fetched","generated","pending","imageSet","crossfade","maskSource"};
  for(const char* n:names)sig[n]=I;
  Ptr image_type=B;for(unsigned i=0;i<12;++i)image_type=fn(B,image_type);
  sig["styleImageEqual"]=fn(I,fn(I,image_type));
  sig["contentImageNeedsReattach"]=fn(I,fn(I,image_type));
  rules.push_back(R(app(sym("notB"),sym("true")),sym("false")));
  rules.push_back(R(app(sym("notB"),sym("false")),sym("true")));
  for(bool a:{false,true})for(bool b:{false,true})rules.push_back(R(and2(bt(a),bt(b)),bt(a&&b)));

  auto add_rule=[&](ImageKind left,ImageKind right,Ptr result){
    std::vector<Ptr> args={image_term(left),image_term(right)};
    for(unsigned i=0;i<12;++i)args.push_back(pvar(i));
    rules.push_back(R(ap(sym("styleImageEqual"),args),std::move(result)));
  };
  Ptr p[12];for(unsigned i=0;i<12;++i)p[i]=pvar(i);
  add_rule(ImageKind::Fetched,ImageKind::Pending,p[0]);
  add_rule(ImageKind::Fetched,ImageKind::Fetched,and2(and2(p[1],p[2]),p[3]));
  add_rule(ImageKind::Generated,ImageKind::Generated,and2(and2(p[4],p[5]),p[6]));
  for(ImageKind other:{ImageKind::Fetched,ImageKind::Generated,ImageKind::Pending,ImageKind::ImageSet,ImageKind::Crossfade,ImageKind::MaskSource})
    add_rule(ImageKind::Pending,other,p[0]);
  add_rule(ImageKind::ImageSet,ImageKind::ImageSet,and2(p[7],p[8]));
  add_rule(ImageKind::Crossfade,ImageKind::Crossfade,p[9]);
  add_rule(ImageKind::MaskSource,ImageKind::Pending,p[0]);
  add_rule(ImageKind::MaskSource,ImageKind::MaskSource,and2(p[10],p[11]));
  std::vector<Ptr> fallback={pvar(20),pvar(21)};for(unsigned i=0;i<12;++i)fallback.push_back(pvar(i));
  rules.push_back(R(ap(sym("styleImageEqual"),fallback),sym("false")));
  std::vector<Ptr> content_pattern={pvar(20),pvar(21)};for(unsigned i=0;i<12;++i)content_pattern.push_back(pvar(i));
  rules.push_back(R(ap(sym("contentImageNeedsReattach"),content_pattern),app(sym("notB"),ap(sym("styleImageEqual"),content_pattern))));

  RuleIndex ix(rules);
  std::vector<Ptr> sample={image_term(ImageKind::Fetched),image_term(ImageKind::Pending)};for(unsigned i=0;i<12;++i)sample.push_back(sym("false"));
  auto eq_type=infer({},ap(sym("styleImageEqual"),sample),sig,rules);assert(eq_type&&conv(*eq_type,B,ix));
  auto reattach_type=infer({},ap(sym("contentImageNeedsReattach"),sample),sig,rules);assert(reattach_type&&conv(*reattach_type,B,ix));

  unsigned comparisons=0,reattach_comparisons=0;
  for(unsigned left=0;left<6;++left)for(unsigned right=0;right<6;++right)for(unsigned mask=0;mask<(1u<<12);++mask){
    bool b[12];std::vector<Ptr> args={image_term(static_cast<ImageKind>(left)),image_term(static_cast<ImageKind>(right))};
    for(unsigned i=0;i<12;++i){b[i]=(mask&(1u<<i))!=0;args.push_back(bt(b[i]));}
    ImageRelations rel{b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],b[10],b[11]};
    bool expected=cpp_style_image_equal(static_cast<ImageKind>(left),static_cast<ImageKind>(right),rel);
    if(!conv(normalize(ap(sym("styleImageEqual"),args),ix),bt(expected),ix)){std::cerr<<"StyleImage equality mismatch left="<<left<<" right="<<right<<" flags="<<mask<<"\n";return 2;}++comparisons;
    if(!conv(normalize(ap(sym("contentImageNeedsReattach"),args),ix),bt(!expected),ix)){std::cerr<<"image content reattach mismatch\n";return 3;}++reattach_comparisons;
  }
  std::cout<<"StyleImage subclass equality agrees on "<<comparisons<<" kind/relation states.\n";
  std::cout<<"ImageContentData inequality reattach trigger agrees on "<<reattach_comparisons<<" corresponding states.\n";
  std::cout<<"Scope: subclass field relations are abstract booleans; detailed CSSValue, resource, URL, container-size, and image-set equality are not evaluated. Pending-image comparisons preserve the pinned cross-subclass behavior.\n";
}
