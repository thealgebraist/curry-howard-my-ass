#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <iostream>
#include <optional>
#include <vector>

using namespace lp;

enum class Kind { CSSImage, Gradient, Unsupported };
struct Selected { bool present=false; Kind kind=Kind::Unsupported; unsigned image=0, resolution=0; };
struct Payload { enum Type { None, Image, Gradient } type=None; unsigned image=0,resolution=0; };
struct Wrapper { Payload payload; };
struct Entry { unsigned scale; Wrapper wrapper; };
enum class Mode { Shared, FetcherKeyed };
struct State { Mode mode; std::optional<Entry> shared; std::array<std::optional<Entry>,2> keyed; };
struct Result { Wrapper returned; State state; };
static Payload cpp_load(const Selected& selected){
  if(!selected.present||selected.kind==Kind::Unsupported)return {};
  if(selected.kind==Kind::CSSImage)return {Payload::Image,selected.image,selected.resolution};
  return {Payload::Gradient,selected.image,0};
}
static Result cpp_pipeline(State state,unsigned fetcher,unsigned scale,const Selected& selected){
  Wrapper wrapper{cpp_load(selected)};Entry entry{scale,wrapper};
  if(state.mode==Mode::Shared)state.shared=entry;else state.keyed[fetcher]=entry;
  return {wrapper,state};
}
static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr apps(Ptr f,const std::vector<Ptr>& xs){for(const auto& x:xs)f=app(f,x);return f;}
static Ptr scale_term(unsigned s){return sym(s==0?"scaleOne":"scaleTwo");}
static Ptr fetcher_term(unsigned f){return sym(f==0?"fetcherA":"fetcherB");}
static Ptr image_term(unsigned i){return sym(i==0?"imageA":"imageB");}
static Ptr payload_term(const Payload& p){switch(p.type){case Payload::None:return sym("noLoadedStyleImage");case Payload::Image:return apps(sym("loadedCSSImage"),{image_term(p.image),scale_term(p.resolution)});case Payload::Gradient:return app(sym("generatedGradient"),image_term(p.image));}return sym("noLoadedStyleImage");}
static Ptr wrapper_term(const Wrapper& w){return app(sym("styleImageSetWrapper"),payload_term(w.payload));}
static Ptr slot_term(const std::optional<Entry>& e){return e?apps(sym("cacheEntry"),{scale_term(e->scale),wrapper_term(e->wrapper)}):sym("emptySlot");}
static Ptr state_term(const State& s){if(s.mode==Mode::Shared)return s.shared?apps(sym("sharedEntry"),{scale_term(s.shared->scale),wrapper_term(s.shared->wrapper)}):sym("sharedEmpty");return apps(sym("fetcherCache"),{slot_term(s.keyed[0]),slot_term(s.keyed[1])});}
static Ptr selected_term(const Selected& s){if(!s.present)return sym("noSelectedOption");const char* k=s.kind==Kind::CSSImage?"cssImageOption":s.kind==Kind::Gradient?"gradientOption":"unsupportedOption";return apps(sym("selectedOption"),{sym(k),image_term(s.image),scale_term(s.resolution)});}
static Ptr mode_term(Mode m){return sym(m==Mode::Shared?"sharedMode":"fetcherKeyedMode");}
static Ptr query(const State& state,unsigned fetcher,unsigned scale,const Selected& selected){return apps(sym("loadAndCacheImageSet"),{mode_term(state.mode),state_term(state),fetcher_term(fetcher),scale_term(scale),selected_term(selected)});}
static Ptr expected(const Result& r){return apps(sym("imageSetCachedAndReturned"),{wrapper_term(r.returned),state_term(r.state)});}

