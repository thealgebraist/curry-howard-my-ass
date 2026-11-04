#include "../src/lambda_pi.hpp"
#include <cassert>
#include <array>
#include <vector>

using namespace lp;

enum class Tag { P, Li, Dt, Option, Other, Button, HtmlMarker, ForeignMarker, FragmentMarker };
using Stack = std::vector<Tag>; // top first

static constexpr std::array<const char*, 9> kTags={"pTag","liTag","dtTag","optionTag","otherTag","buttonTag","htmlMarker","foreignMarker","fragmentMarker"};
static const char* tag_name(Tag t) { return kTags[static_cast<size_t>(t)]; }
// Projection of HTMLConstructionSite::HasImpliedEndTag for the represented
// HTML-namespace candidates. Other tags stand for names outside its switch.
static bool has_implied_end_tag(Tag t) {
    return t==Tag::Li || t==Tag::Dt || t==Tag::Option || t==Tag::P;
}

static bool in_button_scope_p(const Stack& s) {
    for (Tag t : s) {
        if (t == Tag::P) return true;
        if (t == Tag::Button || t == Tag::HtmlMarker ||
            t == Tag::ForeignMarker || t == Tag::FragmentMarker) return false;
    }
    return false;
}
static Stack drop_through_p(Stack s) {
    for (auto i = s.begin(); i != s.end(); ++i)
        if (*i == Tag::P) { s.erase(s.begin(), std::next(i)); return s; }
    return s;
}
static Stack generate_implied_end_tags_excluding_p(Stack s) {
    while (!s.empty() && has_implied_end_tag(s.front()) && s.front()!=Tag::P)
        s.erase(s.begin());
    return s;
}
// Observable stack projection of ProcessEndTagForInBody(kP), including
// candidate-specific implied-end-tag pops before PopUntilPopped(kP).
static Stack cpp_end_p(Stack s) {
    if (!in_button_scope_p(s)) s.insert(s.begin(), Tag::P); // fake <p> start
    s = generate_implied_end_tags_excluding_p(std::move(s));
    return drop_through_p(std::move(s));
}
static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }
static Ptr stack_term(const Stack& s) {
    Ptr r = sym("nilStack");
    for (auto i=s.rbegin(); i!=s.rend(); ++i) {
        r = apps(sym("consStack"), sym(tag_name(*i)), r);
    }
    return r;
}
int main() {
    Signature sig; std::vector<Rule> rs;
    auto p=[](size_t i){return pvar(i);};
    auto tag=sym("Tag"), st=sym("Stack"), bit=sym("Bool");
    auto yes=sym("true"), no=sym("false");
    sig["Tag"]=type_sort();
    for (auto n : kTags) sig[n]=tag;
    sig["Stack"]=type_sort(); sig["nilStack"]=st;
    sig["consStack"]=fn(tag,fn(st,st)); sig["scopeP"]=fn(st,bit);
    sig["dropP"]=fn(st,st); sig["endP"]=fn(st,st);
    sig["Bool"]=type_sort(); sig["true"]=bit; sig["false"]=bit;
    sig["ifStack"]=fn(bit,fn(st,fn(st,st)));
    sig["eqStack"]=fn(st,fn(st,bit)); sig["nativeAt"]=fn(sym("Nat"),st);
    sig["semanticAt"]=fn(sym("Nat"),st); sig["Nat"]=type_sort(); sig["zero"]=sym("Nat");
    rs.push_back(rule(apps(sym("ifStack"),yes,p(0),p(1)),p(0)));
    rs.push_back(rule(apps(sym("ifStack"),no,p(0),p(1)),p(1)));
    rs.push_back(rule(app(sym("scopeP"),sym("nilStack")),no));
    rs.push_back(rule(apps(sym("scopeP"),apps(sym("consStack"),sym("pTag"),p(0))),yes));
    for(auto n:{"liTag","dtTag","optionTag","otherTag"}) rs.push_back(rule(apps(sym("scopeP"),apps(sym("consStack"),sym(n),p(0))),app(sym("scopeP"),p(0))));
    for(auto n:{"buttonTag","htmlMarker","foreignMarker","fragmentMarker"})
        rs.push_back(rule(apps(sym("scopeP"),apps(sym("consStack"),sym(n),p(0))),no));
    rs.push_back(rule(app(sym("dropP"),sym("nilStack")),sym("nilStack")));
    rs.push_back(rule(apps(sym("dropP"),apps(sym("consStack"),sym("pTag"),p(0))),p(0)));
    rs.push_back(rule(apps(sym("dropP"),apps(sym("consStack"),p(0),p(1))),app(sym("dropP"),p(1))));
    rs.push_back(rule(app(sym("endP"),p(0)),app(sym("dropP"),apps(sym("consStack"),sym("pTag"),
        apps(sym("ifStack"),app(sym("scopeP"),p(0)),app(sym("dropP"),p(0)),p(0))))));
    rs.push_back(rule(apps(sym("eqStack"),sym("nilStack"),sym("nilStack")),yes));
    rs.push_back(rule(apps(sym("eqStack"),sym("nilStack"),apps(sym("consStack"),p(0),p(1))),no));
    rs.push_back(rule(apps(sym("eqStack"),apps(sym("consStack"),p(0),p(1)),sym("nilStack")),no));
    rs.push_back(rule(apps(sym("eqStack"),apps(sym("consStack"),p(0),p(1)),apps(sym("consStack"),p(2),p(3))),
        apps(sym("andB"),apps(sym("eqTag"),p(0),p(2)),apps(sym("eqStack"),p(1),p(3)))));
    sig["andB"]=fn(bit,fn(bit,bit)); sig["eqTag"]=fn(tag,fn(tag,bit));
    rs.push_back(rule(apps(sym("andB"),yes,yes),yes)); rs.push_back(rule(apps(sym("andB"),yes,no),no));
    rs.push_back(rule(apps(sym("andB"),no,p(0)),no));
    for(size_t i=0;i<kTags.size();++i) for(size_t j=0;j<kTags.size();++j)
        rs.push_back(rule(apps(sym("eqTag"),sym(kTags[i]),sym(kTags[j])),i==j?yes:no));
    const std::vector<Stack> cases={
        {Tag::P,Tag::Other,Tag::HtmlMarker},
        {Tag::Li,Tag::Dt,Tag::Option,Tag::Other,Tag::P,Tag::Other,Tag::HtmlMarker},
        {Tag::Other,Tag::Li,Tag::Dt,Tag::P,Tag::Other,Tag::HtmlMarker},
        {Tag::Other,Tag::P,Tag::HtmlMarker},
        {Tag::Li,Tag::Other,Tag::P,Tag::HtmlMarker},
        {Tag::Other,Tag::Button,Tag::P,Tag::HtmlMarker},
        {Tag::Button,Tag::P,Tag::HtmlMarker},
        {Tag::P,Tag::Button,Tag::HtmlMarker},
        {Tag::Dt,Tag::Option,Tag::Other,Tag::Button,Tag::P,Tag::HtmlMarker},
        {Tag::P,Tag::Li,Tag::Dt,Tag::Option,Tag::HtmlMarker}};
    for(size_t i=0;i<cases.size();++i){
        const auto n=std::to_string(i); sig["case"+n]=sym("Nat");
        rs.push_back(rule(app(sym("nativeAt"),sym("case"+n)),stack_term(cpp_end_p(cases[i]))));
        rs.push_back(rule(app(sym("semanticAt"),sym("case"+n)),app(sym("endP"),stack_term(cases[i]))));
        auto eq=apps(sym("eqStack"),app(sym("nativeAt"),sym("case"+n)),app(sym("semanticAt"),sym("case"+n)));
        auto ty=infer({},eq,sig,rs); assert(ty&&conv(*ty,bit,rs));
        if (!conv(nf(eq,rs),yes,rs)) { std::println("failed case {}: {}",i,to_string(nf(eq,rs))); assert(false); }
    }
    // Abstraction-boundary check: whether eligible implied tags are popped
    // explicitly first or simply discarded by the later pop-through-p, the
    // final stack must be the suffix below the same p.
    size_t scope_cases=0;
    for(const auto& s:cases) if(in_button_scope_p(s)) {
        ++scope_cases;
        assert(cpp_end_p(s)==drop_through_p(s));
    }
    std::println("Blink </p> stack projection: {} finite stacks agree ({} in-scope direct-pop abstraction checks)",cases.size(),scope_cases);
}
