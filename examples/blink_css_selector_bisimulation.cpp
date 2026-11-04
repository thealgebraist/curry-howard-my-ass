#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <cassert>
#include <vector>

using namespace lp;

enum class SimpleKind { Tag, Id, Class };
struct SimpleSelector { SimpleKind kind; unsigned name; };
struct ElementView { unsigned tag; unsigned id; std::vector<unsigned> classes; };
struct SelectorView { std::vector<SimpleSelector> compound; };
struct MatchCase { SelectorView selector; ElementView element; };

// Native reference for the modeled EasySelectorChecker subset: simple tag,
// id, and class checks, conjunctively applied to one subject element.
static bool cpp_matches(const MatchCase& input) {
    for (const auto& selector : input.selector.compound) {
        bool matches = false;
        switch (selector.kind) {
            case SimpleKind::Tag: matches = selector.name == input.element.tag; break;
            case SimpleKind::Id: matches = input.element.id != 0 && selector.name == input.element.id; break;
            case SimpleKind::Class:
                matches = std::find(input.element.classes.begin(), input.element.classes.end(),
                                    selector.name) != input.element.classes.end();
                break;
        }
        if (!matches) return false;
    }
    return true;
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr nat(size_t n) {
    Ptr t = sym("zero");
    while (n--) t = app(sym("succ"), t);
    return t;
}
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
static Ptr selector_term(const SelectorView& selector) {
    Ptr components = sym("nilSelector");
    for (auto it = selector.compound.rbegin(); it != selector.compound.rend(); ++it)
        components = apps(sym("consSelector"), simple_term(*it), components);
    return app(sym("compoundSel"), components);
}
static Ptr element_term(const ElementView& element) {
    return apps(sym("element"), nat(element.tag), nat(element.id), name_list(element.classes));
}
static Ptr bool_term(bool value) { return sym(value ? "true" : "false"); }

int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto p = [](size_t i) { return pvar(i); };
    auto bit = sym("Bool"), number = sym("Nat"), element_type = sym("Element");
    auto selector_type = sym("Selector"), simple_type = sym("SimpleSelector");
    auto name_list_type = sym("NameList"), simple_list_type = sym("SimpleList");
    auto pairs = sym("PairList");
    auto yes = sym("true"), no = sym("false");

    sig["Bool"] = type_sort(); sig["true"] = bit; sig["false"] = bit;
    sig["Nat"] = type_sort(); sig["zero"] = number;
    sig["succ"] = fn(number, number);
    sig["NameList"] = type_sort(); sig["nilName"] = name_list_type;
    sig["consName"] = fn(number, fn(name_list_type, name_list_type));
    sig["Element"] = type_sort();
    sig["element"] = fn(number, fn(number, fn(name_list_type, element_type)));
    sig["SimpleSelector"] = type_sort();
    sig["tagSel"] = fn(number, simple_type); sig["idSel"] = fn(number, simple_type);
    sig["classSel"] = fn(number, simple_type);
    sig["SimpleList"] = type_sort(); sig["nilSelector"] = simple_list_type;
    sig["consSelector"] = fn(simple_type, fn(simple_list_type, simple_list_type));
    sig["Selector"] = type_sort();
    sig["simpleSel"] = fn(simple_type, selector_type);
    sig["compoundSel"] = fn(simple_list_type, selector_type);
    sig["eqNat"] = fn(number, fn(number, bit));
    sig["andB"] = fn(bit, fn(bit, bit)); sig["orB"] = fn(bit, fn(bit, bit));
    sig["memberName"] = fn(name_list_type, fn(number, bit));
    sig["matchesSimple"] = fn(simple_type, fn(element_type, bit));
    sig["matchesAll"] = fn(simple_list_type, fn(element_type, bit));
    sig["matchesSelector"] = fn(selector_type, fn(element_type, bit));
    sig["selectorAt"] = fn(number, selector_type);
    sig["elementAt"] = fn(number, element_type);
    sig["cppObserve"] = fn(number, bit); sig["semanticObserve"] = fn(number, bit);
    sig["cppNext"] = fn(number, number); sig["semanticNext"] = fn(number, number);
    sig["PairList"] = type_sort(); sig["nilPair"] = pairs;
    sig["pairCons"] = fn(number, fn(number, fn(pairs, pairs)));
    sig["memberPair"] = fn(pairs, fn(number, fn(number, bit)));
    sig["checkPairs"] = fn(pairs, fn(pairs, bit));
    sig["bisimCheck"] = fn(pairs, fn(number, fn(number, bit)));

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

    rules.push_back(rule(apps(sym("memberName"), sym("nilName"), p(0)), no));
    rules.push_back(rule(apps(sym("memberName"), apps(sym("consName"), p(0), p(1)), p(2)),
                         apps(sym("orB"), apps(sym("eqNat"), p(0), p(2)),
                              apps(sym("memberName"), p(1), p(2)))));
    rules.push_back(rule(apps(sym("matchesSimple"), app(sym("tagSel"), p(0)),
                              apps(sym("element"), p(1), p(2), p(3))),
                         apps(sym("eqNat"), p(0), p(1))));
    rules.push_back(rule(apps(sym("matchesSimple"), app(sym("idSel"), p(0)),
                              apps(sym("element"), p(1), sym("zero"), p(3))), no));
    rules.push_back(rule(apps(sym("matchesSimple"), app(sym("idSel"), p(0)),
                              apps(sym("element"), p(1), p(2), p(3))),
                         apps(sym("eqNat"), p(0), p(2))));
    rules.push_back(rule(apps(sym("matchesSimple"), app(sym("classSel"), p(0)),
                              apps(sym("element"), p(1), p(2), p(3))),
                         apps(sym("memberName"), p(3), p(0))));
    rules.push_back(rule(apps(sym("matchesAll"), sym("nilSelector"), p(0)), yes));
    rules.push_back(rule(apps(sym("matchesAll"), apps(sym("consSelector"), p(0), p(1)), p(2)),
                         apps(sym("andB"), apps(sym("matchesSimple"), p(0), p(2)),
                              apps(sym("matchesAll"), p(1), p(2)))));
    rules.push_back(rule(apps(sym("matchesSelector"), app(sym("simpleSel"), p(0)), p(1)),
                         apps(sym("matchesSimple"), p(0), p(1))));
    rules.push_back(rule(apps(sym("matchesSelector"), app(sym("compoundSel"), p(0)), p(1)),
                         apps(sym("matchesAll"), p(0), p(1))));
    rules.push_back(rule(app(sym("semanticObserve"), p(0)),
                         apps(sym("matchesSelector"), app(sym("selectorAt"), p(0)),
                              app(sym("elementAt"), p(0)))));

    const unsigned p_tag = 1, div_tag = 2, hero_id = 3, other_id = 4;
    const unsigned warm = 5, cool = 6, missing = 7;
    const SimpleSelector tag_p{SimpleKind::Tag, p_tag};
    const SimpleSelector id_hero{SimpleKind::Id, hero_id};
    const SimpleSelector id_other{SimpleKind::Id, other_id};
    const SimpleSelector class_warm{SimpleKind::Class, warm};
    const SimpleSelector class_cool{SimpleKind::Class, cool};
    const SimpleSelector class_missing{SimpleKind::Class, missing};
    const ElementView p_element{p_tag, hero_id, {warm, cool}};
    const ElementView div_element{div_tag, 0, {warm}};
    const std::vector<MatchCase> cases = {
        {{{tag_p}}, p_element},
        {{{tag_p}}, div_element},
        {{{id_hero}}, p_element},
        {{{id_other}}, p_element},
        {{{id_hero}}, div_element},
        {{{class_warm}}, p_element},
        {{{class_missing}}, p_element},
        {{{tag_p, id_hero, class_warm, class_cool}}, p_element},
        {{{tag_p, id_other, class_warm}}, p_element},
    };

    for (size_t i = 0; i < cases.size(); ++i) {
        Ptr case_id = nat(i);
        rules.push_back(rule(app(sym("selectorAt"), case_id), selector_term(cases[i].selector)));
        rules.push_back(rule(app(sym("elementAt"), case_id), element_term(cases[i].element)));
        rules.push_back(rule(app(sym("cppObserve"), case_id), bool_term(cpp_matches(cases[i]))));
    }
    Ptr terminal = nat(cases.size());
    rules.push_back(rule(app(sym("selectorAt"), terminal), app(sym("compoundSel"), sym("nilSelector"))));
    rules.push_back(rule(app(sym("elementAt"), terminal), element_term(p_element)));
    rules.push_back(rule(app(sym("cppObserve"), terminal), yes));
    rules.push_back(rule(app(sym("cppNext"), p(0)), terminal));
    rules.push_back(rule(app(sym("semanticNext"), p(0)), terminal));

    rules.push_back(rule(apps(sym("memberPair"), p(0), terminal, terminal), yes));
    rules.push_back(rule(apps(sym("memberPair"), sym("nilPair"), p(0), p(1)), no));
    rules.push_back(rule(apps(sym("memberPair"), apps(sym("pairCons"), p(0), p(1), p(2)), p(3), p(4)),
        apps(sym("orB"), apps(sym("andB"), apps(sym("eqNat"), p(0), p(3)),
                               apps(sym("eqNat"), p(1), p(4))),
             apps(sym("memberPair"), p(2), p(3), p(4)))));
    rules.push_back(rule(apps(sym("checkPairs"), p(0), sym("nilPair")), yes));
    rules.push_back(rule(apps(sym("checkPairs"), p(0), apps(sym("pairCons"), p(1), p(2), p(3))),
        apps(sym("andB"), apps(sym("eqB"), app(sym("cppObserve"), p(1)),
                                         app(sym("semanticObserve"), p(2))),
             apps(sym("andB"), apps(sym("memberPair"), p(0), app(sym("cppNext"), p(1)),
                                                       app(sym("semanticNext"), p(2))),
                  apps(sym("checkPairs"), p(0), p(3))))));
    rules.push_back(rule(apps(sym("bisimCheck"), p(0), p(1), p(2)),
        apps(sym("andB"), apps(sym("memberPair"), p(0), p(1), p(2)),
             apps(sym("checkPairs"), p(0), p(0)))));
    rules.push_back(rule(apps(sym("eqB"), yes, yes), yes));
    rules.push_back(rule(apps(sym("eqB"), no, no), yes));
    rules.push_back(rule(apps(sym("eqB"), yes, no), no));
    rules.push_back(rule(apps(sym("eqB"), no, yes), no));
    sig["eqB"] = fn(bit, fn(bit, bit));

    Ptr relation = sym("nilPair");
    for (int id = static_cast<int>(cases.size()); id >= 0; --id) {
        Ptr state_id = nat(static_cast<size_t>(id));
        relation = apps(sym("pairCons"), state_id, state_id, relation);
    }
    Ptr check = apps(sym("bisimCheck"), relation, nat(0), nat(0));
    auto check_type = infer({}, check, sig, rules);
    assert(check_type && conv(*check_type, bit, rules));
    Ptr result = nf(check, rules);
    assert(conv(result, yes, rules));
    for (size_t i = 0; i < cases.size(); ++i) {
        Ptr expected = bool_term(cpp_matches(cases[i]));
        assert(conv(nf(app(sym("semanticObserve"), nat(i)), rules), expected, rules));
    }

    std::println("CSS simple-selector coalgebra: {} related states; λΠ closure check reduces to true", cases.size() + 1);
    std::println("Native matching and λΠ semantics agree for tag, id, class, and compound selectors");
    std::println("Scope: one subject element; no parsing, combinators, attributes, pseudo-classes, or namespaces.");
}
