#define main blink_image_set_selection_standalone_main
#include "blink_image_set_selection.cpp"
#undef main

enum class OptionCacheKind { Uninitialized, NoSupportedOptions, Options };
struct OptionCache {
  OptionCacheKind kind = OptionCacheKind::Uninitialized;
  Options options;
};
struct Lookup { int selected; OptionCache cache; };
struct DoubleLookup { int first; int second; OptionCache cache; };

static Options cpp_prepare_options(const Options& options, MimeFeatures features) {
  return cpp_stable_sort_unique(cpp_filter(options, features));
}
static int cpp_choose_prepared(const Options& options, unsigned scale) {
  for (const auto& option : options) if (option.resolution >= scale) return option.image;
  return options.empty() ? -1 : static_cast<int>(options.back().image);
}
static Lookup cpp_lookup(OptionCache cache, const Options& input, unsigned scale,
                         MimeFeatures features) {
  if (cache.kind == OptionCacheKind::Uninitialized) {
    cache.options = cpp_prepare_options(input, features);
    cache.kind = cache.options.empty() ? OptionCacheKind::NoSupportedOptions
                                       : OptionCacheKind::Options;
  }
  if (cache.kind == OptionCacheKind::NoSupportedOptions) return {-1, cache};
  return {cpp_choose_prepared(cache.options, scale), cache};
}
static Ptr cache_term(const OptionCache& cache) {
  if (cache.kind == OptionCacheKind::Uninitialized) return sym("uninitializedOptions");
  if (cache.kind == OptionCacheKind::NoSupportedOptions) return sym("noSupportedOptions");
  return app(sym("cachedOptions"), options_term(cache.options));
}
static Ptr lookup_term(const OptionCache& cache, const Options& input,
                       unsigned scale, MimeFeatures features) {
  return apps(sym("lookupBestOption"), {cache_term(cache), options_term(input),
      scale_term(scale), features_term(features)});
}
static Ptr expected_selection(int image, const Options& options) {
  if (image < 0) return sym("noOption");
  for (const auto& option : options)
    if (static_cast<int>(option.image) == image)
      return app(sym("someOption"), option_term(option));
  return app(sym("someOption"), option_term({static_cast<unsigned>(image), 1, std::nullopt}));
}
static Ptr pair_lookup_term(const Options& options, unsigned first_scale,
                            MimeFeatures first_features, unsigned second_scale,
                            MimeFeatures second_features) {
  return apps(sym("lookupTwice"), {options_term(options), scale_term(first_scale),
      features_term(first_features), scale_term(second_scale), features_term(second_features)});
}

