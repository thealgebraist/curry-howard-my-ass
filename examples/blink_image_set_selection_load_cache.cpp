#define main blink_image_set_selection_standalone_main
#include "blink_image_set_selection.cpp"
#undef main

#include <array>

enum class ValueKind { CSSImage, Gradient, Unsupported };
enum class CacheMode { Shared, FetcherKeyed };
struct CacheState {
  CacheMode mode;
  std::optional<unsigned> shared_image;
  std::array<std::optional<unsigned>, 2> fetcher_images;
};
struct PipelineExpected { int selected; ValueKind kind; int cached_image; };

static ValueKind cpp_value_kind(unsigned image) {
  if (image == 2) return ValueKind::Gradient;
  if (image == 5) return ValueKind::Unsupported;
  return ValueKind::CSSImage;
}
static PipelineExpected cpp_pipeline(const Options& options, unsigned scale,
                                    MimeFeatures features, CacheMode mode,
                                    unsigned fetcher, CacheState state) {
  int selected = cpp_best_option(options, scale, features);
  ValueKind kind = selected < 0 ? ValueKind::Unsupported
                                : cpp_value_kind(static_cast<unsigned>(selected));
  int cached = kind == ValueKind::Unsupported ? -1 : selected;
  if (mode == CacheMode::Shared) state.shared_image = cached < 0 ? std::nullopt : std::optional<unsigned>(cached);
  else state.fetcher_images[fetcher] = cached < 0 ? std::nullopt : std::optional<unsigned>(cached);
  return {selected, kind, cached};
}
static Ptr cache_state_term(CacheMode mode, const CacheState& state) {
  auto slot = [](const std::optional<unsigned>& image) {
    return image ? app(sym("cacheHasImage"), sym(("image" + std::to_string(*image)).c_str()))
                 : sym("cacheEmpty");
  };
  if (mode == CacheMode::Shared)
    return state.shared_image ? app(sym("sharedCache"), slot(state.shared_image)) : sym("sharedEmpty");
  return apps(sym("fetcherCache"), {slot(state.fetcher_images[0]), slot(state.fetcher_images[1])});
}
static Ptr pipeline_term(const Options& options, unsigned scale, MimeFeatures features,
                         CacheMode mode, unsigned fetcher, const CacheState& state) {
  return apps(sym("selectLoadCache"), {options_term(options), scale_term(scale),
      features_term(features), sym(mode == CacheMode::Shared ? "sharedMode" : "fetcherMode"),
      sym(fetcher == 0 ? "fetcherA" : "fetcherB"), cache_state_term(mode, state)});
}
static Ptr option_kind_term(ValueKind k) {
  return sym(k == ValueKind::CSSImage ? "cssImageKind" :
             k == ValueKind::Gradient ? "gradientKind" : "unsupportedKind");
}

