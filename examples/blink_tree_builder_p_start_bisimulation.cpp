#include "../src/lambda_pi.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <string_view>
#include <vector>

using namespace lp;

enum class Tag {
    P, Address, Article, Aside, Blockquote, Center, Details, Dialog, Dir, Div,
    Dl, Fieldset, Figcaption, Figure, Footer, Header, Hgroup, Main, Menu, Nav,
    Ol, Search, Section, Summary, Ul, Button, HtmlMarker, ForeignMarker,
    FragmentMarker, Body, Other
};
using OpenStack = std::vector<Tag>;  // top first

static constexpr std::array<std::string_view, 24> kSharedPClosingStartTags = {
    "address", "article", "aside", "blockquote", "center", "details",
    "dialog", "dir", "div", "dl", "fieldset", "figcaption", "figure",
    "footer", "header", "hgroup", "main", "menu", "nav", "ol", "search",
    "section", "summary", "ul"};
static constexpr std::array<Tag, 24> kSharedPClosingTagValues = {
    Tag::Address, Tag::Article, Tag::Aside, Tag::Blockquote, Tag::Center,
    Tag::Details, Tag::Dialog, Tag::Dir, Tag::Div, Tag::Dl, Tag::Fieldset,
    Tag::Figcaption, Tag::Figure, Tag::Footer, Tag::Header, Tag::Hgroup,
    Tag::Main, Tag::Menu, Tag::Nav, Tag::Ol, Tag::Search, Tag::Section,
    Tag::Summary, Tag::Ul};
static constexpr std::array<std::string_view, 31> kTagSymbols = {
    "pTag", "addressTag", "articleTag", "asideTag", "blockquoteTag",
    "centerTag", "detailsTag", "dialogTag", "dirTag", "divTag", "dlTag",
    "fieldsetTag", "figcaptionTag", "figureTag", "footerTag", "headerTag",
    "hgroupTag", "mainTag", "menuTag", "navTag", "olTag", "searchTag",
    "sectionTag", "summaryTag", "ulTag", "buttonTag", "htmlMarkerTag",
    "foreignMarkerTag", "fragmentMarkerTag", "bodyTag", "otherTag"};

static std::string_view tag_symbol(Tag tag) {
    return kTagSymbols[static_cast<size_t>(tag)];
}
static std::optional<Tag> classify_shared_start_tag(std::string_view name) {
    if (name == "p") return Tag::P;
    for (size_t i = 0; i < kSharedPClosingStartTags.size(); ++i)
        if (name == kSharedPClosingStartTags[i]) return kSharedPClosingTagValues[i];
    return std::nullopt;
}

