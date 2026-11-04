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
  Ptr type = B;
  for (unsigned i = 0; i < 19; ++i) type = fn(B, type);
  sig["needsReattach"] = type;

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
  rules.push_back(R(ap(sym("needsReattach"), pattern(0, true)), sym("false")));
  for (unsigned i = 1; i < 19; ++i) {
    rules.push_back(R(ap(sym("needsReattach"), pattern(0, false, i, true)),
                       sym("true")));
  }
  std::vector<Ptr> fallback;
  for (unsigned i = 0; i < 19; ++i) fallback.push_back(pvar(i));
  rules.push_back(R(ap(sym("needsReattach"), fallback), sym("false")));

  RuleIndex ix(rules);
  std::vector<Ptr> sample_args(19, sym("false"));
  Ptr sample = ap(sym("needsReattach"), sample_args);
  auto result_type = infer({}, sample, sig, rules);
  assert(result_type && conv(*result_type, B, ix));

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
  std::cout << "NeedsReattachLayoutTree agrees on " << compared
            << " abstract source-condition assignments.\n";
  std::cout << "Scope: 19 source observations are varied independently; compound element/style predicates are inputs, not evaluated fields.\n";
}
