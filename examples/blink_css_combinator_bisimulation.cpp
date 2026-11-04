#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <cassert>
#include <vector>

using namespace lp;

enum class Relation { Child, Descendant };
enum class SimpleKind { Tag, Id, Class };
struct SimpleSelector { SimpleKind kind; unsigned name; };
using Compound = std::vector<SimpleSelector>;
struct SelectorGroup { Compound compound; Relation to_next; bool last; };
struct Node { unsigned tag; unsigned id; std::vector<unsigned> classes; unsigned parent; };
struct MatchCase { std::vector<SelectorGroup> right_to_left; unsigned subject; };

static bool match_simple(const SimpleSelector& selector, const Node& node) {
    switch (selector.kind) {
        case SimpleKind::Tag: return selector.name == node.tag;
        case SimpleKind::Id: return node.id != 0 && selector.name == node.id;
        case SimpleKind::Class:
            return std::find(node.classes.begin(), node.classes.end(), selector.name) != node.classes.end();
    }
    return false;
}

static bool match_compound(const Compound& compound, const Node& node) {
    return std::all_of(compound.begin(), compound.end(),
                       [&](const SimpleSelector& s) { return match_simple(s, node); });
}

// High-level C++ reference for compound complex selectors. Groups are stored
// from the subject toward ancestors, matching CSSSelector's right-to-left
// combinator order. Inputs obey EasySelectorChecker::IsEasy's child/descendant
// constraint; the reference uses the ordinary parent/ancestor interpretation.
static bool match_chain(const MatchCase& input,
                        const std::vector<Node>& tree,
                        size_t group_index,
                        unsigned node_id) {
    if (node_id == 0 || node_id >= tree.size() ||
        !match_compound(input.right_to_left[group_index].compound, tree[node_id])) {
        return false;
    }
    const auto& group = input.right_to_left[group_index];
    if (group.last) return true;
    const unsigned parent = tree[node_id].parent;
    if (group.to_next == Relation::Child)
        return match_chain(input, tree, group_index + 1, parent);
    for (unsigned ancestor = parent; ancestor != 0; ancestor = tree[ancestor].parent) {
        if (match_chain(input, tree, group_index + 1, ancestor)) return true;
    }
    return false;
}