int main() {
  Signature sig;
  std::vector<Rule> rules;
  add_selection_rules(sig, rules);
  const Ptr B=sym("Bool"), O=sym("SelectedOption"), L=sym("OptionList");
  const Ptr D=sym("DeviceScale"), F=sym("MimeFeatures"), C=sym("OptionCache");
  const Ptr R=sym("LookupResult"), T=sym("DoubleLookupResult");
  sig["OptionCache"]=type_sort();
  sig["uninitializedOptions"]=C; sig["noSupportedOptions"]=C;
  sig["cachedOptions"]=fn(L,C);
  sig["LookupResult"]=type_sort(); sig["lookupResult"]=fn(O,fn(C,R));
  sig["DoubleLookupResult"]=type_sort();
  sig["twoLookups"]=fn(O,fn(R,T));
  sig["prepareOptions"]=fn(L,fn(F,L));
  sig["initializeLookup"]=fn(L,fn(D,R));
  sig["lookupBestOption"]=fn(C,fn(L,fn(D,fn(F,R))));
  sig["lookupAgain"]=fn(C,fn(D,fn(F,R)));
  sig["lookupTwice"]=fn(L,fn(D,fn(F,fn(D,fn(F,T)))));
  sig["lookupTwiceStep"]=fn(R,fn(D,fn(F,T)));
  rules.push_back(rule(apps(sym("prepareOptions"),{pvar(200),pvar(201)}),
      app(sym("uniqueOptions"),app(sym("sortOptions"),apps(sym("filterOptions"),{pvar(200),pvar(201)})))));
  rules.push_back(rule(apps(sym("initializeLookup"),{sym("nilOptions"),pvar(202)}),
      apps(sym("lookupResult"),{sym("noOption"),sym("noSupportedOptions")})));
  rules.push_back(rule(apps(sym("initializeLookup"),{apps(sym("consOption"),{pvar(203),pvar(204)}),pvar(205)}),
      apps(sym("lookupResult"),{apps(sym("selectOption"),{pvar(205),apps(sym("consOption"),{pvar(203),pvar(204)}),sym("noOption")}),app(sym("cachedOptions"),apps(sym("consOption"),{pvar(203),pvar(204)}))})));
  rules.push_back(rule(apps(sym("lookupBestOption"),{sym("uninitializedOptions"),pvar(206),pvar(207),pvar(208)}),
      apps(sym("initializeLookup"),{apps(sym("prepareOptions"),{pvar(206),pvar(208)}),pvar(207)})));
  rules.push_back(rule(apps(sym("lookupBestOption"),{sym("noSupportedOptions"),pvar(209),pvar(210),pvar(211)}),
      apps(sym("lookupResult"),{sym("noOption"),sym("noSupportedOptions")})));
  rules.push_back(rule(apps(sym("lookupBestOption"),{app(sym("cachedOptions"),pvar(212)),pvar(213),pvar(214),pvar(215)}),
      apps(sym("lookupResult"),{apps(sym("selectOption"),{pvar(214),pvar(212),sym("noOption")}),app(sym("cachedOptions"),pvar(212))})));
  rules.push_back(rule(apps(sym("lookupAgain"),{sym("noSupportedOptions"),pvar(216),pvar(217)}),
      apps(sym("lookupResult"),{sym("noOption"),sym("noSupportedOptions")})));
  rules.push_back(rule(apps(sym("lookupAgain"),{app(sym("cachedOptions"),pvar(218)),pvar(219),pvar(220)}),
      apps(sym("lookupResult"),{apps(sym("selectOption"),{pvar(219),pvar(218),sym("noOption")}),app(sym("cachedOptions"),pvar(218))})));
  rules.push_back(rule(apps(sym("lookupTwiceStep"),{apps(sym("lookupResult"),{pvar(221),pvar(222)}),pvar(223),pvar(224)}),
      apps(sym("twoLookups"),{pvar(221),apps(sym("lookupAgain"),{pvar(222),pvar(223),pvar(224)})})));
  rules.push_back(rule(apps(sym("lookupTwice"),{pvar(225),pvar(226),pvar(227),pvar(228),pvar(229)}),
      apps(sym("lookupTwiceStep"),{apps(sym("lookupBestOption"),{sym("uninitializedOptions"),pvar(225),pvar(226),pvar(227)}),pvar(228),pvar(229)})));

  RuleIndex index(rules);
  const std::vector<Options> inputs={
      {{0,1,"image/jpeg"},{1,2,"image/avif"}},
      {{2,1,"image/svg+xml"}}};
  unsigned states=0;
  for(const auto& input:inputs) for(unsigned first_scale=1;first_scale<=4;++first_scale)
    for(unsigned second_scale=1;second_scale<=4;++second_scale) for(unsigned first_avif=0;first_avif<2;++first_avif)
      for(unsigned second_avif=0;second_avif<2;++second_avif) {
        MimeFeatures first{first_avif!=0,false,false}, second{second_avif!=0,false,false};
        OptionCache empty;
        Ptr first_term=lookup_term(empty,input,first_scale,first);
        auto first_type=infer({},first_term,sig,rules);
        if(!first_type || !conv(first_type.value(),R,index)) { std::cerr<<"GetBestOption cache lookup failed type inference\n"; return 2; }
        Lookup first_expected=cpp_lookup(empty,input,first_scale,first);
        Ptr first_wanted=apps(sym("lookupResult"),{expected_selection(first_expected.selected,first_expected.cache.options),cache_term(first_expected.cache)});
        if(!conv(nf(first_term,index),first_wanted,index)) { std::cerr<<"GetBestOption first-call cache mismatch\n"; return 3; }
        Ptr twice=pair_lookup_term(input,first_scale,first,second_scale,second);
        auto twice_type=infer({},twice,sig,rules);
        if(!twice_type || !conv(twice_type.value(),T,index)) { std::cerr<<"GetBestOption repeated lookup failed type inference\n"; return 4; }
        Lookup second_expected=cpp_lookup(first_expected.cache,input,second_scale,second);
        Ptr twice_wanted=apps(sym("twoLookups"),{
            expected_selection(first_expected.selected,first_expected.cache.options),
            apps(sym("lookupResult"),{expected_selection(second_expected.selected,second_expected.cache.options),cache_term(second_expected.cache)})});
        if(!conv(nf(twice,index),twice_wanted,index)) { std::cerr<<"GetBestOption cache reuse mismatch\n"; return 5; }
        ++states;
      }
  std::cout<<"CSSImageSetValue::GetBestOption lazy options_ cache and repeat selection cover "<<states<<" finite two-call states.\n";
  std::cout<<"Scope: two option lists, resolutions 1..2, scales 1..4, AVIF on/off at each call, and first-call initialization including the nullptr sentinel. Computed-resolution recalculation, mutable resolver internals, options_ pointer identity, arbitrary floats, and concurrent calls are abstract.\n";
  return 0;
}
