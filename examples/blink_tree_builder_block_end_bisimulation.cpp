#include "../src/lambda_pi.hpp"
#include <array>
#include <cassert>
#include <vector>

using namespace lp;

// Target tags are listed in the same order as the shared C++ switch branch.
enum class Target {
    Address, Article, Aside, Blockquote, Button, Center, Details, Dialog, Dir,
    Div, Dl, Fieldset, Figcaption, Figure, Footer, Header, Hgroup, Listing,
    Main, Menu, Nav, Ol, Pre, Search, Section, Summary, Select, Ul
};
enum class Tag {
    Address, Article, Aside, Blockquote, Button, Center, Details, Dialog, Dir,
    Div, Dl, Fieldset, Figcaption, Figure, Footer, Header, Hgroup, Listing,
    Main, Menu, Nav, Ol, Pre, Search, Section, Summary, Select, Ul,
    Dd, Dt, Li, Option, Optgroup, P, Rb, Rp, Rt, Rtc,
    Other, HtmlScopeMarker, ForeignScopeMarker, FragmentRoot
};
using Stack=std::vector<Tag>; // top first

static constexpr std::array<const char*,28> kTargetNames={
    "addressTag","articleTag","asideTag","blockquoteTag","buttonTag",
    "centerTag","detailsTag","dialogTag","dirTag","divTag","dlTag",
    "fieldsetTag","figcaptionTag","figureTag","footerTag","headerTag",
    "hgroupTag","listingTag","mainTag","menuTag","navTag","olTag",
    "preTag","searchTag","sectionTag","summaryTag","selectTag","ulTag"};
static constexpr std::array<const char*,42> kTagNames={
    "addressTag","articleTag","asideTag","blockquoteTag","buttonTag",
    "centerTag","detailsTag","dialogTag","dirTag","divTag","dlTag",
    "fieldsetTag","figcaptionTag","figureTag","footerTag","headerTag",
    "hgroupTag","listingTag","mainTag","menuTag","navTag","olTag",
    "preTag","searchTag","sectionTag","summaryTag","selectTag","ulTag",
    "ddTag","dtTag","liTag","optionTag","optgroupTag","pTag","rbTag",
    "rpTag","rtTag","rtcTag","otherTag","htmlScopeMarker",
    "foreignScopeMarker","fragmentRoot"};
