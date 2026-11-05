#pragma once

#include "../src/lambda_pi.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace blink_entity_semantics {
using namespace lp;

struct Entity { std::string name; unsigned first_value = 0; std::optional<unsigned> second_value; bool semicolon = false; };
inline std::vector<Entity> load_entities() {
  std::ifstream input("vendor/chromium-src/third_party/blink/renderer/core/html/parser/html_entity_names.csv");
  assert(input && "run from repository root: pinned Blink entity CSV is required");
  std::vector<Entity> result;
  std::string line;
  while (std::getline(input, line)) {
    const auto comma = line.find(',');
    assert(comma != std::string::npos);
    Entity entity;
    entity.name = line.substr(1, comma - 2);
    entity.semicolon = !entity.name.empty() && entity.name.back() == ';';
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

inline Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
inline Rule rule(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }
inline Ptr p(int i) { return pvar(i); }
inline Ptr nat(unsigned n) { Ptr r = sym("z"); while (n--) r = app(sym("s"), r); return r; }
inline Ptr ch(unsigned char c) { return sym("ch" + std::to_string(c)); }
inline Ptr unit(unsigned u) { return sym("u16_" + std::to_string(u)); }
inline Ptr chars(const std::string& s) {
  Ptr r = sym("nilChar");
  for (auto i = s.rbegin(); i != s.rend(); ++i) r = apps(sym("consChar"), ch(static_cast<unsigned char>(*i)), r);
  return r;
}
inline std::vector<unsigned> utf16_units(unsigned codepoint) {
  if (codepoint <= 0xFFFF) return {codepoint};
  codepoint -= 0x10000;
  return {0xD800 + (codepoint >> 10), 0xDC00 + (codepoint & 0x3FF)};
}
template <class Table>
inline Ptr entity_table_term(const Table& table) {
  Ptr r = sym("nilEntities");
  for (auto i = table.rbegin(); i != table.rend(); ++i)
    r = apps(sym("consEntity"), apps(sym("entity"), chars(i->name), nat(static_cast<unsigned>(i->name.size())), sym("cp_" + std::to_string(i->first_value)), i->second_value ? app(sym("someSecond"), sym("cp_" + std::to_string(*i->second_value))) : sym("noSecond"), sym(i->semicolon ? "true" : "false")), r);
  return r;
}
inline bool ascii_alnum(unsigned char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
template <class Table>
inline void installNamedEntitySemantics(Signature& sig, std::vector<Rule>& rules, const Table& table) {
  const Ptr Nat = sym("Nat"), Bool = sym("Bool"), Char = sym("Char"), Chars = sym("Chars");
  const Ptr Entity = sym("Entity"), Entities = sym("Entities"), MaybeChar = sym("MaybeChar");
  const Ptr Match = sym("Match"), SearchState = sym("SearchState");
  const Ptr Codepoint = sym("Codepoint"), OptionCp = sym("OptionCp"), EntityResult = sym("EntityResult");
  const Ptr ConsumeResult = sym("ConsumeResult");
  const Ptr CodeUnit = sym("CodeUnit"), Units = sym("Units"), Token = sym("Token");
  const Ptr ReturnState = sym("ReturnState"), Status = sym("Status"), TokenizerState = sym("TokenizerState"), TokenizerOutcome = sym("TokenizerOutcome");
  sig["Nat"] = type_sort(); sig["z"] = Nat; sig["s"] = fn(Nat, Nat);
  sig["Bool"] = type_sort(); sig["true"] = Bool; sig["false"] = Bool;
  sig["Char"] = type_sort(); sig["nilChar"] = Chars; sig["consChar"] = fn(Char, fn(Chars, Chars));
  for (unsigned i = 0; i < 128; ++i) sig["ch" + std::to_string(i)] = Char;
  sig["Chars"] = type_sort();
  sig["Codepoint"] = type_sort(); sig["OptionCp"] = type_sort(); sig["noSecond"] = OptionCp; sig["someSecond"] = fn(Codepoint, OptionCp);
  sig["CodeUnit"] = type_sort(); sig["Units"] = type_sort(); sig["nilUnits"] = Units; sig["consUnit"] = fn(CodeUnit, fn(Units, Units));
  auto register_units = [&](unsigned cp) { for (unsigned value : utf16_units(cp)) sig["u16_" + std::to_string(value)] = CodeUnit; };
  register_units(0x26);
  for (const auto& item : table) {
    sig["cp_" + std::to_string(item.first_value)] = Codepoint;
    register_units(item.first_value);
    if (item.second_value) sig["cp_" + std::to_string(*item.second_value)] = Codepoint;
    if (item.second_value) register_units(*item.second_value);
  }
  sig["Entity"] = type_sort(); sig["entity"] = fn(Chars, fn(Nat, fn(Codepoint, fn(OptionCp, fn(Bool, Entity)))));
  sig["Entities"] = type_sort(); sig["nilEntities"] = Entities; sig["consEntity"] = fn(Entity, fn(Entities, Entities));
  sig["MaybeChar"] = type_sort(); sig["noChar"] = MaybeChar; sig["someChar"] = fn(Char, MaybeChar);
  sig["Match"] = type_sort(); sig["noMatch"] = Match; sig["someMatch"] = fn(Entity, Match);
  sig["EntityResult"] = type_sort(); sig["noEntity"] = EntityResult; sig["decodedEntity"] = fn(Codepoint, fn(OptionCp, EntityResult));
  sig["SearchState"] = type_sort(); sig["searchState"] = fn(Nat, fn(Entities, fn(Match, SearchState)));
  sig["entityName"] = fn(Entity, Chars); sig["entityLength"] = fn(Entity, Nat); sig["entitySemi"] = fn(Entity, Bool); sig["searchResult"] = fn(SearchState, EntityResult); sig["resultOfMatch"] = fn(Match, EntityResult);
  sig["observeResult"] = fn(Nat, EntityResult); sig["expectedResult"] = fn(Nat, EntityResult);
  sig["takeChars"] = fn(Chars, fn(Nat, Chars)); sig["dropChars"] = fn(Chars, fn(Nat, Chars));
  sig["lastConsumed"] = fn(Chars, fn(Nat, MaybeChar)); sig["needsMore"] = fn(Bool, fn(Nat, fn(Chars, Bool)));
  sig["isAlnum"] = fn(Char, Bool); sig["orBool"] = fn(Bool, fn(Bool, Bool)); sig["notBool"] = fn(Bool, Bool);
  sig["guardAllows"] = fn(Bool, fn(Bool, fn(MaybeChar, Bool)));
  sig["consumeNamed"] = fn(SearchState, fn(Chars, fn(Bool, fn(Bool, ConsumeResult))));
  sig["consumeAfterNeed"] = fn(Bool, fn(Match, fn(Chars, fn(Nat, fn(Bool, ConsumeResult)))));
  sig["consumeKnown"] = fn(Entity, fn(Chars, fn(Bool, ConsumeResult)));
  sig["consumeKnownStep"] = fn(Bool, fn(Codepoint, fn(OptionCp, fn(Nat, fn(Chars, ConsumeResult)))));
  sig["consumeAt"] = fn(Nat, ConsumeResult); sig["expectedConsume"] = fn(Nat, ConsumeResult);
  sig["consumeInputAt"] = fn(Nat, Chars); sig["consumeEofAt"] = fn(Nat, Bool); sig["consumeAdditionalAt"] = fn(Nat, Bool);
  sig["consumeResult"] = type_sort(); sig["consumeNeedMore"] = fn(Chars, ConsumeResult);
  sig["consumeFailure"] = fn(Chars, ConsumeResult); sig["consumeGuardReject"] = fn(Chars, ConsumeResult);
  sig["consumeDecoded"] = fn(Codepoint, fn(OptionCp, fn(Nat, fn(Chars, ConsumeResult))));
  sig["Token"] = type_sort(); sig["token"] = fn(Units, fn(Bool, fn(Bool, Token)));
  sig["ReturnState"] = type_sort(); sig["data"] = ReturnState; sig["rcdata"] = ReturnState;
  sig["TokenizerState"] = type_sort(); sig["dataState"] = TokenizerState; sig["rcdataState"] = TokenizerState;
  sig["charRefDataState"] = TokenizerState; sig["charRefRCDATAState"] = TokenizerState;
  sig["Status"] = type_sort(); sig["resume"] = Status; sig["suspend"] = Status;
  sig["TokenizerOutcome"] = type_sort(); sig["tokenizerOutcome"] = fn(Status, fn(TokenizerState, fn(Bool, fn(Token, TokenizerOutcome))));
  sig["resumedState"] = fn(ReturnState, TokenizerState); sig["suspendedState"] = fn(ReturnState, TokenizerState);
  sig["codepointUnits"] = fn(Codepoint, Units); sig["appendUnits"] = fn(Units, fn(Units, Units));
  sig["decodedUnits"] = fn(Codepoint, fn(OptionCp, Units));
  sig["processConsume"] = fn(ConsumeResult, fn(ReturnState, fn(Bool, fn(Token, TokenizerOutcome))));
  sig["processAt"] = fn(Nat, TokenizerOutcome); sig["expectedProcess"] = fn(Nat, TokenizerOutcome);
  sig["returnAt"] = fn(Nat, ReturnState); sig["tokenAt"] = fn(Nat, Token); sig["characterTokenAt"] = fn(Nat, Bool);
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
  rules.push_back(rule(app(sym("entityName"), apps(sym("entity"), p(0), p(1), p(2), p(3), p(4))), p(0)));
  rules.push_back(rule(app(sym("entityLength"), apps(sym("entity"), p(0), p(1), p(2), p(3), p(4))), p(1)));
  rules.push_back(rule(app(sym("entitySemi"), apps(sym("entity"), p(0), p(1), p(2), p(3), p(4))), p(4)));
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
  rules.push_back(rule(app(sym("resultOfMatch"), app(sym("someMatch"), apps(sym("entity"), p(0), p(1), p(2), p(3), p(4)))), apps(sym("decodedEntity"), p(2), p(3))));
  rules.push_back(rule(app(sym("searchResult"), apps(sym("searchState"), p(0), p(1), p(2))), app(sym("resultOfMatch"), p(2))));

  rules.push_back(rule(apps(sym("takeChars"), sym("nilChar"), p(0)), sym("nilChar")));
  rules.push_back(rule(apps(sym("takeChars"), apps(sym("consChar"), p(0), p(1)), sym("z")), sym("nilChar")));
  rules.push_back(rule(apps(sym("takeChars"), apps(sym("consChar"), p(0), p(1)), app(sym("s"), p(2))), apps(sym("consChar"), p(0), apps(sym("takeChars"), p(1), p(2)))));
  rules.push_back(rule(apps(sym("dropChars"), sym("nilChar"), p(0)), sym("nilChar")));
  rules.push_back(rule(apps(sym("dropChars"), p(0), sym("z")), p(0)));
  rules.push_back(rule(apps(sym("dropChars"), apps(sym("consChar"), p(0), p(1)), app(sym("s"), p(2))), apps(sym("dropChars"), p(1), p(2))));
  rules.push_back(rule(apps(sym("lastConsumed"), p(0), sym("z")), sym("noChar")));
  rules.push_back(rule(apps(sym("lastConsumed"), p(0), app(sym("s"), p(1))), apps(sym("charAt"), p(0), p(1))));
  rules.push_back(rule(apps(sym("needsMore"), no, p(0), p(1)), no));
  rules.push_back(rule(apps(sym("needsMore"), yes, sym("z"), p(0)), no));
  rules.push_back(rule(apps(sym("needsMore"), yes, app(sym("s"), p(0)), p(1)), app(sym("notSemicolon"), apps(sym("lastConsumed"), p(1), app(sym("s"), p(0))))));
  sig["notSemicolon"] = fn(MaybeChar, Bool);
  rules.push_back(rule(app(sym("notSemicolon"), sym("noChar")), yes));
  rules.push_back(rule(app(sym("notSemicolon"), apps(sym("someChar"), ch(';'))), no));
  rules.push_back(rule(app(sym("notSemicolon"), apps(sym("someChar"), p(0))), yes));
  rules.push_back(rule(apps(sym("orBool"), yes, p(0)), yes));
  rules.push_back(rule(apps(sym("orBool"), no, p(0)), p(0)));
  rules.push_back(rule(app(sym("notBool"), yes), no));
  rules.push_back(rule(app(sym("notBool"), no), yes));
  for (unsigned c = 0; c < 128; ++c) if (ascii_alnum(static_cast<unsigned char>(c)))
    rules.push_back(rule(app(sym("isAlnum"), ch(c)), yes));
  rules.push_back(rule(app(sym("isAlnum"), p(0)), no));
  rules.push_back(rule(apps(sym("guardAllows"), yes, p(0), p(1)), yes));
  rules.push_back(rule(apps(sym("guardAllows"), no, no, p(0)), yes));
  rules.push_back(rule(apps(sym("guardAllows"), no, yes, sym("noChar")), yes));
  rules.push_back(rule(apps(sym("guardAllows"), no, yes, apps(sym("someChar"), p(0))), apps(sym("notBool"), apps(sym("orBool"), app(sym("isAlnum"), p(0)), apps(sym("eqChar"), p(0), ch('='))))));
  rules.push_back(rule(apps(sym("consumeNamed"), apps(sym("searchState"), p(0), p(1), p(2)), p(3), p(4), p(5)),
      apps(sym("consumeAfterNeed"), apps(sym("needsMore"), p(4), p(0), p(3)), p(2), p(3), p(0), p(5))));
  rules.push_back(rule(apps(sym("consumeAfterNeed"), yes, p(0), p(1), p(2), p(3)), app(sym("consumeNeedMore"), p(1))));
  rules.push_back(rule(apps(sym("consumeAfterNeed"), no, sym("noMatch"), p(0), p(1), p(2)), app(sym("consumeFailure"), p(0))));
  rules.push_back(rule(apps(sym("consumeAfterNeed"), no, app(sym("someMatch"), p(0)), p(1), p(2), p(3)), apps(sym("consumeKnown"), p(0), p(1), p(3))));
  rules.push_back(rule(apps(sym("consumeKnown"), apps(sym("entity"), p(0), p(1), p(2), p(3), p(4)), p(5), p(6)),
      apps(sym("consumeKnownStep"), apps(sym("guardAllows"), p(4), p(6), apps(sym("charAt"), p(5), p(1))), p(2), p(3), p(1), p(5))));
  rules.push_back(rule(apps(sym("consumeKnownStep"), yes, p(0), p(1), p(2), p(3)), apps(sym("consumeDecoded"), p(0), p(1), p(2), apps(sym("dropChars"), p(3), p(2)))));
  rules.push_back(rule(apps(sym("consumeKnownStep"), no, p(0), p(1), p(2), p(3)), app(sym("consumeGuardReject"), p(3))));

  std::set<unsigned> known_codepoints;
  for (const auto& item : table) {
    known_codepoints.insert(item.first_value);
    if (item.second_value) known_codepoints.insert(*item.second_value);
  }
  for (unsigned cp : known_codepoints) {
    const auto values = utf16_units(cp);
    Ptr units = sym("nilUnits");
    for (auto i = values.rbegin(); i != values.rend(); ++i) units = apps(sym("consUnit"), unit(*i), units);
    rules.push_back(rule(app(sym("codepointUnits"), sym("cp_" + std::to_string(cp))), units));
  }
  rules.push_back(rule(apps(sym("appendUnits"), sym("nilUnits"), p(0)), p(0)));
  rules.push_back(rule(apps(sym("appendUnits"), apps(sym("consUnit"), p(0), p(1)), p(2)), apps(sym("consUnit"), p(0), apps(sym("appendUnits"), p(1), p(2)))));
  rules.push_back(rule(apps(sym("decodedUnits"), p(0), sym("noSecond")), app(sym("codepointUnits"), p(0))));
  rules.push_back(rule(apps(sym("decodedUnits"), p(0), app(sym("someSecond"), p(1))), apps(sym("appendUnits"), app(sym("codepointUnits"), p(0)), app(sym("codepointUnits"), p(1)))));
  rules.push_back(rule(app(sym("resumedState"), sym("data")), sym("dataState")));
  rules.push_back(rule(app(sym("resumedState"), sym("rcdata")), sym("rcdataState")));
  rules.push_back(rule(app(sym("suspendedState"), sym("data")), sym("charRefDataState")));
  rules.push_back(rule(app(sym("suspendedState"), sym("rcdata")), sym("charRefRCDATAState")));
  rules.push_back(rule(apps(sym("processConsume"), app(sym("consumeNeedMore"), p(0)), p(1), p(2), apps(sym("token"), p(3), p(4), p(5))), apps(sym("tokenizerOutcome"), sym("suspend"), app(sym("suspendedState"), p(1)), p(2), apps(sym("token"), p(3), p(4), p(5)))));
  rules.push_back(rule(apps(sym("processConsume"), app(sym("consumeFailure"), p(0)), p(1), p(2), apps(sym("token"), p(3), p(4), p(5))), apps(sym("tokenizerOutcome"), sym("resume"), app(sym("resumedState"), p(1)), no, apps(sym("token"), apps(sym("appendUnits"), p(3), apps(sym("consUnit"), unit(0x26), sym("nilUnits"))), p(4), yes))));
  rules.push_back(rule(apps(sym("processConsume"), app(sym("consumeGuardReject"), p(0)), p(1), p(2), apps(sym("token"), p(3), p(4), p(5))), apps(sym("tokenizerOutcome"), sym("resume"), app(sym("resumedState"), p(1)), no, apps(sym("token"), apps(sym("appendUnits"), p(3), apps(sym("consUnit"), unit(0x26), sym("nilUnits"))), p(4), yes))));
  rules.push_back(rule(apps(sym("processConsume"), apps(sym("consumeDecoded"), p(0), p(1), p(2), p(3)), p(4), p(5), apps(sym("token"), p(6), p(7), p(8))), apps(sym("tokenizerOutcome"), sym("resume"), app(sym("resumedState"), p(4)), no, apps(sym("token"), apps(sym("appendUnits"), p(6), apps(sym("decodedUnits"), p(0), p(1))), yes, yes))));

  sig["allEntities"] = Entities;
  rules.push_back(rule(sym("allEntities"), entity_table_term(table)));
}
}  // namespace blink_entity_semantics