int main() {
  Signature sig;
  std::vector<Rule> rules;
  add_selection_rules(sig, rules);
  const Ptr B=sym("Bool"), I=sym("ImageId"), R=sym("Resolution");
  const Ptr O=sym("SelectedOption"), K=sym("ImageOptionKind");
  const Ptr P=sym("StyleImagePayload"), W=sym("WrappedStyleImage");
  const Ptr S=sym("CacheSlot"), C=sym("CacheState"), M=sym("CacheMode"), F=sym("Fetcher");
  const Ptr V=sym("PipelineResult");
  sig["ImageOptionKind"]=type_sort();
  sig["cssImageKind"]=K; sig["gradientKind"]=K; sig["unsupportedKind"]=K;
  sig["SelectedOption"]=type_sort(); sig["noSelectedOption"]=O;
  sig["selectedOption"]=fn(K,fn(I,fn(R,O)));
  sig["imageValueKind"]=fn(I,K); sig["toSelectedOption"]=fn(sym("SelectedOption"),O);
  for(unsigned i=0;i<6;++i) {
    const ValueKind k=cpp_value_kind(i);
    rules.push_back(rule(app(sym("imageValueKind"),sym(("image"+std::to_string(i)).c_str())),option_kind_term(k)));
  }
  rules.push_back(rule(app(sym("toSelectedOption"),sym("noOption")),sym("noSelectedOption")));
  rules.push_back(rule(app(sym("toSelectedOption"),app(sym("someOption"),
      apps(sym("option"),{pvar(100),pvar(101),pvar(102)}))),
      apps(sym("selectedOption"),{app(sym("imageValueKind"),pvar(100)),pvar(100),pvar(101)})));

  sig["StyleImagePayload"]=type_sort(); sig["noLoadedImage"]=P;
  sig["loadedCSSImage"]=fn(I,fn(R,P)); sig["generatedGradient"]=fn(I,P);
  sig["WrappedStyleImage"]=type_sort(); sig["styleImageSetWrapper"]=fn(P,W);
  sig["ImageOptionKind"]=type_sort(); sig["loadSelected"]=fn(O,P);
  sig["wrapImageSet"]=fn(P,W);
  rules.push_back(rule(app(sym("loadSelected"),sym("noSelectedOption")),sym("noLoadedImage")));
  rules.push_back(rule(apps(sym("loadSelected"),{apps(sym("selectedOption"),{sym("cssImageKind"),pvar(110),pvar(111)})}),apps(sym("loadedCSSImage"),{pvar(110),pvar(111)})));
  rules.push_back(rule(apps(sym("loadSelected"),{apps(sym("selectedOption"),{sym("gradientKind"),pvar(112),pvar(113)})}),app(sym("generatedGradient"),pvar(112))));
  rules.push_back(rule(apps(sym("loadSelected"),{apps(sym("selectedOption"),{sym("unsupportedKind"),pvar(114),pvar(115)})}),sym("noLoadedImage")));
  rules.push_back(rule(app(sym("wrapImageSet"),pvar(116)),app(sym("styleImageSetWrapper"),pvar(116))));

  sig["CacheMode"]=type_sort(); sig["sharedMode"]=M; sig["fetcherMode"]=M;
  sig["Fetcher"]=type_sort(); sig["fetcherA"]=F; sig["fetcherB"]=F;
  sig["CacheSlot"]=type_sort(); sig["cacheEmpty"]=S; sig["cacheHasImage"]=fn(I,S);
  sig["CacheState"]=type_sort(); sig["sharedEmpty"]=C; sig["sharedCache"]=fn(S,C);
  sig["fetcherCache"]=fn(S,fn(S,C));
  sig["PipelineResult"]=type_sort(); sig["pipelineResult"]=fn(O,fn(W,fn(C,V)));
  sig["updateCache"]=fn(M,fn(C,fn(F,fn(W,C))));
  sig["selectLoadCache"]=fn(sym("OptionList"),fn(sym("DeviceScale"),fn(sym("MimeFeatures"),fn(M,fn(F,fn(C,V))))));
  auto cache_rule = [&](Ptr mode, Ptr state, Ptr fetcher, Ptr wrapper, Ptr result) {
    rules.push_back(rule(apps(sym("updateCache"),{mode,state,fetcher,wrapper}),result));
  };
  cache_rule(sym("sharedMode"),pvar(120),pvar(121),app(sym("styleImageSetWrapper"),sym("noLoadedImage")),app(sym("sharedCache"),sym("cacheEmpty")));
  cache_rule(sym("sharedMode"),pvar(122),pvar(123),app(sym("styleImageSetWrapper"),apps(sym("loadedCSSImage"),{pvar(124),pvar(125)})),app(sym("sharedCache"),app(sym("cacheHasImage"),pvar(124))));
  cache_rule(sym("sharedMode"),pvar(126),pvar(127),app(sym("styleImageSetWrapper"),app(sym("generatedGradient"),pvar(128))),app(sym("sharedCache"),app(sym("cacheHasImage"),pvar(128))));
  cache_rule(sym("fetcherMode"),apps(sym("fetcherCache"),{pvar(129),pvar(130)}),sym("fetcherA"),app(sym("styleImageSetWrapper"),sym("noLoadedImage")),apps(sym("fetcherCache"),{sym("cacheEmpty"),pvar(130)}));
  cache_rule(sym("fetcherMode"),apps(sym("fetcherCache"),{pvar(131),pvar(132)}),sym("fetcherB"),app(sym("styleImageSetWrapper"),sym("noLoadedImage")),apps(sym("fetcherCache"),{pvar(131),sym("cacheEmpty")}));
  cache_rule(sym("fetcherMode"),apps(sym("fetcherCache"),{pvar(133),pvar(134)}),sym("fetcherA"),app(sym("styleImageSetWrapper"),apps(sym("loadedCSSImage"),{pvar(135),pvar(136)})),apps(sym("fetcherCache"),{app(sym("cacheHasImage"),pvar(135)),pvar(134)}));
  cache_rule(sym("fetcherMode"),apps(sym("fetcherCache"),{pvar(137),pvar(138)}),sym("fetcherB"),app(sym("styleImageSetWrapper"),apps(sym("loadedCSSImage"),{pvar(139),pvar(140)})),apps(sym("fetcherCache"),{pvar(137),app(sym("cacheHasImage"),pvar(139))}));
  cache_rule(sym("fetcherMode"),apps(sym("fetcherCache"),{pvar(141),pvar(142)}),sym("fetcherA"),app(sym("styleImageSetWrapper"),app(sym("generatedGradient"),pvar(143))),apps(sym("fetcherCache"),{app(sym("cacheHasImage"),pvar(143)),pvar(142)}));
  cache_rule(sym("fetcherMode"),apps(sym("fetcherCache"),{pvar(144),pvar(145)}),sym("fetcherB"),app(sym("styleImageSetWrapper"),app(sym("generatedGradient"),pvar(146))),apps(sym("fetcherCache"),{pvar(144),app(sym("cacheHasImage"),pvar(146))}));
  Ptr best=apps(sym("bestOption"),{pvar(130),pvar(131),pvar(132)});
  Ptr selected=app(sym("toSelectedOption"),best);
  Ptr wrapper=app(sym("wrapImageSet"),app(sym("loadSelected"),selected));
  Ptr updated=apps(sym("updateCache"),{pvar(133),pvar(135),pvar(134),wrapper});
  rules.push_back(rule(apps(sym("selectLoadCache"),{pvar(130),pvar(131),pvar(132),pvar(133),pvar(134),pvar(135)}),
      apps(sym("pipelineResult"),{selected,wrapper,updated})));

  RuleIndex index(rules);
  const std::vector<Options> option_cases={
      {{0,1,"image/jpeg"},{2,2,"image/png"},{1,3,"image/avif"}},
      {{5,1,"image/jpeg"}},
      {{5,1,"image/svg+xml"}}};
  unsigned states=0;
  for(const Options& input:option_cases) for(unsigned scale=1;scale<=4;++scale) for(unsigned avif=0;avif<2;++avif)
    for(CacheMode mode:{CacheMode::Shared,CacheMode::FetcherKeyed}) for(unsigned fetcher=0;fetcher<2;++fetcher) {
      MimeFeatures features{avif!=0,false,false};
      CacheState state{mode,std::nullopt,{std::nullopt,std::nullopt}};
      Ptr term=pipeline_term(input,scale,features,mode,fetcher,state);
      auto inferred=infer({},term,sig,rules);
      if(!inferred || !conv(inferred.value(),V,index)) { std::cerr<<"selection/load/cache term failed type inference\n"; return 2; }
      Ptr reduced=nf(term,index);
      const auto expected=cpp_pipeline(input,scale,features,mode,fetcher,state);
      const int selected=cpp_best_option(input,scale,features);
      Ptr selected_term=selected<0?sym("noSelectedOption"):
          apps(sym("selectedOption"),{option_kind_term(cpp_value_kind(static_cast<unsigned>(selected))),sym(("image"+std::to_string(selected)).c_str()),resolution_term(input[0].resolution)});
      if(selected>=0) for(const auto& option:input) if(static_cast<int>(option.image)==selected)
        selected_term=apps(sym("selectedOption"),{option_kind_term(cpp_value_kind(option.image)),sym(("image"+std::to_string(option.image)).c_str()),resolution_term(option.resolution)});
      Ptr wrapper;
      if(selected<0||expected.kind==ValueKind::Unsupported) wrapper=app(sym("styleImageSetWrapper"),sym("noLoadedImage"));
      else if(expected.kind==ValueKind::Gradient) wrapper=app(sym("styleImageSetWrapper"),app(sym("generatedGradient"),sym(("image"+std::to_string(selected)).c_str())));
      else { unsigned resolution=0; for(const auto& option:input) if(static_cast<int>(option.image)==selected) resolution=option.resolution;
        wrapper=app(sym("styleImageSetWrapper"),apps(sym("loadedCSSImage"),{sym(("image"+std::to_string(selected)).c_str()),resolution_term(resolution)})); }
      Ptr cache=expected.cached_image<0? (mode==CacheMode::Shared?app(sym("sharedCache"),sym("cacheEmpty")):apps(sym("fetcherCache"),{fetcher==0?sym("cacheEmpty"):sym("cacheEmpty"),sym("cacheEmpty")})):
        (mode==CacheMode::Shared?app(sym("sharedCache"),app(sym("cacheHasImage"),sym(("image"+std::to_string(expected.cached_image)).c_str()))):
          apps(sym("fetcherCache"),{fetcher==0?app(sym("cacheHasImage"),sym(("image"+std::to_string(expected.cached_image)).c_str())):sym("cacheEmpty"),fetcher==1?app(sym("cacheHasImage"),sym(("image"+std::to_string(expected.cached_image)).c_str())):sym("cacheEmpty")}));
      Ptr wanted=apps(sym("pipelineResult"),{selected_term,wrapper,cache});
      if(!conv(reduced,wanted,index)) { std::cerr<<"selection/load/cache result mismatch at scale "<<scale<<"\n"; return 3; }
      ++states;
    }
  std::cout<<"Composed MIME-aware option selection, load dispatch, wrapper creation, and cache update across "<<states<<" finite transitions.\n";
  std::cout<<"Scope: three options with resolutions 1..3, four device scales, AVIF feature gate, CSS image/gradient/unsupported kind observations, and empty cache states. Resource loading, lazy options_ mutation, arbitrary strings, floating resolutions, and GC identity are abstract.\n";
}
