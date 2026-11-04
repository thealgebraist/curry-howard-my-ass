#include "../src/lambda_pi.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace lp;

// Source-condition abstraction of ComputedStyle::NeedsReattachLayoutTree.
// Compound conditions are named as already-evaluated Boolean observations.
struct ReattachConditions {
  bool same_style_pointer, old_style_missing, new_style_missing;
  bool display_changed, first_letter_presence_changed, content_data_different;
  bool text_combine_changed, scroll_marker_group_different;
  bool marker_group_scroll_container_changed;
  bool marker_group_secondary_body_changed;
  bool effective_overscroll_container_changed, custom_display_layout_name_changed;
  bool progress_effective_appearance_changed, base_appearance_support_changed;
  bool legend_float_or_out_of_flow_changed;
  bool text_combine_writing_mode_support_changed;
  bool first_line_pseudo_added, overlay_changed, list_style_position_changed;
};
enum class WritingMode : unsigned { HorizontalTb, VerticalRl, VerticalLr, SidewaysRl, SidewaysLr };
enum class MarkerPosition : unsigned { After, Before };
enum class MarkerMode : unsigned { Tabs, Links };
enum class MarkerGroup : unsigned { None, AfterLinks, AfterTabs, BeforeLinks, BeforeTabs };
struct MarkerGroupValue { bool present; MarkerPosition position; MarkerMode mode; };
static MarkerGroupValue cpp_marker_group_value(MarkerGroup group) {
  switch (group) {
    case MarkerGroup::None: return {false,MarkerPosition::After,MarkerMode::Links};
    case MarkerGroup::AfterLinks: return {true,MarkerPosition::After,MarkerMode::Links};
    case MarkerGroup::AfterTabs: return {true,MarkerPosition::After,MarkerMode::Tabs};
    case MarkerGroup::BeforeLinks: return {true,MarkerPosition::Before,MarkerMode::Links};
    case MarkerGroup::BeforeTabs: return {true,MarkerPosition::Before,MarkerMode::Tabs};
  }
  return {false,MarkerPosition::After,MarkerMode::Links};
}
static bool cpp_marker_group_equal(MarkerGroup old_group,MarkerGroup new_group) {
  auto old_value=cpp_marker_group_value(old_group),new_value=cpp_marker_group_value(new_group);
  if(!old_value.present||!new_value.present)return old_value.present==new_value.present;
  return old_value.position==new_value.position&&old_value.mode==new_value.mode;
}
static bool cpp_text_combine_supported(WritingMode mode) {
  return mode==WritingMode::VerticalRl||mode==WritingMode::VerticalLr;
}
static bool cpp_text_combine_mode_change(bool new_has_text_combine,
                                         WritingMode old_mode,
                                         WritingMode new_mode) {
  return new_has_text_combine &&
         cpp_text_combine_supported(old_mode)!=cpp_text_combine_supported(new_mode);
}

