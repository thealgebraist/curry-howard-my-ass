#include "../src/lambda_pi.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

using namespace lp;

struct Entity { std::string name; unsigned first_value = 0; std::optional<unsigned> second_value; };
static std::vector<Entity> load_entities() {
  std::ifstream input("vendor/chromium-src/third_party/blink/renderer/core/html/parser/html_entity_names.csv");
  assert(input && "run from repository root: pinned Blink entity CSV is required");
  std::vector<Entity> result;
  std::string line;
  while (std::getline(input, line)) {
    const auto comma = line.find(',');
    assert(comma != std::string::npos);
    Entity entity;
    entity.name = line.substr(1, comma - 2);
    std::string values = line.substr(comma + 1);
    if (!values.empty() && values.front() == '"') values.erase(values.begin());
    if (!values.empty() && values.back() == '"') values.pop_back();
    size_t pos = 0;
    bool first_value_seen = false;
    while (pos < values.size()) {
      const size_t end = values.find(' ', pos);
      const std::string value = values.substr(pos, end == std::string::npos ? end : end - pos);
      assert(value.rfind("U+", 0) == 0);
      const unsigned codepoint = static_cast<unsigned>(std::stoul(value.substr(2), nullptr, 16));
      if (!first_value_seen) { entity.first_value = codepoint; first_value_seen = true; }
      else entity.second_value = codepoint;
      if (end == std::string::npos) break;
      pos = end + 1;
    }
    assert(first_value_seen);
    result.push_back(std::move(entity));
  }
  assert(result.size() == 2231);
  for (const auto& entity : result) {
    assert(!entity.name.empty());
    for (unsigned char c : entity.name) assert(c < 128);
  }
  // The generated HTMLEntityTable sorts the CSV rows before creating
  // kStaticEntityTable, so its observable range order is lexical by name.
  std::sort(result.begin(), result.end(), [](const Entity& a, const Entity& b) { return a.name < b.name; });
  assert(std::is_sorted(result.begin(), result.end(), [](const Entity& a, const Entity& b) { return a.name < b.name; }));
  assert(std::adjacent_find(result.begin(), result.end(), [](const Entity& a, const Entity& b) { return a.name == b.name; }) == result.end());
  return result;
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }
static Ptr p(int i) { return pvar(i); }
static Ptr nat(unsigned n) { Ptr r = sym("z"); while (n--) r = app(sym("s"), r); return r; }
static Ptr ch(unsigned char c) { return sym("ch" + std::to_string(c)); }
static Ptr chars(const std::string& s) {
  Ptr r = sym("nilChar");
  for (auto i = s.rbegin(); i != s.rend(); ++i) r = apps(sym("consChar"), ch(static_cast<unsigned char>(*i)), r);
  return r;
}
static Ptr table_term(const std::vector<Entity>& table) {
  Ptr r = sym("nilEntities");
  for (auto i = table.rbegin(); i != table.rend(); ++i)
    r = apps(sym("consEntity"), apps(sym("entity"), chars(i->name), nat(static_cast<unsigned>(i->name.size())), sym("cp_" + std::to_string(i->first_value)), i->second_value ? app(sym("someSecond"), sym("cp_" + std::to_string(*i->second_value))) : sym("noSecond")), r);
  return r;
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
    range = apps(sym("consEntity"), apps(sym("entity"), chars(table[*i].name), nat(static_cast<unsigned>(table[*i].name.size())), sym("cp_" + std::to_string(table[*i].first_value)), table[*i].second_value ? app(sym("someSecond"), sym("cp_" + std::to_string(*table[*i].second_value))) : sym("noSecond")), range);
  Ptr match = state.most_recent_match
      ? app(sym("someMatch"), apps(sym("entity"), chars(table[*state.most_recent_match].name), nat(static_cast<unsigned>(table[*state.most_recent_match].name.size())), sym("cp_" + std::to_string(table[*state.most_recent_match].first_value)), table[*state.most_recent_match].second_value ? app(sym("someSecond"), sym("cp_" + std::to_string(*table[*state.most_recent_match].second_value))) : sym("noSecond")))
      : sym("noMatch");
  return apps(sym("searchState"), nat(state.current_length), range, match);
}
static Ptr result_term(const std::vector<Entity>& table, const Search& state) {
  if (!state.most_recent_match) return sym("noEntity");
  const auto& entity = table[*state.most_recent_match];
  return apps(sym("decodedEntity"), sym("cp_" + std::to_string(entity.first_value)),
      entity.second_value ? app(sym("someSecond"), sym("cp_" + std::to_string(*entity.second_value))) : sym("noSecond"));
}