static bool cpp_matches(const MatchCase& input, const std::vector<Node>& tree) {
    return match_chain(input, tree, 0, input.subject);
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr nat(size_t n) {
    Ptr t = sym("zero");
    while (n--) t = app(sym("succ"), t);
    return t;
}
static Ptr bool_term(bool value) { return sym(value ? "true" : "false"); }
static Ptr name_list(const std::vector<unsigned>& names) {
    Ptr result = sym("nilName");
    for (auto it = names.rbegin(); it != names.rend(); ++it)
        result = apps(sym("consName"), nat(*it), result);
    return result;
}
static Ptr simple_term(const SimpleSelector& selector) {
    const char* constructor = selector.kind == SimpleKind::Tag ? "tagSel" :
                              selector.kind == SimpleKind::Id ? "idSel" : "classSel";
    return app(sym(constructor), nat(selector.name));
}
static Ptr compound_term(const Compound& compound) {
    Ptr result = sym("nilSimple");
    for (auto it = compound.rbegin(); it != compound.rend(); ++it)
        result = apps(sym("consSimple"), simple_term(*it), result);
    return result;
}
static Ptr chain_term(const std::vector<SelectorGroup>& groups, size_t i = 0) {
    const auto& group = groups.at(i);
    if (group.last) return app(sym("lastGroup"), compound_term(group.compound));
    return apps(sym(group.to_next == Relation::Child ? "childGroup" : "descGroup"),
                compound_term(group.compound), chain_term(groups, i + 1));
}

int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto p = [](size_t i) { return pvar(i); };
    auto bit = sym("Bool"), number = sym("Nat"), group_list = sym("SelectorChain");
    auto compound_type = sym("Compound"), simple_type = sym("SimpleSelector");
    auto name_list_type = sym("NameList");
    auto yes = sym("true"), no = sym("false");

    sig["Bool"] = type_sort(); sig["true"] = bit; sig["false"] = bit;
    sig["Nat"] = type_sort(); sig["zero"] = number;
    sig["succ"] = fn(number, number);
    sig["SelectorChain"] = type_sort();
    sig["NameList"] = type_sort(); sig["nilName"] = name_list_type;
    sig["consName"] = fn(number, fn(name_list_type, name_list_type));
    sig["SimpleSelector"] = type_sort();
    sig["tagSel"] = fn(number, simple_type); sig["idSel"] = fn(number, simple_type);
    sig["classSel"] = fn(number, simple_type);
    sig["Compound"] = type_sort(); sig["nilSimple"] = compound_type;
    sig["consSimple"] = fn(simple_type, fn(compound_type, compound_type));
    sig["lastGroup"] = fn(compound_type, group_list);
    sig["childGroup"] = fn(compound_type, fn(group_list, group_list));
    sig["descGroup"] = fn(compound_type, fn(group_list, group_list));
    sig["eqNat"] = fn(number, fn(number, bit));
    sig["andB"] = fn(bit, fn(bit, bit)); sig["orB"] = fn(bit, fn(bit, bit));
    sig["parentOf"] = fn(number, number); sig["tagAt"] = fn(number, number);
    sig["idAt"] = fn(number, number); sig["classesAt"] = fn(number, fn(number, bit));
    sig["memberName"] = fn(name_list_type, fn(number, bit));
    sig["matchSimple"] = fn(simple_type, fn(number, bit));
    sig["matchCompound"] = fn(compound_type, fn(number, bit));
    sig["matchChain"] = fn(group_list, fn(number, bit));
    sig["matchChild"] = fn(group_list, fn(number, bit));
    sig["matchDescendant"] = fn(group_list, fn(number, bit));
    sig["selectorAt"] = fn(number, group_list);
    sig["subjectAt"] = fn(number, number);
    sig["cppObserve"] = fn(number, bit); sig["semanticObserve"] = fn(number, bit);
    sig["cppNext"] = fn(number, number); sig["semanticNext"] = fn(number, number);
    sig["eqB"] = fn(bit, fn(bit, bit));
    sig["PairList"] = type_sort(); sig["nilPair"] = sym("PairList");
    sig["pairCons"] = fn(number, fn(number, fn(sym("PairList"), sym("PairList"))));
    sig["memberPair"] = fn(sym("PairList"), fn(number, fn(number, bit)));

    rules.push_back(rule(apps(sym("eqNat"), sym("zero"), sym("zero")), yes));
    rules.push_back(rule(apps(sym("eqNat"), sym("zero"), app(sym("succ"), p(0))), no));
    rules.push_back(rule(apps(sym("eqNat"), app(sym("succ"), p(0)), sym("zero")), no));
    rules.push_back(rule(apps(sym("eqNat"), app(sym("succ"), p(0)), app(sym("succ"), p(1))),
                         apps(sym("eqNat"), p(0), p(1))));
    rules.push_back(rule(apps(sym("andB"), yes, yes), yes));
    rules.push_back(rule(apps(sym("andB"), yes, no), no));
    rules.push_back(rule(apps(sym("andB"), no, p(0)), no));
    rules.push_back(rule(apps(sym("orB"), yes, p(0)), yes));
    rules.push_back(rule(apps(sym("orB"), no, yes), yes));
    rules.push_back(rule(apps(sym("orB"), no, no), no));
    rules.push_back(rule(apps(sym("eqB"), yes, yes), yes));
    rules.push_back(rule(apps(sym("eqB"), no, no), yes));
    rules.push_back(rule(apps(sym("eqB"), yes, no), no));
    rules.push_back(rule(apps(sym("eqB"), no, yes), no));

    // Static finite DOM ancestry. Node 0 is the absent-parent sentinel.
    const std::vector<Node> tree = {
        {0, 0, {}, 0},                 // 0: sentinel
        {1, 0, {10}, 0},               // 1: div.root
        {2, 0, {11}, 1},               // 2: span.hot under div
        {3, 20, {12, 13}, 2},          // 3: p#hero.selected.text under span
        {1, 0, {10}, 0},               // 4: separate div.root
        {3, 20, {12}, 4},              // 5: p#hero.selected under div
        {2, 0, {14}, 4},               // 6: span.cool under div
        {3, 0, {12}, 6},               // 7: p.selected under span.cool
    };
    for (size_t i = 1; i < tree.size(); ++i) {
        rules.push_back(rule(app(sym("tagAt"), nat(i)), nat(tree[i].tag)));
        rules.push_back(rule(app(sym("idAt"), nat(i)), nat(tree[i].id)));
        rules.push_back(rule(app(sym("parentOf"), nat(i)), nat(tree[i].parent)));
        rules.push_back(rule(apps(sym("classesAt"), nat(i), p(0)),
            apps(sym("memberName"), name_list(tree[i].classes), p(0))));
    }
    rules.push_back(rule(app(sym("tagAt"), sym("zero")), sym("zero")));
    rules.push_back(rule(app(sym("idAt"), sym("zero")), sym("zero")));
    rules.push_back(rule(app(sym("parentOf"), sym("zero")), sym("zero")));
    rules.push_back(rule(apps(sym("classesAt"), sym("zero"), p(0)), no));

    rules.push_back(rule(apps(sym("memberName"), sym("nilName"), p(0)), no));
    rules.push_back(rule(apps(sym("memberName"), apps(sym("consName"), p(0), p(1)), p(2)),
        apps(sym("orB"), apps(sym("eqNat"), p(0), p(2)), apps(sym("memberName"), p(1), p(2)))));
    rules.push_back(rule(apps(sym("matchSimple"), app(sym("tagSel"), p(0)), p(1)),
                         apps(sym("eqNat"), p(0), app(sym("tagAt"), p(1)))));
    // All modeled CSS identifiers are nonzero, so direct equality rejects the
    // no-ID sentinel (zero).
    rules.push_back(rule(apps(sym("matchSimple"), app(sym("idSel"), p(0)), p(1)),
                         apps(sym("eqNat"), p(0), app(sym("idAt"), p(1)))));
    rules.push_back(rule(apps(sym("matchSimple"), app(sym("classSel"), p(0)), p(1)),
                         apps(sym("classesAt"), p(1), p(0))));
    rules.push_back(rule(apps(sym("matchCompound"), sym("nilSimple"), p(0)), yes));
    rules.push_back(rule(apps(sym("matchCompound"), apps(sym("consSimple"), p(0), p(1)), p(2)),
        apps(sym("andB"), apps(sym("matchSimple"), p(0), p(2)),
             apps(sym("matchCompound"), p(1), p(2)))));

    rules.push_back(rule(apps(sym("matchChain"), app(sym("lastGroup"), p(0)), p(1)),
                         apps(sym("matchCompound"), p(0), p(1))));
    rules.push_back(rule(apps(sym("matchChain"), apps(sym("childGroup"), p(0), p(1)), p(2)),
        apps(sym("andB"), apps(sym("matchCompound"), p(0), p(2)),
             apps(sym("matchChild"), p(1), app(sym("parentOf"), p(2))))));
    rules.push_back(rule(apps(sym("matchChain"), apps(sym("descGroup"), p(0), p(1)), p(2)),
        apps(sym("andB"), apps(sym("matchCompound"), p(0), p(2)),
             apps(sym("matchDescendant"), p(1), app(sym("parentOf"), p(2))))));
    rules.push_back(rule(apps(sym("matchChild"), p(0), sym("zero")), no));
    rules.push_back(rule(apps(sym("matchChild"), p(0), p(1)), apps(sym("matchChain"), p(0), p(1))));
    rules.push_back(rule(apps(sym("matchDescendant"), p(0), sym("zero")), no));
    rules.push_back(rule(apps(sym("matchDescendant"), p(0), p(1)),
        apps(sym("orB"), apps(sym("matchChain"), p(0), p(1)),
             apps(sym("matchDescendant"), p(0), app(sym("parentOf"), p(1))))));
    rules.push_back(rule(app(sym("semanticObserve"), p(0)),
        apps(sym("matchChain"), app(sym("selectorAt"), p(0)), app(sym("subjectAt"), p(0)))));

    const unsigned div_tag = 1, span_tag = 2, p_tag = 3;
    const unsigned hero_id = 20, root_class = 10, hot_class = 11;
    const unsigned selected_class = 12, cool_class = 14;
    auto tag = [](unsigned name) { return SimpleSelector{SimpleKind::Tag, name}; };
    auto id = [](unsigned name) { return SimpleSelector{SimpleKind::Id, name}; };
    auto cls = [](unsigned name) { return SimpleSelector{SimpleKind::Class, name}; };
    auto compound = [](std::initializer_list<SimpleSelector> xs) { return Compound(xs); };
    const std::vector<MatchCase> cases = {
        {{{compound({tag(span_tag), cls(hot_class)}), Relation::Child, false},
          {compound({tag(div_tag), cls(root_class)}), Relation::Child, true}}, 2},
        {{{compound({tag(span_tag), cls(hot_class)}), Relation::Child, false},
          {compound({tag(div_tag), cls(root_class)}), Relation::Child, true}}, 6},
        {{{compound({tag(p_tag), id(hero_id)}), Relation::Descendant, false},
          {compound({tag(div_tag), cls(root_class)}), Relation::Child, true}}, 3},
        {{{compound({tag(p_tag), cls(selected_class)}), Relation::Descendant, false},
          {compound({tag(div_tag), cls(root_class)}), Relation::Child, true}}, 3},
        {{{compound({tag(p_tag), id(hero_id), cls(selected_class)}), Relation::Child, false},
          {compound({tag(span_tag), cls(hot_class)}), Relation::Descendant, false},
          {compound({tag(div_tag), cls(root_class)}), Relation::Child, true}}, 3},
        {{{compound({tag(p_tag), id(hero_id), cls(selected_class)}), Relation::Child, false},
          {compound({tag(span_tag), cls(cool_class)}), Relation::Descendant, false},
          {compound({tag(div_tag), cls(root_class)}), Relation::Child, true}}, 7},
        {{{compound({tag(p_tag), id(hero_id)}), Relation::Child, false},
          {compound({tag(div_tag), cls(root_class)}), Relation::Child, true}}, 5},
        {{{compound({tag(p_tag), id(hero_id)}), Relation::Descendant, false},
          {compound({tag(div_tag), cls(root_class)}), Relation::Child, true}}, 7},
        {{{compound({tag(span_tag), cls(hot_class)}), Relation::Descendant, false},
          {compound({tag(div_tag), cls(root_class)}), Relation::Child, true}}, 5},
    };

    for (size_t i = 0; i < cases.size(); ++i) {
        Ptr id = nat(i);
        rules.push_back(rule(app(sym("selectorAt"), id), chain_term(cases[i].right_to_left)));
        rules.push_back(rule(app(sym("subjectAt"), id), nat(cases[i].subject)));
        rules.push_back(rule(app(sym("cppObserve"), id), bool_term(cpp_matches(cases[i], tree))));
    }
    Ptr terminal = nat(cases.size());
    rules.push_back(rule(app(sym("selectorAt"), terminal), app(sym("lastGroup"), sym("nilSimple"))));
    rules.push_back(rule(app(sym("subjectAt"), terminal), nat(0)));
    rules.push_back(rule(app(sym("cppObserve"), terminal), yes));
    rules.push_back(rule(app(sym("parentOf"), p(0)), sym("zero")));
    rules.push_back(rule(app(sym("tagAt"), p(0)), sym("zero")));

    rules.push_back(rule(app(sym("cppNext"), p(0)), terminal));
    rules.push_back(rule(app(sym("semanticNext"), p(0)), terminal));

    rules.push_back(rule(apps(sym("memberPair"), p(0), terminal, terminal), yes));
    rules.push_back(rule(apps(sym("memberPair"), sym("nilPair"), p(0), p(1)), no));
    rules.push_back(rule(apps(sym("memberPair"), apps(sym("pairCons"), p(0), p(1), p(2)), p(3), p(4)),
        apps(sym("orB"), apps(sym("andB"), apps(sym("eqNat"), p(0), p(3)),
                               apps(sym("eqNat"), p(1), p(4))),
             apps(sym("memberPair"), p(2), p(3), p(4)))));
    Ptr relation = sym("nilPair");
    std::vector<Ptr> states;
    for (int id = static_cast<int>(cases.size()); id >= 0; --id) {
        Ptr state_id = nat(static_cast<size_t>(id));
        states.push_back(state_id);
        relation = apps(sym("pairCons"), state_id, state_id, relation);
    }
    std::reverse(states.begin(), states.end());
    Ptr check = apps(sym("memberPair"), relation, nat(0), nat(0));
    for (const auto& state_id : states) {
        Ptr observations_equal = apps(sym("eqB"), app(sym("cppObserve"), state_id),
                                       app(sym("semanticObserve"), state_id));
        Ptr successors_related = apps(sym("memberPair"), relation,
                                      app(sym("cppNext"), state_id),
                                      app(sym("semanticNext"), state_id));
        Ptr pair_closed = apps(sym("andB"), observations_equal, successors_related);
        check = apps(sym("andB"), pair_closed, check);
    }
    auto check_type = infer({}, check, sig, rules);
    assert(check_type && conv(*check_type, bit, rules));
    assert(conv(nf(check, rules), yes, rules));
    for (size_t i = 0; i < cases.size(); ++i) {
        const bool expected = cpp_matches(cases[i], tree);
        assert(conv(nf(app(sym("semanticObserve"), nat(i)), rules), bool_term(expected), rules));
    }

    std::println("CSS child/descendant coalgebra: {} related states; λΠ closure check reduces to true", cases.size() + 1);
    std::println("Native tree matching and λΠ semantics agree on {} selector/tree cases", cases.size());
    std::println("Scope: tag/id/class compounds, child and descendant relations, finite trees; no sibling or shadow relations.");
}