static Tag as_tag(Target target) { return static_cast<Tag>(target); }
static const char* tag_name(Tag tag) { return kTagNames[static_cast<size_t>(tag)]; }
static const char* target_name(Target target) { return kTargetNames[static_cast<size_t>(target)]; }
static bool is_scope_marker(Tag tag) {
    return tag==Tag::Select || tag==Tag::HtmlScopeMarker ||
           tag==Tag::ForeignScopeMarker || tag==Tag::FragmentRoot;
}
static bool in_scope(Target target,const Stack& stack) {
    const Tag wanted=as_tag(target);
    for(Tag tag:stack) {
        if(tag==wanted) return true; // match is checked before marker, as in Blink
        if(is_scope_marker(tag)) return false;
    }
    return false;
}
static bool has_implied_end_tag(Tag tag) {
    switch(tag) {
        case Tag::Dd: case Tag::Dt: case Tag::Li: case Tag::Option:
        case Tag::Optgroup: case Tag::P: case Tag::Rb: case Tag::Rp:
        case Tag::Rt: case Tag::Rtc: return true;
        default: return false;
    }
}
static Stack implied_end_tags(Stack stack) {
    while(!stack.empty() && has_implied_end_tag(stack.front())) stack.erase(stack.begin());
    return stack;
}
static Stack pop_through(Target target,Stack stack) {
    const Tag wanted=as_tag(target);
    for(auto i=stack.begin();i!=stack.end();++i)
        if(*i==wanted) { stack.erase(stack.begin(),std::next(i)); return stack; }
    return stack;
}
static Stack cpp_end_block(Target target,Stack stack) {
    if(!in_scope(target,stack)) return stack;
    stack=implied_end_tags(std::move(stack));
    return pop_through(target,std::move(stack));
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
    const Ptr target=sym("BlockTarget"), tag=sym("StackTag"), stack=sym("OpenStack");
    const Ptr bit=sym("Bool"), yes=sym("true"), no=sym("false");
    sig["BlockTarget"]=type_sort();
    for(const auto name:kTargetNames) sig[name]=target;
    sig["StackTag"]=type_sort();
    for(const auto name:kTagNames) sig[name]=tag;
    sig["OpenStack"]=type_sort(); sig["nilStack"]=stack;
    sig["consStack"]=fn(tag,fn(stack,stack));
    sig["Bool"]=type_sort(); sig["true"]=bit; sig["false"]=bit;
    sig["ifB"]=fn(bit,fn(bit,fn(bit,bit)));
    sig["ifStack"]=fn(bit,fn(stack,fn(stack,stack)));
    sig["matchesTarget"]=fn(target,fn(tag,bit));
    sig["isScopeMarker"]=fn(tag,bit); sig["isImpliedEndTag"]=fn(tag,bit);
    sig["inScope"]=fn(target,fn(stack,bit));
    sig["generateImpliedEndTags"]=fn(stack,stack);
    sig["popThroughTarget"]=fn(target,fn(stack,stack));
    sig["blockEnd"]=fn(target,fn(stack,stack));
    sig["directBlockEnd"]=fn(target,fn(stack,stack));
    sig["Input"]=type_sort(); sig["targetAt"]=fn(sym("Input"),target);
    sig["stackAt"]=fn(sym("Input"),stack);
    sig["cppResult"]=fn(sym("Input"),stack); sig["semanticResult"]=fn(sym("Input"),stack);

    rules.push_back(rule(apps(sym("ifB"),yes,pv(0),pv(1)),pv(0)));
    rules.push_back(rule(apps(sym("ifB"),no,pv(0),pv(1)),pv(1)));
    rules.push_back(rule(apps(sym("ifStack"),yes,pv(0),pv(1)),pv(0)));
    rules.push_back(rule(apps(sym("ifStack"),no,pv(0),pv(1)),pv(1)));
    // The finite alphabet contains all branch targets plus implied candidates
    // and marker constructors. Every target is distinct; `select` is both a
    // branch target and an HTML scope marker, with target matching tested first.
    const std::array<Tag,15> scan_tags={Tag::Dd,Tag::Dt,Tag::Li,Tag::Option,
        Tag::Optgroup,Tag::P,Tag::Rb,Tag::Rp,Tag::Rt,Tag::Rtc,Tag::Other,
        Tag::Select,Tag::HtmlScopeMarker,Tag::ForeignScopeMarker,Tag::FragmentRoot};
    for(size_t i=0;i<kTargetNames.size();++i) {
        rules.push_back(rule(apps(sym("matchesTarget"),sym(kTargetNames[i]),
                                  sym(tag_name(static_cast<Tag>(i)))),yes));
        for(Tag st:scan_tags)
            rules.push_back(rule(apps(sym("matchesTarget"),sym(kTargetNames[i]),sym(tag_name(st))),
                (static_cast<size_t>(st)==i)?yes:no));
        // Target constructors not used as stack heads in the finite inputs do
        // not receive mismatch rules; this is intentionally a finite model.
    }
    for(Tag st:{Tag::Select,Tag::HtmlScopeMarker,Tag::ForeignScopeMarker,Tag::FragmentRoot})
        rules.push_back(rule(app(sym("isScopeMarker"),sym(tag_name(st))),yes));
    for(Tag st:{Tag::Address,Tag::Article,Tag::Aside,Tag::Blockquote,Tag::Button,
        Tag::Center,Tag::Details,Tag::Dialog,Tag::Dir,Tag::Div,Tag::Dl,Tag::Fieldset,
        Tag::Figcaption,Tag::Figure,Tag::Footer,Tag::Header,Tag::Hgroup,Tag::Listing,
        Tag::Main,Tag::Menu,Tag::Nav,Tag::Ol,Tag::Pre,Tag::Search,Tag::Section,
        Tag::Summary,Tag::Ul,Tag::Dd,Tag::Dt,Tag::Li,Tag::Option,Tag::Optgroup,
        Tag::P,Tag::Rb,Tag::Rp,Tag::Rt,Tag::Rtc,Tag::Other})
        rules.push_back(rule(app(sym("isScopeMarker"),sym(tag_name(st))),no));
    for(Tag st:{Tag::Dd,Tag::Dt,Tag::Li,Tag::Option,Tag::Optgroup,Tag::P,
                Tag::Rb,Tag::Rp,Tag::Rt,Tag::Rtc})
        rules.push_back(rule(app(sym("isImpliedEndTag"),sym(tag_name(st))),yes));
    for(Tag st:{Tag::Address,Tag::Article,Tag::Aside,Tag::Blockquote,Tag::Button,
        Tag::Center,Tag::Details,Tag::Dialog,Tag::Dir,Tag::Div,Tag::Dl,Tag::Fieldset,
        Tag::Figcaption,Tag::Figure,Tag::Footer,Tag::Header,Tag::Hgroup,Tag::Listing,
        Tag::Main,Tag::Menu,Tag::Nav,Tag::Ol,Tag::Pre,Tag::Search,Tag::Section,
        Tag::Summary,Tag::Select,Tag::Ul,Tag::Other,Tag::HtmlScopeMarker,
        Tag::ForeignScopeMarker,Tag::FragmentRoot})
        rules.push_back(rule(app(sym("isImpliedEndTag"),sym(tag_name(st))),no));

    rules.push_back(rule(apps(sym("inScope"),pv(0),sym("nilStack")),no));
    Ptr scope_body=apps(sym("ifB"),apps(sym("matchesTarget"),pv(0),pv(1)),yes,
        apps(sym("ifB"),app(sym("isScopeMarker"),pv(1)),no,apps(sym("inScope"),pv(0),pv(2))));
    rules.push_back(rule(apps(sym("inScope"),pv(0),apps(sym("consStack"),pv(1),pv(2))),scope_body));
    rules.push_back(rule(app(sym("generateImpliedEndTags"),sym("nilStack")),sym("nilStack")));
    rules.push_back(rule(app(sym("generateImpliedEndTags"),apps(sym("consStack"),pv(0),pv(1))),
        apps(sym("ifStack"),app(sym("isImpliedEndTag"),pv(0)),
            app(sym("generateImpliedEndTags"),pv(1)),apps(sym("consStack"),pv(0),pv(1)))));
    rules.push_back(rule(apps(sym("popThroughTarget"),pv(0),sym("nilStack")),sym("nilStack")));
    rules.push_back(rule(apps(sym("popThroughTarget"),pv(0),apps(sym("consStack"),pv(1),pv(2))),
        apps(sym("ifStack"),apps(sym("matchesTarget"),pv(0),pv(1)),pv(2),
             apps(sym("popThroughTarget"),pv(0),pv(2)))));
    rules.push_back(rule(apps(sym("blockEnd"),pv(0),pv(1)),
        apps(sym("ifStack"),apps(sym("inScope"),pv(0),pv(1)),
            apps(sym("popThroughTarget"),pv(0),app(sym("generateImpliedEndTags"),pv(1))),pv(1))));
    rules.push_back(rule(apps(sym("directBlockEnd"),pv(0),pv(1)),
        apps(sym("ifStack"),apps(sym("inScope"),pv(0),pv(1)),
            apps(sym("popThroughTarget"),pv(0),pv(1)),pv(1))));

    struct Input { Target target; Stack stack; };
    std::vector<Input> inputs;
    // Each HTMLTag constructor in the shared C++ case set gets an independent
    // target identity and a matching stack item.
    for(size_t i=0;i<kTargetNames.size();++i)
        inputs.push_back({static_cast<Target>(i),{static_cast<Tag>(i),Tag::Other,Tag::HtmlScopeMarker}});
    // Additional scans exercise implied tags, ordinary tops, a target hidden
    // below markers, select's target-before-marker ordering, and a true miss.
    inputs.push_back({Target::Section,{Tag::P,Tag::Li,Tag::Dt,Tag::Option,Tag::Rtc,
                                      Tag::Section,Tag::Other,Tag::HtmlScopeMarker}});
    inputs.push_back({Target::Article,{Tag::Other,Tag::Article,Tag::Other,Tag::HtmlScopeMarker}});
    inputs.push_back({Target::Select,{Tag::Select,Tag::Other,Tag::HtmlScopeMarker}});
    inputs.push_back({Target::Div,{Tag::Other,Tag::HtmlScopeMarker,Tag::Div}});
    inputs.push_back({Target::Div,{Tag::Other,Tag::ForeignScopeMarker,Tag::Div}});
    inputs.push_back({Target::Div,{Tag::Other,Tag::FragmentRoot,Tag::Div}});
    inputs.push_back({Target::Div,{Tag::Other,Tag::HtmlScopeMarker}});

    size_t scope_hits=0,scope_misses=0,abstraction_checks=0;
    for(size_t i=0;i<inputs.size();++i) {
        const auto input_name="input"+std::to_string(i); sig[input_name]=sym("Input");
        const auto& in=inputs[i];
        rules.push_back(rule(app(sym("targetAt"),sym(input_name)),sym(target_name(in.target))));
        rules.push_back(rule(app(sym("stackAt"),sym(input_name)),stack_term(in.stack)));
        rules.push_back(rule(app(sym("cppResult"),sym(input_name)),stack_term(cpp_end_block(in.target,in.stack))));
        rules.push_back(rule(app(sym("semanticResult"),sym(input_name)),
            apps(sym("blockEnd"),app(sym("targetAt"),sym(input_name)),app(sym("stackAt"),sym(input_name)))));
        auto native=app(sym("cppResult"),sym(input_name));
        auto semantic=app(sym("semanticResult"),sym(input_name));
        auto native_type=infer({},native,sig,rules), semantic_type=infer({},semantic,sig,rules);
        assert(native_type&&semantic_type&&conv(*native_type,stack,rules)&&conv(*semantic_type,stack,rules));
        assert(conv(nf(native,rules),nf(semantic,rules),rules));
        if(in_scope(in.target,in.stack)) {
            ++scope_hits; ++abstraction_checks;
            assert(cpp_end_block(in.target,in.stack)==pop_through(in.target,in.stack));
            auto direct=apps(sym("directBlockEnd"),sym(target_name(in.target)),stack_term(in.stack));
            auto staged=apps(sym("blockEnd"),sym(target_name(in.target)),stack_term(in.stack));
            assert(conv(nf(staged,rules),nf(direct,rules),rules));
        } else {
            ++scope_misses;
            assert(cpp_end_block(in.target,in.stack)==in.stack);
        }
    }
    std::println("Blink shared block end-tag transition: {} configurations, {} scope hits, {} unchanged misses, {} implied-pass abstraction checks",
        inputs.size(),scope_hits,scope_misses,abstraction_checks);
}