int main() {
  const auto table = load_entities();
  Signature sig;
  std::vector<Rule> rules;
  const Ptr Nat = sym("Nat"), Bool = sym("Bool"), Char = sym("Char"), Chars = sym("Chars");
  const Ptr Entity = sym("Entity"), Entities = sym("Entities"), MaybeChar = sym("MaybeChar");
  const Ptr Match = sym("Match"), SearchState = sym("SearchState");
  const Ptr Codepoint = sym("Codepoint"), OptionCp = sym("OptionCp"), EntityResult = sym("EntityResult");
  sig["Nat"] = type_sort(); sig["z"] = Nat; sig["s"] = fn(Nat, Nat);
  sig["Bool"] = type_sort(); sig["true"] = Bool; sig["false"] = Bool;
  sig["Char"] = type_sort(); sig["nilChar"] = Chars; sig["consChar"] = fn(Char, fn(Chars, Chars));
  for (unsigned i = 0; i < 128; ++i) sig["ch" + std::to_string(i)] = Char;
  sig["Chars"] = type_sort();
  sig["Codepoint"] = type_sort(); sig["OptionCp"] = type_sort(); sig["noSecond"] = OptionCp; sig["someSecond"] = fn(Codepoint, OptionCp);
  for (const auto& item : table) {
    sig["cp_" + std::to_string(item.first_value)] = Codepoint;
    if (item.second_value) sig["cp_" + std::to_string(*item.second_value)] = Codepoint;
  }
  sig["Entity"] = type_sort(); sig["entity"] = fn(Chars, fn(Nat, fn(Codepoint, fn(OptionCp, Entity))));
  sig["Entities"] = type_sort(); sig["nilEntities"] = Entities; sig["consEntity"] = fn(Entity, fn(Entities, Entities));
  sig["MaybeChar"] = type_sort(); sig["noChar"] = MaybeChar; sig["someChar"] = fn(Char, MaybeChar);
  sig["Match"] = type_sort(); sig["noMatch"] = Match; sig["someMatch"] = fn(Entity, Match);
  sig["EntityResult"] = type_sort(); sig["noEntity"] = EntityResult; sig["decodedEntity"] = fn(Codepoint, fn(OptionCp, EntityResult));
  sig["SearchState"] = type_sort(); sig["searchState"] = fn(Nat, fn(Entities, fn(Match, SearchState)));
  sig["entityName"] = fn(Entity, Chars); sig["entityLength"] = fn(Entity, Nat); sig["searchResult"] = fn(SearchState, EntityResult); sig["resultOfMatch"] = fn(Match, EntityResult);
  sig["observeResult"] = fn(Nat, EntityResult); sig["expectedResult"] = fn(Nat, EntityResult);
  sig["eqNat"] = fn(Nat, fn(Nat, Bool)); sig["eqChar"] = fn(Char, fn(Char, Bool));
  sig["eqMaybeChar"] = fn(MaybeChar, fn(MaybeChar, Bool));
  sig["charAt"] = fn(Chars, fn(Nat, MaybeChar));
  sig["filterFirst"] = fn(Entities, fn(Char, Entities));
  sig["filterAt"] = fn(Entities, fn(Nat, fn(Char, Entities)));
  sig["filterFirstStep"] = fn(Bool, fn(Entity, fn(Entities, fn(Char, Entities))));
  sig["filterAtStep"] = fn(Bool, fn(Entity, fn(Entities, fn(Nat, fn(Char, Entities)))));
  sig["advance"] = fn(SearchState, fn(Entities, fn(Char, SearchState)));
  sig["run"] = fn(SearchState, fn(Entities, fn(Chars, SearchState)));
  sig["advanceRange"] = fn(Entities, fn(Nat, fn(Match, SearchState)));
  sig["recordMatch"] = fn(Bool, fn(Entity, fn(Nat, fn(Match, Match))));
  sig["initialState"] = fn(Entities, SearchState);
  const auto yes = sym("true"), no = sym("false");

  rules.push_back(rule(apps(sym("eqNat"), sym("z"), sym("z")), yes));
  rules.push_back(rule(apps(sym("eqNat"), sym("z"), app(sym("s"), p(0))), no));
  rules.push_back(rule(apps(sym("eqNat"), app(sym("s"), p(0)), sym("z")), no));
  rules.push_back(rule(apps(sym("eqNat"), app(sym("s"), p(0)), app(sym("s"), p(1))), apps(sym("eqNat"), p(0), p(1))));
  for (unsigned a = 0; a < 128; ++a) for (unsigned b = 0; b < 128; ++b)
    rules.push_back(rule(apps(sym("eqChar"), ch(a), ch(b)), a == b ? yes : no));
  rules.push_back(rule(apps(sym("eqMaybeChar"), sym("noChar"), sym("noChar")), yes));
  rules.push_back(rule(apps(sym("eqMaybeChar"), apps(sym("someChar"), p(0)), apps(sym("someChar"), p(0))), yes));
  rules.push_back(rule(apps(sym("eqMaybeChar"), p(0), p(1)), no));
  rules.push_back(rule(app(sym("entityName"), apps(sym("entity"), p(0), p(1), p(2), p(3))), p(0)));
  rules.push_back(rule(app(sym("entityLength"), apps(sym("entity"), p(0), p(1), p(2), p(3))), p(1)));
  rules.push_back(rule(apps(sym("charAt"), sym("nilChar"), p(0)), sym("noChar")));
  rules.push_back(rule(apps(sym("charAt"), apps(sym("consChar"), p(0), p(1)), sym("z")), apps(sym("someChar"), p(0))));
  rules.push_back(rule(apps(sym("charAt"), apps(sym("consChar"), p(0), p(1)), app(sym("s"), p(2))), apps(sym("charAt"), p(1), p(2))));
  rules.push_back(rule(apps(sym("filterFirst"), sym("nilEntities"), p(0)), sym("nilEntities")));
  rules.push_back(rule(apps(sym("filterFirst"), apps(sym("consEntity"), p(0), p(1)), p(2)),
      apps(sym("filterFirstStep"), apps(sym("eqChar"), app(sym("headChar"), app(sym("entityName"), p(0))), p(2)), p(0), p(1), p(2))));
  sig["headChar"] = fn(Chars, Char);
  rules.push_back(rule(app(sym("headChar"), apps(sym("consChar"), p(0), p(1))), p(0)));
  rules.push_back(rule(apps(sym("filterFirstStep"), yes, p(0), p(1), p(2)), apps(sym("consEntity"), p(0), apps(sym("filterFirst"), p(1), p(2)))));
  rules.push_back(rule(apps(sym("filterFirstStep"), no, p(0), p(1), p(2)), apps(sym("filterFirst"), p(1), p(2))));
  rules.push_back(rule(apps(sym("filterAt"), sym("nilEntities"), p(0), p(1)), sym("nilEntities")));
  rules.push_back(rule(apps(sym("filterAt"), apps(sym("consEntity"), p(0), p(1)), p(2), p(3)),
      apps(sym("filterAtStep"), apps(sym("eqMaybeChar"), apps(sym("charAt"), app(sym("entityName"), p(0)), p(2)), apps(sym("someChar"), p(3))), p(0), p(1), p(2), p(3))));
  rules.push_back(rule(apps(sym("filterAtStep"), yes, p(0), p(1), p(2), p(3)), apps(sym("consEntity"), p(0), apps(sym("filterAt"), p(1), p(2), p(3)))));
  rules.push_back(rule(apps(sym("filterAtStep"), no, p(0), p(1), p(2), p(3)), apps(sym("filterAt"), p(1), p(2), p(3))));
  rules.push_back(rule(apps(sym("recordMatch"), yes, p(0), p(1), p(2)), apps(sym("someMatch"), p(0))));
  rules.push_back(rule(apps(sym("recordMatch"), no, p(0), p(1), p(2)), p(2)));
  rules.push_back(rule(apps(sym("advanceRange"), sym("nilEntities"), p(0), p(1)), apps(sym("searchState"), p(0), sym("nilEntities"), p(1))));
  rules.push_back(rule(apps(sym("advanceRange"), apps(sym("consEntity"), p(0), p(1)), p(2), p(3)),
      apps(sym("advanceRangeStep"), apps(sym("eqNat"), app(sym("entityLength"), p(0)), app(sym("s"), p(2))), p(0), p(1), p(2), p(3))));
  sig["advanceRangeStep"] = fn(Bool, fn(Entity, fn(Entities, fn(Nat, fn(Match, SearchState)))));
  rules.push_back(rule(apps(sym("advanceRangeStep"), p(4), p(0), p(1), p(2), p(3)),
      apps(sym("searchState"), app(sym("s"), p(2)), apps(sym("consEntity"), p(0), p(1)), apps(sym("recordMatch"), p(4), p(0), app(sym("s"), p(2)), p(3)))));
  rules.push_back(rule(apps(sym("advance"), apps(sym("searchState"), p(0), p(1), p(2)), p(3), p(4)),
      apps(sym("advanceFrom"), apps(sym("isZero"), p(0)), p(0), p(1), p(2), p(3), p(4))));
  sig["advanceFrom"] = fn(Bool, fn(Nat, fn(Entities, fn(Match, fn(Entities, fn(Char, SearchState))))));
  sig["isZero"] = fn(Nat, Bool); rules.push_back(rule(app(sym("isZero"), sym("z")), yes)); rules.push_back(rule(app(sym("isZero"), app(sym("s"), p(0))), no));
  rules.push_back(rule(apps(sym("advanceFrom"), yes, p(0), p(1), p(2), p(3), p(4)), apps(sym("advanceRange"), apps(sym("filterFirst"), p(3), p(4)), p(0), p(2))));
  rules.push_back(rule(apps(sym("advanceFrom"), no, p(0), p(1), p(2), p(3), p(4)), apps(sym("advanceRange"), apps(sym("filterAt"), p(1), p(0), p(4)), p(0), p(2))));
  rules.push_back(rule(app(sym("initialState"), p(0)), apps(sym("searchState"), sym("z"), p(0), sym("noMatch"))));
  rules.push_back(rule(apps(sym("run"), p(0), p(1), sym("nilChar")), p(0)));
  rules.push_back(rule(apps(sym("run"), p(0), p(1), apps(sym("consChar"), p(2), p(3))), apps(sym("run"), apps(sym("advance"), p(0), p(1), p(2)), p(1), p(3))));
  rules.push_back(rule(app(sym("resultOfMatch"), sym("noMatch")), sym("noEntity")));
  rules.push_back(rule(app(sym("resultOfMatch"), app(sym("someMatch"), apps(sym("entity"), p(0), p(1), p(2), p(3)))), apps(sym("decodedEntity"), p(2), p(3))));
  rules.push_back(rule(app(sym("searchResult"), apps(sym("searchState"), p(0), p(1), p(2))), app(sym("resultOfMatch"), p(2))));

  // Finite observations use prefixes from the pinned entity table, including
  // terminal names and prefixes that fail after a successful match.
  const std::vector<std::string> prefixes = {"A", "am", "amp", "amp;", "not", "noti", "notin;", "not="};
  sig["allEntities"] = Entities;
  rules.push_back(rule(sym("allEntities"), table_term(table)));
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
  sig["observe"] = fn(Nat, SearchState);
  RuleIndex index(rules);
  const auto type = infer({}, app(sym("observe"), nat(0)), sig, index);
  assert(type && conv(*type, SearchState, index));
  const auto expected_type = infer({}, app(sym("expected"), nat(0)), sig, index);
  assert(expected_type && conv(*expected_type, SearchState, index));
  const auto output_type = infer({}, app(sym("observeResult"), nat(0)), sig, index);
  assert(output_type && conv(*output_type, EntityResult, index));
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
  std::println("Blink HTMLEntitySearch model: loaded {} CSV entries; built typed advance relation over range/length/last-match state", table.size());
  std::println("The source transition is represented over the full pinned table; {} finite prefixes are wired as observers", prefixes.size());
  std::println("Scope: models HTMLEntitySearch::Advance and constructor state, not ConsumeNamedEntity rollback, segmented input, or Unicode decoding.");
}