int main(){
  const Ptr I=sym("ImageId"),R=sym("Resolution"),F=sym("Fetcher"),M=sym("CacheMode"),P=sym("StyleImagePayload"),W=sym("WrappedStyleImage"),S=sym("CacheSlot"),C=sym("CacheState"),O=sym("SelectedOption"),V=sym("ImageSetLoadCacheResult");Signature sig;std::vector<Rule> rules;
  sig["ImageId"]=type_sort();sig["imageA"]=I;sig["imageB"]=I;
  sig["Resolution"]=type_sort();sig["scaleOne"]=R;sig["scaleTwo"]=R;
  sig["Fetcher"]=type_sort();sig["fetcherA"]=F;sig["fetcherB"]=F;
  sig["CacheMode"]=type_sort();sig["sharedMode"]=M;sig["fetcherKeyedMode"]=M;
  sig["StyleImagePayload"]=type_sort();sig["noLoadedStyleImage"]=P;sig["loadedCSSImage"]=fn(I,fn(R,P));sig["generatedGradient"]=fn(I,P);
  sig["WrappedStyleImage"]=type_sort();sig["styleImageSetWrapper"]=fn(P,W);
  sig["CacheSlot"]=type_sort();sig["emptySlot"]=S;sig["cacheEntry"]=fn(R,fn(W,S));
  sig["CacheState"]=type_sort();sig["sharedEmpty"]=C;sig["sharedEntry"]=fn(R,fn(W,C));sig["fetcherCache"]=fn(S,fn(S,C));
  sig["SelectedOption"]=type_sort();sig["noSelectedOption"]=O;sig["cssImageOption"]=sym("ImageOptionKind");sig["gradientOption"]=sym("ImageOptionKind");sig["unsupportedOption"]=sym("ImageOptionKind");sig["selectedOption"]=fn(sym("ImageOptionKind"),fn(I,fn(R,O)));
  sig["ImageOptionKind"]=type_sort();
  sig["ImageSetLoadCacheResult"]=type_sort();sig["imageSetCachedAndReturned"]=fn(W,fn(C,V));
  sig["loadSelectedImage"]=fn(O,P);sig["wrapImageSet"]=fn(P,W);sig["cacheLoadedImageSet"]=fn(M,fn(C,fn(F,fn(R,fn(W,C)))));sig["loadAndCacheImageSet"]=fn(M,fn(C,fn(F,fn(R,fn(O,V)))));
  rules.push_back(rule(app(sym("loadSelectedImage"),sym("noSelectedOption")),sym("noLoadedStyleImage")));
  rules.push_back(rule(apps(sym("loadSelectedImage"),{apps(sym("selectedOption"),{sym("cssImageOption"),pvar(0),pvar(1)})}),apps(sym("loadedCSSImage"),{pvar(0),pvar(1)})));
  rules.push_back(rule(apps(sym("loadSelectedImage"),{apps(sym("selectedOption"),{sym("gradientOption"),pvar(2),pvar(3)})}),app(sym("generatedGradient"),pvar(2))));
  rules.push_back(rule(apps(sym("loadSelectedImage"),{apps(sym("selectedOption"),{sym("unsupportedOption"),pvar(4),pvar(5)})}),sym("noLoadedStyleImage")));
  rules.push_back(rule(app(sym("wrapImageSet"),pvar(6)),app(sym("styleImageSetWrapper"),pvar(6))));
  rules.push_back(rule(apps(sym("cacheLoadedImageSet"),{sym("sharedMode"),pvar(7),pvar(8),pvar(9),pvar(10)}),apps(sym("sharedEntry"),{pvar(9),pvar(10)})));
  rules.push_back(rule(apps(sym("cacheLoadedImageSet"),{sym("fetcherKeyedMode"),apps(sym("fetcherCache"),{pvar(11),pvar(12)}),sym("fetcherA"),pvar(13),pvar(14)}),apps(sym("fetcherCache"),{apps(sym("cacheEntry"),{pvar(13),pvar(14)}),pvar(12)})));
  rules.push_back(rule(apps(sym("cacheLoadedImageSet"),{sym("fetcherKeyedMode"),apps(sym("fetcherCache"),{pvar(15),pvar(16)}),sym("fetcherB"),pvar(17),pvar(18)}),apps(sym("fetcherCache"),{pvar(15),apps(sym("cacheEntry"),{pvar(17),pvar(18)})})));
  rules.push_back(rule(apps(sym("loadAndCacheImageSet"),{pvar(19),pvar(20),pvar(21),pvar(22),pvar(23)}),apps(sym("imageSetCachedAndReturned"),{app(sym("wrapImageSet"),app(sym("loadSelectedImage"),pvar(23))),apps(sym("cacheLoadedImageSet"),{pvar(19),pvar(20),pvar(21),pvar(22),app(sym("wrapImageSet"),app(sym("loadSelectedImage"),pvar(23)))})})));
  RuleIndex index(rules);Selected sample{true,Kind::CSSImage,0,0};State init{Mode::Shared,std::nullopt,{}};auto inferred=infer({},query(init,0,0,sample),sig,rules);assert(inferred&&conv(*inferred,V,index));
  std::vector<Selected> options={{false},{true,Kind::Unsupported,0,0},{true,Kind::CSSImage,0,0},{true,Kind::CSSImage,1,1},{true,Kind::Gradient,0,1},{true,Kind::Gradient,1,0}};
  std::vector<State> states={{Mode::Shared,std::nullopt,{}},{Mode::Shared,Entry{0,Wrapper{{Payload::Image,1,1}}},{}},{Mode::FetcherKeyed,std::nullopt,{std::nullopt,std::nullopt}},{Mode::FetcherKeyed,std::nullopt,{Entry{0,Wrapper{{Payload::Gradient,0,0}}},std::nullopt}},{Mode::FetcherKeyed,std::nullopt,{Entry{1,Wrapper{{Payload::Image,1,0}}},Entry{0,Wrapper{{Payload::Image,0,1}}}}}};
  unsigned cases=0;for(const auto& st:states)for(unsigned f=0;f<2;++f)for(unsigned scale=0;scale<2;++scale)for(const auto& opt:options){auto term=query(st,f,scale,opt);auto want=expected(cpp_pipeline(st,f,scale,opt));if(!conv(nf(term,index),want,index)){std::cerr<<"Image-set load/cache composition mismatch\n";return 2;}++cases;}
  std::cout<<"CSS image-set load plus cache update covers "<<cases<<" finite state transitions.\n";
  std::cout<<"Scope: selected-option dispatch, StyleImageSet wrapping (including null inner images), and shared/fetcher-keyed CacheImage replacement are composed. GetBestOption selection and actual image resource/generator loading are supplied observations.\n";
}
