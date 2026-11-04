#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <vector>

using namespace lp;

// The enum keeps ordinary HTML tags, HTML scope markers, non-HTML scope
// markers, and document-fragment roots distinct at the abstraction boundary.
enum class Tag {
    Li, Dd, Dt, Option, Optgroup, P, Rb, Rp, Rt, Rtc, Other, Button, Ol, Ul,
    HtmlMarker, ForeignMarker, FragmentMarker
};
using Stack = std::vector<Tag>; // top first
static constexpr std::array<const char*, 17> kTagNames={
    "liTag","ddTag","dtTag","optionTag","optgroupTag","pTag","rbTag",
    "rpTag","rtTag","rtcTag","otherTag","buttonTag","olTag","ulTag",
    "htmlMarker","foreignMarker","fragmentMarker"};
static const char* tag_name(Tag t) { return kTagNames[static_cast<size_t>(t)]; }

// IsListItemScopeMarker from html_element_stack.cc. HtmlMarker groups HTML
// scope-marker tags; ForeignMarker groups non-HTML scope-marker names.
static bool is_list_item_scope_marker(Tag t) {
    return t==Tag::Ol || t==Tag::Ul || t==Tag::HtmlMarker ||
           t==Tag::ForeignMarker || t==Tag::FragmentMarker;
}
static bool in_list_item_scope_li(const Stack& stack) {
    for (Tag t : stack) {
        if (t==Tag::Li) return true;
        if (is_list_item_scope_marker(t)) return false;
    }
    return false;
}

// Represented subset of HTMLConstructionSite::HasImpliedEndTag. All these
// constructors stand for HTML-namespace items; Other stands for a tag outside
// that switch. The complete Blink set is documented beside the source map.
static bool has_implied_end_tag(Tag t) {
    switch (t) {
        case Tag::Dd: case Tag::Dt: case Tag::Li: case Tag::Option:
        case Tag::Optgroup: case Tag::P: case Tag::Rb: case Tag::Rp:
        case Tag::Rt: case Tag::Rtc: return true;
        default: return false;
    }
}
static Stack generate_implied_end_tags_excluding_li(Stack stack) {
    while (!stack.empty() && has_implied_end_tag(stack.front()) &&
           stack.front()!=Tag::Li) {
        stack.erase(stack.begin());
    }
    return stack;
}
static Stack drop_through_li(Stack stack) {
    for (auto i=stack.begin(); i!=stack.end(); ++i)
        if (*i==Tag::Li) { stack.erase(stack.begin(),std::next(i)); return stack; }
    return stack;
}
// C++ transcription of only the stack effect of the </li> branch.
static Stack cpp_end_li(Stack stack) {
    if (!in_list_item_scope_li(stack)) return stack; // parse error; no mutation
    stack=generate_implied_end_tags_excluding_li(std::move(stack));
    return drop_through_li(std::move(stack));
}

static Ptr fn(Ptr a,Ptr b) { return arrow(std::move(a),std::move(b)); }
static Rule rule(Ptr lhs,Ptr rhs) { return {std::move(lhs),std::move(rhs)}; }
static Ptr stack_term(const Stack& stack) {
    Ptr result=sym("nilStack");
    for(auto i=stack.rbegin();i!=stack.rend();++i)
        result=apps(sym("consStack"),sym(tag_name(*i)),result);
    return result;
}

