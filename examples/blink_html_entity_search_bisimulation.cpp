#include "../src/lambda_pi.hpp"
#include "blink_html_entity_search_model.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <vector>

using namespace lp;

using blink_entity_semantics::Entity;
using blink_entity_semantics::load_entities;

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }
static Ptr p(int i) { return pvar(i); }
static Ptr nat(unsigned n) { Ptr r = sym("z"); while (n--) r = app(sym("s"), r); return r; }
static Ptr ch(unsigned char c) { return sym("ch" + std::to_string(c)); }
static Ptr unit(unsigned u) { return sym("u16_" + std::to_string(u)); }
static Ptr chars(const std::string& s) {
  Ptr r = sym("nilChar");
  for (auto i = s.rbegin(); i != s.rend(); ++i) r = apps(sym("consChar"), ch(static_cast<unsigned char>(*i)), r);
  return r;
}
static Ptr units_term(const std::vector<unsigned>& values) {
  Ptr r = sym("nilUnits");
  for (auto i = values.rbegin(); i != values.rend(); ++i) r = apps(sym("consUnit"), unit(*i), r);
  return r;
}
static Ptr token_term(const std::vector<unsigned>& values, bool has_entity, bool is_character_token) {
  return apps(sym("token"), units_term(values), sym(has_entity ? "true" : "false"), sym(is_character_token ? "true" : "false"));
}
struct Search { unsigned current_length = 0; std::vector<size_t> range; std::optional<size_t> most_recent_match; };
static Search initial_search(size_t count) {
  Search state;
  for (size_t i = 0; i < count; ++i) state.range.push_back(i);
  return state;
}
static Search cpp_advance(const std::vector<Entity>& table, Search state, unsigned char next) {
  std::vector<size_t> narrowed;
  if (state.current_length == 0) {
    for (size_t id = 0; id < table.size(); ++id)
      if (!table[id].name.empty() && table[id].name.front() == next) narrowed.push_back(id);
  } else {
    for (size_t id : state.range) {
      const auto& name = table[id].name;
      if (state.current_length < name.size() && name[state.current_length] == next) narrowed.push_back(id);
    }
  }
  state.range = std::move(narrowed);
  if (!state.range.empty()) {
    ++state.current_length;
    if (table[state.range.front()].name.size() == state.current_length)
      state.most_recent_match = state.range.front();
  }
  return state;
}
static Ptr search_term(const std::vector<Entity>& table, const Search& state) {
  Ptr range = sym("nilEntities");
  for (auto i = state.range.rbegin(); i != state.range.rend(); ++i)
    range = apps(sym("consEntity"), apps(sym("entity"), chars(table[*i].name), nat(static_cast<unsigned>(table[*i].name.size())), sym("cp_" + std::to_string(table[*i].first_value)), table[*i].second_value ? app(sym("someSecond"), sym("cp_" + std::to_string(*table[*i].second_value))) : sym("noSecond"), sym(table[*i].semicolon ? "true" : "false")), range);
  Ptr match = state.most_recent_match
      ? app(sym("someMatch"), apps(sym("entity"), chars(table[*state.most_recent_match].name), nat(static_cast<unsigned>(table[*state.most_recent_match].name.size())), sym("cp_" + std::to_string(table[*state.most_recent_match].first_value)), table[*state.most_recent_match].second_value ? app(sym("someSecond"), sym("cp_" + std::to_string(*table[*state.most_recent_match].second_value))) : sym("noSecond"), sym(table[*state.most_recent_match].semicolon ? "true" : "false")))
      : sym("noMatch");
  return apps(sym("searchState"), nat(state.current_length), range, match);
}
static Ptr result_term(const std::vector<Entity>& table, const Search& state) {
  if (!state.most_recent_match) return sym("noEntity");
  const auto& entity = table[*state.most_recent_match];
  return apps(sym("decodedEntity"), sym("cp_" + std::to_string(entity.first_value)),
      entity.second_value ? app(sym("someSecond"), sym("cp_" + std::to_string(*entity.second_value))) : sym("noSecond"));
}
enum class ConsumeKind { NeedMore, Failure, GuardReject, Decoded };
struct ConsumeOutcome {
  ConsumeKind kind = ConsumeKind::Failure;
  unsigned first = 0;
  std::optional<unsigned> second;
  size_t consumed = 0;
  std::string remainder;
};
struct ConsumeCase {
  std::string input;
  bool eof;
  bool additional_allowed;
  bool rcdata = false;
  std::vector<unsigned> prior_units;
  bool prior_has_entity = false;
  bool prior_character_token = false;
  bool compose_process_entity = true;
};
struct ProcessOutcome { bool suspended = false; bool rcdata = false; bool returns_now = false; std::vector<unsigned> units; bool has_entity = false; bool is_character_token = false; };
static std::vector<unsigned> utf16_units(unsigned codepoint) {
  if (codepoint <= 0xFFFF) return {codepoint};
  codepoint -= 0x10000;
  return {0xD800 + (codepoint >> 10), 0xDC00 + (codepoint & 0x3FF)};
}
static ProcessOutcome cpp_process_entity(const ConsumeOutcome& decoded, const ConsumeCase& item) {
  ProcessOutcome out{false, item.rcdata, false, item.prior_units, item.prior_has_entity, item.prior_character_token};
  if (decoded.kind == ConsumeKind::NeedMore) { out.suspended = true; out.returns_now = item.prior_character_token; return out; }
  if (decoded.kind == ConsumeKind::Failure || decoded.kind == ConsumeKind::GuardReject) {
    out.units.push_back(0x26);
    out.is_character_token = true;
    return out;
  }
  const auto first = utf16_units(decoded.first);
  out.units.insert(out.units.end(), first.begin(), first.end());
  if (decoded.second) {
    const auto second = utf16_units(*decoded.second);
    out.units.insert(out.units.end(), second.begin(), second.end());
  }
  assert(out.units.size() <= item.prior_units.size() + 4);
  out.has_entity = true;
  out.is_character_token = true;
  return out;
}
static bool ascii_alnum(unsigned char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
static ConsumeOutcome cpp_consume(const std::vector<Entity>& table, const Search& state,
                                  const std::string& input, bool eof, bool additional_allowed) {
  if (eof && state.current_length && input[state.current_length - 1] != ';')
    return {ConsumeKind::NeedMore, 0, std::nullopt, 0, input};
  if (!state.most_recent_match) return {ConsumeKind::Failure, 0, std::nullopt, 0, input};
  const Entity& entity = table[*state.most_recent_match];
  const unsigned char next = entity.name.size() < input.size()
      ? static_cast<unsigned char>(input[entity.name.size()]) : 0;
  if (!entity.semicolon && additional_allowed && (ascii_alnum(next) || next == '='))
    return {ConsumeKind::GuardReject, 0, std::nullopt, 0, input};
  return {ConsumeKind::Decoded, entity.first_value, entity.second_value,
          entity.name.size(), input.substr(entity.name.size())};
}
static Ptr consume_term(const ConsumeOutcome& outcome, const std::string& input) {
  switch (outcome.kind) {
    case ConsumeKind::NeedMore: return app(sym("consumeNeedMore"), chars(input));
    case ConsumeKind::Failure: return app(sym("consumeFailure"), chars(input));
    case ConsumeKind::GuardReject: return app(sym("consumeGuardReject"), chars(input));
    case ConsumeKind::Decoded:
      return apps(sym("consumeDecoded"), sym("cp_" + std::to_string(outcome.first)),
          outcome.second ? app(sym("someSecond"), sym("cp_" + std::to_string(*outcome.second))) : sym("noSecond"),
          nat(static_cast<unsigned>(outcome.consumed)), chars(outcome.remainder));
  }
  return app(sym("consumeFailure"), chars(input));
}
static Ptr process_term(const ProcessOutcome& outcome) {
  const char* state = outcome.suspended
      ? (outcome.rcdata ? "charRefRCDATAState" : "charRefDataState")
      : (outcome.rcdata ? "rcdataState" : "dataState");
  return apps(sym("tokenizerOutcome"), sym(outcome.suspended ? "suspend" : "resume"),
      sym(state),
      sym(outcome.returns_now ? "true" : "false"),
      token_term(outcome.units, outcome.has_entity, outcome.is_character_token));
}

int main() {
  const auto table = load_entities();
  Signature sig;
  std::vector<Rule> rules;
  blink_entity_semantics::installNamedEntitySemantics(sig, rules, table);
  const Ptr Nat = sym("Nat"), SearchState = sym("SearchState"), EntityResult = sym("EntityResult");
  const Ptr ConsumeResult = sym("ConsumeResult"), TokenizerOutcome = sym("TokenizerOutcome");
  // Finite observations use prefixes from the pinned entity table, including
  // terminal names and prefixes that fail after a successful match.
  const std::vector<std::string> prefixes = {"A", "am", "amp", "amp;", "not", "noti", "notin;", "not="};
  const std::vector<ConsumeCase> consume_cases = {
      {"amp;", true, false, false, {}, false}, {"NotEqualTilde;", true, false, true, {0x70}, false, true},
      {"Afr;", true, false, true, {0x70}, false, true},
      {"not", true, false, true, {0x70}, true, true}, {"notx", false, false, false, {0x70}, true, true},
      {"notx", false, true, true, {0x70}, false, true, false}, {"not=", false, true, false, {}, false},
      {"copyx", false, false, false, {}, false}, {"copyx", false, true, true, {0x6F}, true, true, false},
      {"xyz", false, false, false, {}, false}, {"amp;!", false, true, true, {0x71}, false, true},
      {"NotEqu", true, false, false, {0x70}, true, true}, {"NotEqu", true, false, false, {}, false, false}};
  sig["expected"] = fn(Nat, SearchState);
  std::vector<Search> expected_states;
  for (size_t i = 0; i < prefixes.size(); ++i) {
    rules.push_back(rule(app(sym("observe"), nat(static_cast<unsigned>(i))), apps(sym("run"), app(sym("initialState"), sym("allEntities")), sym("allEntities"), chars(prefixes[i]))));
    Search expected = initial_search(table.size());
    for (unsigned char letter : prefixes[i]) expected = cpp_advance(table, std::move(expected), letter);
    expected_states.push_back(std::move(expected));
    rules.push_back(rule(app(sym("expected"), nat(static_cast<unsigned>(i))), search_term(table, expected_states.back())));
    rules.push_back(rule(app(sym("observeResult"), nat(static_cast<unsigned>(i))), app(sym("searchResult"), apps(sym("run"), app(sym("initialState"), sym("allEntities")), sym("allEntities"), chars(prefixes[i])))));
    rules.push_back(rule(app(sym("expectedResult"), nat(static_cast<unsigned>(i))), result_term(table, expected_states.back())));
  }
  for (size_t i = 0; i < consume_cases.size(); ++i) {
    const auto& item = consume_cases[i];
    const Ptr id = nat(static_cast<unsigned>(i));
    rules.push_back(rule(app(sym("consumeInputAt"), id), chars(item.input)));
    rules.push_back(rule(app(sym("consumeEofAt"), id), sym(item.eof ? "true" : "false")));
    rules.push_back(rule(app(sym("consumeAdditionalAt"), id), sym(item.additional_allowed ? "true" : "false")));
    rules.push_back(rule(app(sym("returnAt"), id), sym(item.rcdata ? "rcdata" : "data")));
    rules.push_back(rule(app(sym("characterTokenAt"), id), sym(item.prior_character_token ? "true" : "false")));
    rules.push_back(rule(app(sym("tokenAt"), id), token_term(item.prior_units, item.prior_has_entity, item.prior_character_token)));
    rules.push_back(rule(app(sym("consumeAt"), id), apps(sym("consumeNamed"),
        apps(sym("run"), app(sym("initialState"), sym("allEntities")), sym("allEntities"), app(sym("consumeInputAt"), id)),
        app(sym("consumeInputAt"), id), app(sym("consumeEofAt"), id), app(sym("consumeAdditionalAt"), id))));
    Search scanned = initial_search(table.size());
    for (unsigned char c : item.input) scanned = cpp_advance(table, std::move(scanned), c);
    const ConsumeOutcome decoded = cpp_consume(table, scanned, item.input, item.eof, item.additional_allowed);
    rules.push_back(rule(app(sym("expectedConsume"), id), consume_term(decoded, item.input)));
    if (item.compose_process_entity)
      rules.push_back(rule(app(sym("expectedProcess"), id), process_term(cpp_process_entity(decoded, item))));
  }
  rules.push_back(rule(app(sym("processAt"), p(0)), apps(sym("processConsume"), app(sym("consumeAt"), p(0)), app(sym("returnAt"), p(0)), app(sym("characterTokenAt"), p(0)), app(sym("tokenAt"), p(0)))));
  sig["observe"] = fn(Nat, SearchState);
  RuleIndex index(rules);
  const auto type = infer({}, app(sym("observe"), nat(0)), sig, index);
  assert(type && conv(*type, SearchState, index));
  const auto expected_type = infer({}, app(sym("expected"), nat(0)), sig, index);
  assert(expected_type && conv(*expected_type, SearchState, index));
  const auto output_type = infer({}, app(sym("observeResult"), nat(0)), sig, index);
  assert(output_type && conv(*output_type, EntityResult, index));
  const auto consume_type = infer({}, app(sym("consumeAt"), nat(0)), sig, index);
  assert(consume_type && conv(*consume_type, ConsumeResult, index));
  const auto process_type = infer({}, app(sym("processAt"), nat(0)), sig, index);
  assert(process_type && conv(*process_type, TokenizerOutcome, index));
  for (size_t i = 0; i < prefixes.size(); ++i) {
    const Ptr actual = nf(app(sym("observe"), nat(static_cast<unsigned>(i))), index);
    const Ptr expected = app(sym("expected"), nat(static_cast<unsigned>(i)));
    if (!conv(actual, expected, index)) {
      std::println("HTMLEntitySearch transition mismatch for prefix {}", prefixes[i]);
      return 2;
    }
    const Ptr decoded = nf(app(sym("observeResult"), nat(static_cast<unsigned>(i))), index);
    if (!conv(decoded, app(sym("expectedResult"), nat(static_cast<unsigned>(i))), index)) {
      std::println("HTMLEntitySearch output mismatch for prefix {}", prefixes[i]);
      return 3;
    }
  }
  for (size_t i = 0; i < consume_cases.size(); ++i) {
    const Ptr actual = nf(app(sym("consumeAt"), nat(static_cast<unsigned>(i))), index);
    if (!conv(actual, app(sym("expectedConsume"), nat(static_cast<unsigned>(i))), index)) {
      std::println("ConsumeNamedEntity mismatch for candidate {}", consume_cases[i].input);
      return 4;
    }
    if (consume_cases[i].compose_process_entity) {
      const Ptr processed = nf(app(sym("processAt"), nat(static_cast<unsigned>(i))), index);
      if (!conv(processed, app(sym("expectedProcess"), nat(static_cast<unsigned>(i))), index)) {
        std::println("ProcessEntity composition mismatch for candidate {}", consume_cases[i].input);
        return 5;
      }
    }
  }
  std::println("Blink HTMLEntitySearch model: loaded {} CSV entries; built typed advance relation over range/length/last-match state", table.size());
  std::println("The source transition is represented over the full pinned table; {} finite prefixes are wired as observers", prefixes.size());
  std::println("ConsumeNamedEntity composition covers {} finite complete/cutoff candidates; segmented-string rewind operations remain abstracted as consumed length and restored suffix", consume_cases.size());
  std::println("ProcessEntity appends UTF-16 units, sets the entity flag on success, emits '&' on failure, and preserves the token on suspension.");
  std::println("Scope: numeric references, Windows-1252 correction, and actual SegmentedString mutations remain outside this model.");
}