static bool cpp_needs_reattach(const ReattachConditions& c) {
  if (c.same_style_pointer) return false;
  if (c.old_style_missing || c.new_style_missing) return true;
  return c.display_changed || c.first_letter_presence_changed ||
         c.content_data_different || c.text_combine_changed ||
         c.scroll_marker_group_different ||
         c.marker_group_scroll_container_changed ||
         c.marker_group_secondary_body_changed ||
         c.effective_overscroll_container_changed ||
         c.custom_display_layout_name_changed ||
         c.progress_effective_appearance_changed ||
         c.base_appearance_support_changed ||
         c.legend_float_or_out_of_flow_changed ||
         c.text_combine_writing_mode_support_changed ||
         c.first_line_pseudo_added || c.overlay_changed ||
         c.list_style_position_changed;
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule R(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }
static Ptr ap(Ptr f, const std::vector<Ptr>& xs) {
  for (const auto& x : xs) f = app(f, x);
  return f;
}
static Ptr bt(bool value) { return sym(value ? "true" : "false"); }

int main() {
  Signature sig;
  std::vector<Rule> rules;
  Ptr B = sym("Bool");
  sig["Bool"] = type_sort();
  sig["true"] = B;
  sig["false"] = B;
  sig["notB"]=fn(B,B);
  Ptr type = B;
  for (unsigned i = 0; i < 19; ++i) type = fn(B, type);
  sig["needsReattach"] = type;
  Ptr W=sym("WritingMode");sig["WritingMode"]=type_sort();
  const char* mode_names[]={"horizontalTb","verticalRl","verticalLr","sidewaysRl","sidewaysLr"};
  const WritingMode modes[]={WritingMode::HorizontalTb,WritingMode::VerticalRl,WritingMode::VerticalLr,WritingMode::SidewaysRl,WritingMode::SidewaysLr};
  for(const char* n:mode_names)sig[n]=W;
  sig["textCombineModeChanged"]=fn(B,fn(W,fn(W,B)));
  Ptr G=sym("ScrollMarkerGroupValue");sig["ScrollMarkerGroupValue"]=type_sort();
  const char* group_names[]={"groupNone","groupAfterLinks","groupAfterTabs","groupBeforeLinks","groupBeforeTabs"};
  const MarkerGroup groups[]={MarkerGroup::None,MarkerGroup::AfterLinks,MarkerGroup::AfterTabs,MarkerGroup::BeforeLinks,MarkerGroup::BeforeTabs};
  for(const char* n:group_names)sig[n]=G;
  sig["scrollMarkerGroupEqual"]=fn(G,fn(G,B));

  auto pattern = [](int special_index, bool special_value,
                    int other_index = -1, bool other_value = false) {
    std::vector<Ptr> args;
    for (unsigned i = 0; i < 19; ++i) {
      if (static_cast<int>(i) == special_index) args.push_back(bt(special_value));
      else if (static_cast<int>(i) == other_index) args.push_back(bt(other_value));
      else args.push_back(pvar(i));
    }
    return args;
  };
  // Pointer identity suppresses every later condition. Otherwise any null-style
  // condition or source comparison that requires layout reconstruction wins.
  rules.push_back(R(app(sym("notB"),sym("true")),sym("false")));
  rules.push_back(R(app(sym("notB"),sym("false")),sym("true")));
  rules.push_back(R(ap(sym("needsReattach"), pattern(0, true)), sym("false")));
  for (unsigned i = 1; i < 19; ++i) {
    rules.push_back(R(ap(sym("needsReattach"), pattern(0, false, i, true)),
                       sym("true")));
  }
  std::vector<Ptr> fallback;
  for (unsigned i = 0; i < 19; ++i) fallback.push_back(pvar(i));
  rules.push_back(R(ap(sym("needsReattach"), fallback), sym("false")));
  const bool lambda_supported[]={false,true,true,false,false};
  for(bool new_has:{false,true})for(unsigned old_i=0;old_i<5;++old_i)for(unsigned new_i=0;new_i<5;++new_i){
    bool changed=new_has&&(lambda_supported[old_i]!=lambda_supported[new_i]);
    rules.push_back(R(ap(sym("textCombineModeChanged"),{bt(new_has),sym(mode_names[old_i]),sym(mode_names[new_i])}),bt(changed)));
  }
  for(unsigned old_i=0;old_i<5;++old_i)for(unsigned new_i=0;new_i<5;++new_i){
    // Distinct constructors encode the optional presence, position, and mode.
    rules.push_back(R(ap(sym("scrollMarkerGroupEqual"),{sym(group_names[old_i]),sym(group_names[new_i])}),bt(old_i==new_i)));
  }

  RuleIndex ix(rules);
  std::vector<Ptr> sample_args(19, sym("false"));
  Ptr sample = ap(sym("needsReattach"), sample_args);
  auto result_type = infer({}, sample, sig, rules);
  assert(result_type && conv(*result_type, B, ix));
  Ptr text_combine_sample=ap(sym("textCombineModeChanged"),{sym("true"),sym("horizontalTb"),sym("verticalRl")});
  auto text_combine_type=infer({},text_combine_sample,sig,rules);assert(text_combine_type&&conv(*text_combine_type,B,ix));
  Ptr marker_sample=ap(sym("scrollMarkerGroupEqual"),{sym("groupAfterLinks"),sym("groupBeforeTabs")});
  auto marker_type=infer({},marker_sample,sig,rules);assert(marker_type&&conv(*marker_type,B,ix));

  unsigned compared = 0;
  for (unsigned mask = 0; mask < (1u << 19); ++mask) {
    bool b[19];
    std::vector<Ptr> args;
    for (unsigned i = 0; i < 19; ++i) {
      b[i] = (mask & (1u << i)) != 0;
      args.push_back(bt(b[i]));
    }
    ReattachConditions c{b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
                         b[8], b[9], b[10], b[11], b[12], b[13], b[14],
                         b[15], b[16], b[17], b[18]};
    const bool expected = cpp_needs_reattach(c);
    const Ptr actual = nf(ap(sym("needsReattach"), args), ix);
    if (!conv(actual, bt(expected), ix)) {
      std::cerr << "NeedsReattachLayoutTree mismatch at condition mask " << mask
                << "\n";
      return 2;
    }
    ++compared;
  }
  unsigned text_combine_cases=0;
  for(bool old_has:{false,true})for(bool new_has:{false,true})for(WritingMode old_mode:modes)for(WritingMode new_mode:modes){
    const bool toggled=old_has!=new_has;
    const bool mode_trigger=cpp_text_combine_mode_change(new_has,old_mode,new_mode);
    std::vector<Ptr> args(19,sym("false"));args[6]=bt(toggled);args[15]=ap(sym("textCombineModeChanged"),{bt(new_has),sym(mode_names[static_cast<unsigned>(old_mode)]),sym(mode_names[static_cast<unsigned>(new_mode)])});
    auto term=ap(sym("needsReattach"),args);
    const bool expected=toggled||mode_trigger;
    Ptr actual=nf(nf(nf(term,ix),ix),ix);
    if(!conv(actual,bt(expected),ix)){std::cerr<<"text-combine reattach composition mismatch\n";return 3;}++text_combine_cases;
  }
  unsigned marker_equality_cases=0,marker_reattach_cases=0;
  for(unsigned old_i=0;old_i<5;++old_i)for(unsigned new_i=0;new_i<5;++new_i){
    auto equal_term=ap(sym("scrollMarkerGroupEqual"),{sym(group_names[old_i]),sym(group_names[new_i])});
    bool equal=cpp_marker_group_equal(groups[old_i],groups[new_i]);
    if(!conv(nf(equal_term,ix),bt(equal),ix)){std::cerr<<"scroll marker group equality mismatch\n";return 4;}++marker_equality_cases;
    std::vector<Ptr> args(19,sym("false"));args[7]=app(sym("notB"),equal_term);
    auto reattach_term=ap(sym("needsReattach"),args);
    if(!conv(nf(nf(reattach_term,ix),ix),bt(!equal),ix)){std::cerr<<"scroll marker group reattach composition mismatch old="<<old_i<<" new="<<new_i<<" actual="<<to_string(nf(nf(reattach_term,ix),ix))<<" expected="<<to_string(bt(!equal))<<"\n";return 5;}++marker_reattach_cases;
  }
  std::cout << "NeedsReattachLayoutTree agrees on " << compared
            << " abstract source-condition assignments.\n";
  std::cout << "Text-combine writing-mode subpredicate and reattach composition agree on " << text_combine_cases << " mode/state combinations.\n";
  std::cout << "ScrollMarkerGroup value equality agrees on " << marker_equality_cases << " optional position/mode pairs; composed reattach agrees on " << marker_reattach_cases << " pairs.\n";
  std::cout << "Scope: 19 source observations are varied independently; compound element/style predicates are inputs, not evaluated fields.\n";
}
