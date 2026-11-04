#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

using namespace lp;

enum class CacheMode { Shared, FetcherKeyed };
struct Entry { unsigned scale; unsigned image; };
struct CacheState {
  CacheMode mode;
  std::optional<Entry> shared;
  std::array<std::optional<Entry>, 2> keyed;
};
static const std::array<float, 3> scales = {
    1.0f, std::nextafter(1.0f, 2.0f), 2.0f};
static bool equal_resolution(unsigned a, unsigned b) {
  return std::abs(scales[a] - scales[b]) < std::numeric_limits<float>::epsilon();
}
static bool cpp_is_pending(const CacheState& state, unsigned fetcher, unsigned scale) {
  const std::optional<Entry>& entry = state.mode == CacheMode::Shared
      ? state.shared : state.keyed[fetcher];
  return !entry || !equal_resolution(scale, entry->scale);
}
static CacheState cpp_cache_image(CacheState state, unsigned fetcher,
                                  unsigned scale, unsigned image) {
  if (state.mode == CacheMode::Shared) state.shared = Entry{scale, image};
  else state.keyed[fetcher] = Entry{scale, image};
  return state;
}
static std::optional<unsigned> cpp_cached_image(const CacheState& state,
                                                unsigned fetcher,
                                                unsigned scale) {
  if (cpp_is_pending(state, fetcher, scale)) return std::nullopt;
  const auto& entry = state.mode == CacheMode::Shared
      ? state.shared : state.keyed[fetcher];
  return entry ? std::optional<unsigned>(entry->image) : std::nullopt;
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr apps(Ptr f, const std::vector<Ptr>& xs) {
  for (const auto& x : xs) f = app(f, x);
  return f;
}
static Ptr bool_term(bool value) { return sym(value ? "true" : "false"); }
static Ptr scale_term(unsigned scale) {
  return sym(scale == 0 ? "scaleOne" : scale == 1 ? "scaleOneNext" : "scaleTwo");
}
static Ptr fetcher_term(unsigned fetcher) { return sym(fetcher == 0 ? "fetcherA" : "fetcherB"); }
static Ptr image_term(unsigned image) { return sym(image == 0 ? "imageA" : "imageB"); }
static Ptr slot_term(const std::optional<Entry>& entry) {
  return entry ? apps(sym("cacheEntry"), {scale_term(entry->scale), image_term(entry->image)})
               : sym("emptySlot");
}
static Ptr state_term(const CacheState& state) {
  if (state.mode == CacheMode::Shared)
    return state.shared ? apps(sym("sharedEntry"),
        {scale_term(state.shared->scale), image_term(state.shared->image)})
        : sym("sharedEmpty");
  return apps(sym("fetcherCache"), {slot_term(state.keyed[0]), slot_term(state.keyed[1])});
}
static Ptr mode_term(CacheMode mode) {
  return sym(mode == CacheMode::Shared ? "sharedMode" : "fetcherKeyedMode");
}
static Ptr pending_term(CacheMode mode, const CacheState& state,
                        unsigned fetcher, unsigned scale) {
  return apps(sym("isCachePending"), {mode_term(mode), state_term(state),
      fetcher_term(fetcher), scale_term(scale)});
}
static Ptr cache_term(CacheMode mode, const CacheState& state, unsigned fetcher,
                      unsigned scale, unsigned image) {
  return apps(sym("cacheImage"), {mode_term(mode), state_term(state),
      fetcher_term(fetcher), scale_term(scale), image_term(image)});
}
static Ptr cached_term(CacheMode mode, Ptr state, unsigned fetcher, unsigned scale) {
  return apps(sym("cachedImage"), {mode_term(mode), std::move(state),
      fetcher_term(fetcher), scale_term(scale)});
}

int main() {
  const Ptr B=sym("Bool"), M=sym("CacheMode"), F=sym("Fetcher");
  const Ptr R=sym("Resolution"), I=sym("CachedImageId"), S=sym("CacheSlot");
  const Ptr C=sym("CacheState"), V=sym("CachedImageResult");
  Signature signature;
  std::vector<Rule> rules;
  signature["Bool"]=type_sort(); signature["true"]=B; signature["false"]=B;
  signature["CacheMode"]=type_sort(); signature["sharedMode"]=M; signature["fetcherKeyedMode"]=M;
  signature["Fetcher"]=type_sort(); signature["fetcherA"]=F; signature["fetcherB"]=F;
  signature["Resolution"]=type_sort();
  for(const char* scale:{"scaleOne","scaleOneNext","scaleTwo"})signature[scale]=R;
  signature["CachedImageId"]=type_sort(); signature["imageA"]=I; signature["imageB"]=I;
  signature["CacheSlot"]=type_sort(); signature["emptySlot"]=S;
  signature["cacheEntry"]=fn(R,fn(I,S));
  signature["CacheState"]=type_sort(); signature["sharedEmpty"]=C;
  signature["sharedEntry"]=fn(R,fn(I,C));
  signature["fetcherCache"]=fn(S,fn(S,C));
  signature["CachedImageResult"]=type_sort(); signature["noCachedImage"]=V;
  signature["cachedImageValue"]=fn(I,V);
  signature["boolNot"]=fn(B,B);
  signature["resolutionEqual"]=fn(R,fn(R,B));
  signature["isCachePending"]=fn(M,fn(C,fn(F,fn(R,B))));
  signature["cacheImage"]=fn(M,fn(C,fn(F,fn(R,fn(I,C)))));
  signature["cachedImage"]=fn(M,fn(C,fn(F,fn(R,V))));

  rules.push_back(rule(app(sym("boolNot"),sym("true")),sym("false")));
  rules.push_back(rule(app(sym("boolNot"),sym("false")),sym("true")));
  for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b)
    rules.push_back(rule(apps(sym("resolutionEqual"),{scale_term(a),scale_term(b)}),
                         bool_term(equal_resolution(a,b))));
  rules.push_back(rule(apps(sym("isCachePending"),
      {sym("sharedMode"),sym("sharedEmpty"),pvar(0),pvar(1)}),sym("true")));
  rules.push_back(rule(apps(sym("isCachePending"),
      {sym("sharedMode"),apps(sym("sharedEntry"),{pvar(2),pvar(3)}),pvar(4),pvar(5)}),
      app(sym("boolNot"),apps(sym("resolutionEqual"),{pvar(5),pvar(2)}))));
  rules.push_back(rule(apps(sym("isCachePending"),
      {sym("fetcherKeyedMode"),apps(sym("fetcherCache"),{sym("emptySlot"),pvar(6)}),sym("fetcherA"),pvar(7)}),sym("true")));
  rules.push_back(rule(apps(sym("isCachePending"),
      {sym("fetcherKeyedMode"),apps(sym("fetcherCache"),{apps(sym("cacheEntry"),{pvar(8),pvar(9)}),pvar(10)}),sym("fetcherA"),pvar(11)}),
      app(sym("boolNot"),apps(sym("resolutionEqual"),{pvar(11),pvar(8)}))));
  rules.push_back(rule(apps(sym("isCachePending"),
      {sym("fetcherKeyedMode"),apps(sym("fetcherCache"),{pvar(12),sym("emptySlot")}),sym("fetcherB"),pvar(13)}),sym("true")));
  rules.push_back(rule(apps(sym("isCachePending"),
      {sym("fetcherKeyedMode"),apps(sym("fetcherCache"),{pvar(14),apps(sym("cacheEntry"),{pvar(15),pvar(16)})}),sym("fetcherB"),pvar(17)}),
      app(sym("boolNot"),apps(sym("resolutionEqual"),{pvar(17),pvar(15)}))));

  rules.push_back(rule(apps(sym("cacheImage"),
      {sym("sharedMode"),pvar(20),pvar(21),pvar(22),pvar(23)}),
      apps(sym("sharedEntry"),{pvar(22),pvar(23)})));
  rules.push_back(rule(apps(sym("cacheImage"),
      {sym("fetcherKeyedMode"),apps(sym("fetcherCache"),{pvar(24),pvar(25)}),sym("fetcherA"),pvar(26),pvar(27)}),
      apps(sym("fetcherCache"),{apps(sym("cacheEntry"),{pvar(26),pvar(27)}),pvar(25)})));
  rules.push_back(rule(apps(sym("cacheImage"),
      {sym("fetcherKeyedMode"),apps(sym("fetcherCache"),{pvar(28),pvar(29)}),sym("fetcherB"),pvar(30),pvar(31)}),
      apps(sym("fetcherCache"),{pvar(28),apps(sym("cacheEntry"),{pvar(30),pvar(31)})})));

  rules.push_back(rule(apps(sym("cachedImage"),
      {sym("sharedMode"),apps(sym("sharedEntry"),{pvar(32),pvar(33)}),pvar(34),pvar(35)}),
      app(sym("cachedImageValue"),pvar(33))));
  rules.push_back(rule(apps(sym("cachedImage"),
      {sym("fetcherKeyedMode"),apps(sym("fetcherCache"),{apps(sym("cacheEntry"),{pvar(36),pvar(37)}),pvar(38)}),sym("fetcherA"),pvar(39)}),
      app(sym("cachedImageValue"),pvar(37))));
  rules.push_back(rule(apps(sym("cachedImage"),
      {sym("fetcherKeyedMode"),apps(sym("fetcherCache"),{pvar(40),apps(sym("cacheEntry"),{pvar(41),pvar(42)})}),sym("fetcherB"),pvar(43)}),
      app(sym("cachedImageValue"),pvar(42))));

  RuleIndex indexed_rules(rules);
  CacheState initial{CacheMode::Shared,std::nullopt,{}};
  auto sample=pending_term(CacheMode::Shared,initial,0,0);
  auto inferred=infer({},sample,signature,rules);
  assert(inferred&&conv(*inferred,B,indexed_rules));
  CacheState cached_initial{CacheMode::Shared,Entry{0,0},{}};
  auto cache_inferred=infer({},cache_term(CacheMode::Shared,initial,0,0,0),signature,rules);
  auto read_inferred=infer({},cached_term(CacheMode::Shared,state_term(cached_initial),0,0),signature,rules);
  assert(cache_inferred&&conv(*cache_inferred,C,indexed_rules));
  assert(read_inferred&&conv(*read_inferred,V,indexed_rules));

  std::vector<CacheState> states;
  states.push_back({CacheMode::Shared,std::nullopt,{}});
  for(unsigned scale=0;scale<3;++scale)for(unsigned image=0;image<2;++image)
    states.push_back({CacheMode::Shared,Entry{scale,image},{}});
  std::vector<std::optional<Entry>> slots={std::nullopt};
  for(unsigned scale=0;scale<3;++scale)for(unsigned image=0;image<2;++image)slots.push_back(Entry{scale,image});
  for(const auto& a:slots)for(const auto& b:slots){CacheState state{CacheMode::FetcherKeyed,std::nullopt,{a,b}};states.push_back(state);}

  unsigned pending_states=0,update_states=0,read_states=0,composed_queries=0;
  for(const auto& state:states){
    const unsigned mode_index=state.mode==CacheMode::Shared?0:1;
    CacheMode mode=static_cast<CacheMode>(mode_index);
    for(unsigned fetcher=0;fetcher<2;++fetcher)for(unsigned scale=0;scale<3;++scale){
      bool expected=cpp_is_pending(state,fetcher,scale);
      if(!conv(nf(pending_term(mode,state,fetcher,scale),indexed_rules),bool_term(expected),indexed_rules)){
        std::cerr<<"IsCachePending mismatch\n";return 2;
      }
      ++pending_states;
      if(!expected){
        auto image=cpp_cached_image(state,fetcher,scale);
        Ptr result=image?app(sym("cachedImageValue"),image_term(*image)):sym("noCachedImage");
        if(!conv(nf(cached_term(mode,state_term(state),fetcher,scale),indexed_rules),result,indexed_rules)){
          std::cerr<<"CachedImage lookup mismatch\n";return 3;
        }
        ++read_states;
      }
      for(unsigned image=0;image<2;++image){
        CacheState updated=cpp_cache_image(state,fetcher,scale,image);
        Ptr term=cache_term(mode,state,fetcher,scale,image);
        if(!conv(nf(term,indexed_rules),state_term(updated),indexed_rules)){
          std::cerr<<"CacheImage state transition mismatch\n";return 4;
        }
        ++update_states;
        for(unsigned query_fetcher=0;query_fetcher<2;++query_fetcher)
          for(unsigned query_scale=0;query_scale<3;++query_scale){
            bool updated_expected=cpp_is_pending(updated,query_fetcher,query_scale);
            Ptr query=apps(sym("isCachePending"),{mode_term(mode),term,
                fetcher_term(query_fetcher),scale_term(query_scale)});
            if(!conv(nf(query,indexed_rules),bool_term(updated_expected),indexed_rules)){
              std::cerr<<"CacheImage then IsCachePending mismatch\n";return 5;
            }
            ++composed_queries;
          }
      }
    }
  }
  std::cout<<"CSSImageSetValue shared/fetcher-keyed cache pending checks cover "<<pending_states<<" state/query combinations.\n";
  std::cout<<"CacheImage state updates cover "<<update_states<<" transitions; composed pending queries cover "<<composed_queries<<" states; permitted CachedImage reads cover "<<read_states<<" states.\n";
  std::cout<<"Scope: two fetchers, three finite float representatives (including a strict-epsilon boundary), two image identities, and the effective cache mode are modeled; GC identity, weak-key expiration, and concurrent mutation are omitted.\n";
}