int main() {
    Signature sig; std::vector<Rule> rules;
    auto pv=[](size_t i){return pvar(i);};
    const Ptr tag=sym("Tag"), stack=sym("Stack"), bit=sym("Bool");
    const Ptr yes=sym("true"), no=sym("false");
    sig["Tag"]=type_sort();
    for(const auto name:kTagNames) sig[name]=tag;
    sig["Stack"]=type_sort(); sig["nilStack"]=stack;
    sig["consStack"]=fn(tag,fn(stack,stack));
    sig["Bool"]=type_sort(); sig["true"]=bit; sig["false"]=bit;
    sig["ifStack"]=fn(bit,fn(stack,fn(stack,stack)));
    sig["inListScopeLi"]=fn(stack,bit);
    sig["impliedExceptLi"]=fn(stack,stack);
    sig["dropThroughLi"]=fn(stack,stack);
    sig["endLi"]=fn(stack,stack);
    sig["eqTag"]=fn(tag,fn(tag,bit)); sig["eqStack"]=fn(stack,fn(stack,bit));
    sig["andB"]=fn(bit,fn(bit,bit));
    sig["Nat"]=type_sort(); sig["zero"]=sym("Nat");
    sig["nativeAt"]=fn(sym("Nat"),stack); sig["semanticAt"]=fn(sym("Nat"),stack);

    rules.push_back(rule(apps(sym("ifStack"),yes,pv(0),pv(1)),pv(0)));
    rules.push_back(rule(apps(sym("ifStack"),no,pv(0),pv(1)),pv(1)));
    rules.push_back(rule(app(sym("inListScopeLi"),sym("nilStack")),no));
    rules.push_back(rule(apps(sym("inListScopeLi"),apps(sym("consStack"),sym("liTag"),pv(0))),yes));
    for(Tag t:{Tag::Ol,Tag::Ul,Tag::HtmlMarker,Tag::ForeignMarker,Tag::FragmentMarker})
        rules.push_back(rule(apps(sym("inListScopeLi"),apps(sym("consStack"),sym(tag_name(t)),pv(0))),no));
    for(Tag t:{Tag::Dd,Tag::Dt,Tag::Option,Tag::Optgroup,Tag::P,Tag::Rb,Tag::Rp,
                Tag::Rt,Tag::Rtc,Tag::Other,Tag::Button})
        rules.push_back(rule(apps(sym("inListScopeLi"),apps(sym("consStack"),sym(tag_name(t)),pv(0))),
                             app(sym("inListScopeLi"),pv(0))));

    rules.push_back(rule(app(sym("impliedExceptLi"),sym("nilStack")),sym("nilStack")));
    // The excluded li stops this loop; every other represented candidate is
    // popped. Non-candidates preserve the entire remaining stack.
    rules.push_back(rule(apps(sym("impliedExceptLi"),apps(sym("consStack"),sym("liTag"),pv(0))),
                         apps(sym("consStack"),sym("liTag"),pv(0))));
    for(Tag t:{Tag::Dd,Tag::Dt,Tag::Option,Tag::Optgroup,Tag::P,Tag::Rb,Tag::Rp,Tag::Rt,Tag::Rtc})
        rules.push_back(rule(apps(sym("impliedExceptLi"),apps(sym("consStack"),sym(tag_name(t)),pv(0))),
                             app(sym("impliedExceptLi"),pv(0))));
    for(Tag t:{Tag::Other,Tag::Button,Tag::Ol,Tag::Ul,Tag::HtmlMarker,Tag::ForeignMarker,Tag::FragmentMarker})
        rules.push_back(rule(apps(sym("impliedExceptLi"),apps(sym("consStack"),sym(tag_name(t)),pv(0))),
                             apps(sym("consStack"),sym(tag_name(t)),pv(0))));

    rules.push_back(rule(app(sym("dropThroughLi"),sym("nilStack")),sym("nilStack")));
    rules.push_back(rule(apps(sym("dropThroughLi"),apps(sym("consStack"),sym("liTag"),pv(0))),pv(0)));
    for(Tag t:{Tag::Dd,Tag::Dt,Tag::Option,Tag::Optgroup,Tag::P,Tag::Rb,Tag::Rp,Tag::Rt,
               Tag::Rtc,Tag::Other,Tag::Button,Tag::Ol,Tag::Ul,Tag::HtmlMarker,
               Tag::ForeignMarker,Tag::FragmentMarker})
        rules.push_back(rule(apps(sym("dropThroughLi"),apps(sym("consStack"),sym(tag_name(t)),pv(0))),
                             app(sym("dropThroughLi"),pv(0))));
    // Distinct semantic end-li transition: list-item-scope miss is a no-op;
    // a hit runs the implied-end-tag phase, then pops through the target li.
    rules.push_back(rule(app(sym("endLi"),pv(0)),
        apps(sym("ifStack"),app(sym("inListScopeLi"),pv(0)),
             app(sym("dropThroughLi"),app(sym("impliedExceptLi"),pv(0))),pv(0))));

    rules.push_back(rule(apps(sym("andB"),yes,yes),yes));
    rules.push_back(rule(apps(sym("andB"),yes,no),no));
    rules.push_back(rule(apps(sym("andB"),no,pv(0)),no));
    rules.push_back(rule(apps(sym("eqStack"),sym("nilStack"),sym("nilStack")),yes));
    rules.push_back(rule(apps(sym("eqStack"),sym("nilStack"),apps(sym("consStack"),pv(0),pv(1))),no));
    rules.push_back(rule(apps(sym("eqStack"),apps(sym("consStack"),pv(0),pv(1)),sym("nilStack")),no));
    rules.push_back(rule(apps(sym("eqStack"),apps(sym("consStack"),pv(0),pv(1)),
                              apps(sym("consStack"),pv(2),pv(3))),
        apps(sym("andB"),apps(sym("eqTag"),pv(0),pv(2)),apps(sym("eqStack"),pv(1),pv(3)))));
    for(size_t i=0;i<kTagNames.size();++i) for(size_t j=0;j<kTagNames.size();++j)
        rules.push_back(rule(apps(sym("eqTag"),sym(kTagNames[i]),sym(kTagNames[j])),i==j?yes:no));

    const std::vector<Stack> cases={
        {Tag::Li,Tag::Other,Tag::HtmlMarker},
        {Tag::P,Tag::Dt,Tag::Option,Tag::Li,Tag::Other,Tag::HtmlMarker},
        {Tag::Other,Tag::Li,Tag::Other,Tag::HtmlMarker},
        {Tag::Button,Tag::Li,Tag::Other,Tag::HtmlMarker}, // button isn't a li-scope marker
        {Tag::Other,Tag::Ol,Tag::Li,Tag::HtmlMarker},
        {Tag::Other,Tag::Ul,Tag::Li,Tag::HtmlMarker},
        {Tag::Other,Tag::HtmlMarker,Tag::Li},
        {Tag::Other,Tag::ForeignMarker,Tag::Li},
        {Tag::Other,Tag::FragmentMarker,Tag::Li},
        {Tag::Dt,Tag::Option,Tag::Other}, // miss: no li
        {Tag::Li,Tag::P,Tag::Li,Tag::Other,Tag::HtmlMarker}};
    size_t hit_cases=0, miss_cases=0, boundary_checks=0;
    for(size_t i=0;i<cases.size();++i) {
        const auto case_name="liCase"+std::to_string(i); sig[case_name]=sym("Nat");
        rules.push_back(rule(app(sym("nativeAt"),sym(case_name)),stack_term(cpp_end_li(cases[i]))));
        rules.push_back(rule(app(sym("semanticAt"),sym(case_name)),app(sym("endLi"),stack_term(cases[i]))));
        const auto comparison=apps(sym("eqStack"),app(sym("nativeAt"),sym(case_name)),
                                  app(sym("semanticAt"),sym(case_name)));
        const auto type=infer({},comparison,sig,rules);
        assert(type&&conv(*type,bit,rules));
        assert(conv(nf(comparison,rules),yes,rules));
        if(in_list_item_scope_li(cases[i])) {
            ++hit_cases;
            ++boundary_checks;
            assert(cpp_end_li(cases[i])==drop_through_li(cases[i]));
        } else {
            ++miss_cases;
            assert(cpp_end_li(cases[i])==cases[i]);
        }
    }
    std::println("Blink </li> transition: {} stacks agree ({} scope hits, {} unchanged misses; {} implied-pop abstraction checks)",
                 cases.size(),hit_cases,miss_cases,boundary_checks);
}