// Extensional reference for the pinned HTMLElementStack::InButtonScope(kP)
// behavior. The finite model groups concrete markers into the HTML, foreign,
// document-fragment, and button categories used by IsButtonScopeMarker.
static bool cpp_scope_p(const OpenStack& stack) {
    for (Tag tag : stack) {
        if (tag == Tag::P) return true;
        if (tag == Tag::Button || tag == Tag::HtmlMarker ||
            tag == Tag::ForeignMarker || tag == Tag::FragmentMarker) return false;
    }
    return false;
}
static bool is_marker(Tag tag) {
    return tag == Tag::Button || tag == Tag::HtmlMarker ||
           tag == Tag::ForeignMarker || tag == Tag::FragmentMarker;
}
static bool cpp_cached_scope_p(const OpenStack& stack) {
    bool below = false;
    for (auto it = stack.rbegin(); it != stack.rend(); ++it)
        below = *it == Tag::P || (!is_marker(*it) && below);
    return below;
}
struct CachedItem { Tag tag; bool has_p_in_button_scope; };
static bool cpp_remove_non_top_p_cached_scope(const OpenStack& stack) {
    std::vector<CachedItem> bottom_to_top;
    bool below = false;
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        below = *it == Tag::P || (!is_marker(*it) && below);
        bottom_to_top.push_back({*it, below});
    }
    for (size_t top_first = 1; top_first < stack.size(); ++top_first) {
        if (stack[top_first] != Tag::P) continue;
        const size_t target = stack.size() - 1 - top_first;
        bottom_to_top.erase(bottom_to_top.begin() + static_cast<std::ptrdiff_t>(target));
        // UpdatePElementInButtonScope recomputes the affected items from the
        // removed position upward, leaving lower cached bits intact.
        for (size_t i = target; i < bottom_to_top.size(); ++i) {
            const bool lower = i ? bottom_to_top[i - 1].has_p_in_button_scope : false;
            const Tag tag = bottom_to_top[i].tag;
            bottom_to_top[i].has_p_in_button_scope = tag == Tag::P || (!is_marker(tag) && lower);
        }
        return bottom_to_top.empty() ? false : bottom_to_top.back().has_p_in_button_scope;
    }
    return bottom_to_top.empty() ? false : bottom_to_top.back().has_p_in_button_scope;
}
static bool cpp_close_then_insert(const OpenStack& stack) { return cpp_cached_scope_p(stack); }
static OpenStack cpp_remove_non_top_p(OpenStack stack) {
    for (size_t i = 1; i < stack.size(); ++i) {
        if (stack[i] == Tag::P) {
            stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(i));
            return stack;
        }
    }
    return stack;
}
static OpenStack cpp_start_tag_transition(OpenStack stack, Tag inserted) {
    if (cpp_scope_p(stack)) {
        for (size_t i = 0; i < stack.size(); ++i) {
            if (stack[i] == Tag::P) {
                stack.erase(stack.begin(), stack.begin() + static_cast<std::ptrdiff_t>(i + 1));
                break;
            }
        }
    }
    stack.insert(stack.begin(), inserted);
    return stack;
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr nat(size_t n) {
    Ptr result = sym("zero");
    while (n--) result = app(sym("succ"), result);
    return result;
}
static Ptr stack_term(const OpenStack& stack) {
    Ptr result = sym("emptyStack");
    for (auto it = stack.rbegin(); it != stack.rend(); ++it)
        result = apps(sym("pushTag"), sym(tag_symbol(*it).data()), result);
    return result;
}
static Ptr cached_stack_term(const OpenStack& stack) {
    Ptr result = sym("emptyCachedStack");
    for (auto it = stack.rbegin(); it != stack.rend(); ++it)
        result = apps(sym("pushCached"), sym(tag_symbol(*it).data()), result);
    return result;
}

int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto p = [](size_t i) { return pvar(i); };
    const Ptr bit = sym("Bool"), number = sym("Nat"), stack_type = sym("OpenStack");
    const Ptr state_type = sym("TreeState"), action = sym("Action");
    const Ptr yes = sym("true"), no = sym("false");
    const std::vector<OpenStack> stacks = {
        {Tag::P, Tag::Body, Tag::HtmlMarker},
        {Tag::Other, Tag::P, Tag::Body, Tag::HtmlMarker},
        {Tag::P, Tag::Button, Tag::Body, Tag::HtmlMarker},
        {Tag::Button, Tag::P, Tag::Body, Tag::HtmlMarker},
        {Tag::P, Tag::HtmlMarker, Tag::Body},
        {Tag::Other, Tag::Body, Tag::HtmlMarker},
        {Tag::P, Tag::Other, Tag::HtmlMarker},
        {Tag::P, Tag::ForeignMarker, Tag::Body},
        {Tag::ForeignMarker, Tag::P, Tag::Body, Tag::HtmlMarker},
        {Tag::P, Tag::FragmentMarker},
        {Tag::FragmentMarker, Tag::P},
    };
    struct InputCase { Tag token; size_t stack_index; };
    std::vector<InputCase> inputs;
    for (size_t i = 0; i < stacks.size(); ++i) inputs.push_back({Tag::P, i});
    // Exercise every named block token on one in-scope stack and one stack
    // where p is hidden below a scope marker.
    for (Tag token : kSharedPClosingTagValues) {
        inputs.push_back({token, 0});
        inputs.push_back({token, 3});
    }
    const size_t input_count = inputs.size();
    auto stack_index = [&](size_t id) { return inputs[id].stack_index; };
    auto token_for = [&](size_t id) { return inputs[id].token; };
    auto token_term = [&](size_t id) { return sym(tag_symbol(token_for(id)).data()); };

    sig["Bool"] = type_sort(); sig["true"] = bit; sig["false"] = bit;
    sig["Nat"] = type_sort(); sig["zero"] = number; sig["succ"] = fn(number, number);
    sig["Tag"] = type_sort();
    for (auto t : kTagSymbols) sig[std::string(t)] = sym("Tag");
    sig["OpenStack"] = type_sort(); sig["emptyStack"] = stack_type;
    sig["pushTag"] = fn(sym("Tag"), fn(stack_type, stack_type));
    sig["stackAt"] = fn(number, stack_type);
    sig["scopeP"] = fn(stack_type, bit);
    sig["dropThroughP"] = fn(stack_type, stack_type);
    sig["ifStack"] = fn(bit, fn(stack_type, fn(stack_type, stack_type)));
    sig["startTag"] = fn(sym("Tag"), fn(stack_type, stack_type));
    sig["tokenAt"] = fn(number, sym("Tag"));
    sig["cppResult"] = fn(state_type, stack_type);
    sig["semanticResult"] = fn(state_type, stack_type);
    sig["eqTag"] = fn(sym("Tag"), fn(sym("Tag"), bit));
    sig["eqStack"] = fn(stack_type, fn(stack_type, bit));
    sig["CachedStack"] = type_sort(); sig["emptyCachedStack"] = sym("CachedStack");
    sig["cachedCons"] = fn(sym("Tag"), fn(bit, fn(sym("CachedStack"), sym("CachedStack"))));
    sig["cacheOf"] = fn(sym("CachedStack"), bit);
    sig["pushCached"] = fn(sym("Tag"), fn(sym("CachedStack"), sym("CachedStack")));
    sig["cacheFor"] = fn(sym("Tag"), fn(bit, bit));
    sig["removePFromTail"] = fn(sym("CachedStack"), sym("CachedStack"));
    sig["removePNonTop"] = fn(sym("CachedStack"), sym("CachedStack"));
    sig["isP"] = fn(sym("Tag"), bit); sig["isMarker"] = fn(sym("Tag"), bit);
    sig["cachedStackAt"] = fn(number, sym("CachedStack"));
    sig["TreeState"] = type_sort(); sig["inputState"] = fn(number, state_type); sig["terminal"] = state_type;
    sig["Action"] = type_sort(); sig["insertP"] = action; sig["closeThenInsertP"] = action;
    sig["ifB"] = fn(bit, fn(action, fn(action, action)));
    sig["cppAction"] = fn(state_type, action); sig["semanticAction"] = fn(state_type, action);
    sig["cppNext"] = fn(state_type, state_type); sig["semanticNext"] = fn(state_type, state_type);
    sig["andB"] = fn(bit, fn(bit, bit)); sig["orB"] = fn(bit, fn(bit, bit));
    sig["notB"] = fn(bit, bit); sig["eqB"] = fn(bit, fn(bit, bit));
    sig["eqNat"] = fn(number, fn(number, bit)); sig["eqAction"] = fn(action, fn(action, bit));
    sig["eqState"] = fn(state_type, fn(state_type, bit));
    sig["PairList"] = type_sort(); sig["nilPair"] = sym("PairList");
    sig["pairCons"] = fn(state_type, fn(state_type, fn(sym("PairList"), sym("PairList"))));
    sig["memberPair"] = fn(sym("PairList"), fn(state_type, fn(state_type, bit)));
    sig["bisimCheck"] = fn(sym("PairList"), fn(state_type, bit));

    rules.push_back(rule(apps(sym("andB"), yes, yes), yes));
    rules.push_back(rule(apps(sym("andB"), yes, no), no));
    rules.push_back(rule(apps(sym("andB"), no, p(0)), no));
    rules.push_back(rule(apps(sym("orB"), yes, p(0)), yes));
    rules.push_back(rule(apps(sym("orB"), no, yes), yes));
    rules.push_back(rule(apps(sym("orB"), no, no), no));
    rules.push_back(rule(app(sym("notB"), yes), no));
    rules.push_back(rule(app(sym("notB"), no), yes));
    for (size_t i = 0; i < kTagSymbols.size(); ++i) {
        const Tag tag = static_cast<Tag>(i);
        Ptr constructor = sym(kTagSymbols[i].data());
        rules.push_back(rule(app(sym("isP"), constructor), tag == Tag::P ? yes : no));
        rules.push_back(rule(app(sym("isMarker"), constructor), is_marker(tag) ? yes : no));
    }
    rules.push_back(rule(app(sym("cacheOf"), sym("emptyCachedStack")), no));
    rules.push_back(rule(apps(sym("cacheOf"), apps(sym("cachedCons"), p(0), p(1), p(2))), p(1)));
    // Mirrors Push and UpdatePElementInButtonScope: a p sets the bit, a scope
    // marker clears it, and an ordinary element inherits the bit below.
    rules.push_back(rule(apps(sym("cacheFor"), p(0), p(1)),
        apps(sym("orB"), app(sym("isP"), p(0)),
             apps(sym("andB"), app(sym("notB"), app(sym("isMarker"), p(0))), p(1)))));
    rules.push_back(rule(apps(sym("pushCached"), p(0), p(1)),
        apps(sym("cachedCons"), p(0), apps(sym("cacheFor"), p(0), app(sym("cacheOf"), p(1))), p(1))));
    // Remove a target p below the current top and recompute cached bits for
    // intervening items, following RemoveNonTopCommon's affected-stack path.
    rules.push_back(rule(app(sym("removePFromTail"), sym("emptyCachedStack")), sym("emptyCachedStack")));
    rules.push_back(rule(apps(sym("removePFromTail"), apps(sym("cachedCons"), sym("pTag"), p(0), p(1))), p(1)));
    rules.push_back(rule(apps(sym("removePFromTail"), apps(sym("cachedCons"), p(0), p(1), p(2))),
        apps(sym("cachedCons"), p(0),
             apps(sym("cacheFor"), p(0), app(sym("cacheOf"), app(sym("removePFromTail"), p(2)))),
             app(sym("removePFromTail"), p(2)))));
    rules.push_back(rule(apps(sym("removePNonTop"), apps(sym("cachedCons"), p(0), p(1), p(2))),
        apps(sym("cachedCons"), p(0),
             apps(sym("cacheFor"), p(0), app(sym("cacheOf"), app(sym("removePFromTail"), p(2)))),
             app(sym("removePFromTail"), p(2)))));
    rules.push_back(rule(apps(sym("eqNat"), sym("zero"), sym("zero")), yes));
    rules.push_back(rule(apps(sym("eqNat"), sym("zero"), app(sym("succ"), p(0))), no));
    rules.push_back(rule(apps(sym("eqNat"), app(sym("succ"), p(0)), sym("zero")), no));
    rules.push_back(rule(apps(sym("eqNat"), app(sym("succ"), p(0)), app(sym("succ"), p(1))), apps(sym("eqNat"), p(0), p(1))));
    rules.push_back(rule(apps(sym("ifB"), yes, p(0), p(1)), p(0)));
    rules.push_back(rule(apps(sym("ifB"), no, p(0), p(1)), p(1)));
    rules.push_back(rule(apps(sym("eqAction"), sym("insertP"), sym("insertP")), yes));
    rules.push_back(rule(apps(sym("eqAction"), sym("closeThenInsertP"), sym("closeThenInsertP")), yes));
    rules.push_back(rule(apps(sym("eqAction"), sym("insertP"), sym("closeThenInsertP")), no));
    rules.push_back(rule(apps(sym("eqAction"), sym("closeThenInsertP"), sym("insertP")), no));
    rules.push_back(rule(apps(sym("eqB"), yes, yes), yes));
    rules.push_back(rule(apps(sym("eqB"), no, no), yes));
    rules.push_back(rule(apps(sym("eqB"), yes, no), no));
    rules.push_back(rule(apps(sym("eqB"), no, yes), no));
    rules.push_back(rule(apps(sym("ifStack"), yes, p(0), p(1)), p(0)));
    rules.push_back(rule(apps(sym("ifStack"), no, p(0), p(1)), p(1)));
    for (auto tag : kTagSymbols)
        rules.push_back(rule(apps(sym("eqTag"), sym(tag.data()), sym(tag.data())), yes));
    rules.push_back(rule(apps(sym("eqStack"), sym("emptyStack"), sym("emptyStack")), yes));
    rules.push_back(rule(apps(sym("eqStack"), sym("emptyStack"), apps(sym("pushTag"), p(0), p(1))), no));
    rules.push_back(rule(apps(sym("eqStack"), apps(sym("pushTag"), p(0), p(1)), sym("emptyStack")), no));
    rules.push_back(rule(apps(sym("eqStack"), apps(sym("pushTag"), p(0), p(1)),
                              apps(sym("pushTag"), p(2), p(3))),
        apps(sym("andB"), apps(sym("eqTag"), p(0), p(2)), apps(sym("eqStack"), p(1), p(3)))));
    rules.push_back(rule(app(sym("dropThroughP"), sym("emptyStack")), sym("emptyStack")));
    rules.push_back(rule(apps(sym("dropThroughP"), apps(sym("pushTag"), sym("pTag"), p(0))), p(0)));
    rules.push_back(rule(apps(sym("dropThroughP"), apps(sym("pushTag"), p(0), p(1))),
                         app(sym("dropThroughP"), p(1))));
    rules.push_back(rule(apps(sym("startTag"), p(0), p(1)),
        apps(sym("pushTag"), p(0),
             apps(sym("ifStack"), app(sym("scopeP"), p(1)), app(sym("dropThroughP"), p(1)), p(1)))));

    // InButtonScope(kP): return at p; fail at any button-scope marker;
    // otherwise scan lower. Marker constructors aggregate concrete names.
    rules.push_back(rule(app(sym("scopeP"), sym("emptyStack")), no));
    rules.push_back(rule(apps(sym("scopeP"), apps(sym("pushTag"), sym("pTag"), p(0))), yes));
    for (size_t i = 0; i < kTagSymbols.size(); ++i) {
        const Tag tag = static_cast<Tag>(i);
        Ptr constructor = sym(kTagSymbols[i].data());
        if (tag == Tag::P) continue;
        Ptr next = is_marker(tag) ? no : app(sym("scopeP"), p(0));
        rules.push_back(rule(apps(sym("scopeP"), apps(sym("pushTag"), constructor, p(0))), next));
    }
    for (size_t i = 0; i < input_count; ++i) {
        rules.push_back(rule(app(sym("stackAt"), nat(i)), stack_term(stacks[stack_index(i)])));
        rules.push_back(rule(app(sym("cachedStackAt"), nat(i)), cached_stack_term(stacks[stack_index(i)])));
        rules.push_back(rule(app(sym("tokenAt"), nat(i)), token_term(i)));
        rules.push_back(rule(app(sym("cppResult"), app(sym("inputState"), nat(i))),
                             stack_term(cpp_start_tag_transition(stacks[stack_index(i)], token_for(i)))));
    }
    rules.push_back(rule(app(sym("cppResult"), sym("terminal")), sym("emptyStack")));
    rules.push_back(rule(app(sym("semanticResult"), app(sym("inputState"), p(0))),
                         apps(sym("startTag"), app(sym("tokenAt"), p(0)), app(sym("stackAt"), p(0)))));
    rules.push_back(rule(app(sym("semanticResult"), sym("terminal")), sym("emptyStack")));

    // Native C++ observation is tabulated from its cached-bit recurrence;
    // λΠ derives it from the cached stack representation.
    for (size_t i = 0; i < input_count; ++i) {
        const char* result = cpp_close_then_insert(stacks[stack_index(i)]) ? "closeThenInsertP" : "insertP";
        rules.push_back(rule(app(sym("cppAction"), app(sym("inputState"), nat(i))), sym(result)));
    }
    rules.push_back(rule(app(sym("cppAction"), sym("terminal")), sym("insertP")));
    rules.push_back(rule(app(sym("semanticAction"), app(sym("inputState"), p(0))),
        apps(sym("ifB"), app(sym("cacheOf"), app(sym("cachedStackAt"), p(0))), sym("closeThenInsertP"), sym("insertP"))));
    rules.push_back(rule(app(sym("semanticAction"), sym("terminal")), sym("insertP")));
    rules.push_back(rule(app(sym("cppNext"), p(0)), sym("terminal")));
    rules.push_back(rule(app(sym("semanticNext"), p(0)), sym("terminal")));

    rules.push_back(rule(apps(sym("eqState"), sym("terminal"), sym("terminal")), yes));
    rules.push_back(rule(apps(sym("eqState"), sym("terminal"), app(sym("inputState"), p(0))), no));
    rules.push_back(rule(apps(sym("eqState"), app(sym("inputState"), p(0)), sym("terminal")), no));
    rules.push_back(rule(apps(sym("eqState"), app(sym("inputState"), p(0)), app(sym("inputState"), p(1))), apps(sym("eqNat"), p(0), p(1))));
    rules.push_back(rule(apps(sym("memberPair"), p(0), sym("terminal"), sym("terminal")), yes));
    rules.push_back(rule(apps(sym("memberPair"), sym("nilPair"), p(0), p(1)), no));
    rules.push_back(rule(apps(sym("memberPair"), apps(sym("pairCons"), p(0), p(1), p(2)), p(3), p(4)),
        apps(sym("orB"), apps(sym("andB"), apps(sym("eqState"), p(0), p(3)), apps(sym("eqState"), p(1), p(4))), apps(sym("memberPair"), p(2), p(3), p(4)))));
    rules.push_back(rule(apps(sym("bisimCheck"), p(0), p(1)),
        apps(sym("andB"), apps(sym("eqAction"), app(sym("cppAction"), p(1)), app(sym("semanticAction"), p(1))),
             apps(sym("memberPair"), p(0), app(sym("cppNext"), p(1)), app(sym("semanticNext"), p(1))))));

    Ptr relation = apps(sym("pairCons"), sym("terminal"), sym("terminal"), sym("nilPair"));
    for (size_t i = 0; i < input_count; ++i) {
        Ptr q = app(sym("inputState"), nat(i));
        relation = apps(sym("pairCons"), q, q, relation);
    }
    auto check_true = [&](Ptr obligation) {
        auto obligation_type = infer({}, obligation, sig, rules);
        assert(obligation_type && conv(*obligation_type, bit, rules));
        assert(conv(nf(obligation, rules), yes, rules));
    };
    check_true(apps(sym("bisimCheck"), relation, sym("terminal")));
    check_true(apps(sym("eqStack"), app(sym("cppResult"), sym("terminal")),
                                   app(sym("semanticResult"), sym("terminal"))));
    for (size_t i = 0; i < input_count; ++i) {
        Ptr closure = apps(sym("bisimCheck"), relation, app(sym("inputState"), nat(i)));
        Ptr cache_invariant = apps(sym("eqB"), app(sym("scopeP"), app(sym("stackAt"), nat(i))),
                                      app(sym("cacheOf"), app(sym("cachedStackAt"), nat(i))));
        Ptr result_equal = apps(sym("eqStack"), app(sym("cppResult"), app(sym("inputState"), nat(i))),
                                          app(sym("semanticResult"), app(sym("inputState"), nat(i))));
        check_true(closure);
        check_true(cache_invariant);
        check_true(result_equal);
    }
    const OpenStack before_remove = {Tag::Other, Tag::P, Tag::Body, Tag::HtmlMarker};
    const OpenStack after_remove = cpp_remove_non_top_p(before_remove);
    Ptr removed_cache = app(sym("removePNonTop"), cached_stack_term(before_remove));
    Ptr mutation_cache_agrees = apps(sym("eqB"), app(sym("cacheOf"), removed_cache),
                                            app(sym("scopeP"), stack_term(after_remove)));
    Ptr removed_p_is_out_of_scope = apps(sym("eqB"), app(sym("cacheOf"), removed_cache), no);
    check_true(mutation_cache_agrees);
    check_true(removed_p_is_out_of_scope);
    for (size_t i = 0; i < input_count; ++i) {
        const auto& input_stack = stacks[stack_index(i)];
        const bool scope = cpp_scope_p(input_stack);
        assert(cpp_cached_scope_p(input_stack) == scope);
        Ptr expected = sym(scope ? "closeThenInsertP" : "insertP");
        assert(conv(nf(app(sym("semanticAction"), app(sym("inputState"), nat(i))), rules), expected, rules));
    }
    assert(after_remove == OpenStack({Tag::Other, Tag::Body, Tag::HtmlMarker}));
    assert(!cpp_cached_scope_p(after_remove) && !cpp_scope_p(after_remove));
    assert(classify_shared_start_tag("p") == Tag::P);
    for (size_t i = 0; i < kSharedPClosingStartTags.size(); ++i)
        assert(classify_shared_start_tag(kSharedPClosingStartTags[i]) == kSharedPClosingTagValues[i]);
    assert(!classify_shared_start_tag("span"));
    for (size_t i = 0; i < input_count; ++i) {
        const auto& input_stack = stacks[stack_index(i)];
        const Tag expected_top = token_for(i);
        assert(cpp_start_tag_transition(input_stack, token_for(i)).front() == expected_top);
    }
    assert(!cpp_remove_non_top_p_cached_scope(before_remove));
    std::println("HTML in-body start-tag coalgebra: {} token/stack states plus terminal; λΠ closure check reduces to true", input_count);
    std::println("C++ cached-bit scan and λΠ stack semantics agree across {} token/stack states with p and button/HTML/foreign/fragment markers", input_count);
    std::println("Non-top p removal recomputes the affected cached scope bit and agrees with extensional lookup");
    std::println("C++ and λΠ transitions preserve each distinct start-tag identity in the resulting open-element stacks");
    std::println("Scope: 24 exact block-start names and grouped scope markers; only p removal is modeled, not arbitrary stack mutations or DOM effects");
}
