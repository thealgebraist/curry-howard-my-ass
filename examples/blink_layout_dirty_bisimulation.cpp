#include "../src/lambda_pi.hpp"
#include <cassert>
#include <vector>

using namespace lp;

struct LayoutNode {
    bool self_needs_full_layout;
    bool child_needs_full_layout;
};
using LayoutChain = std::vector<LayoutNode>; // target first, then containers

// Bounded transcription of LayoutObject::SetNeedsLayout plus the basic
// container-chain branch of MarkContainerChainForLayout. This excludes locks,
// OOF jumps, unrooted nodes, simplified layout, and scheduling.
static LayoutChain cpp_set_needs_layout(LayoutChain chain) {
    if (chain.empty()) return chain;
    const bool already_needed = chain.front().self_needs_full_layout;
    chain.front().self_needs_full_layout = true;
    if (already_needed) return chain;

    for (size_t i = 1; i < chain.size(); ++i) {
        auto& container = chain[i];
        if (container.self_needs_full_layout ||
            container.child_needs_full_layout) {
            break;
        }
        container.child_needs_full_layout = true;
    }
    return chain;
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr a, Ptr b) { return {std::move(a), std::move(b)}; }

static Ptr bool_term(bool b) { return sym(b ? "true" : "false"); }
static Ptr node_term(LayoutNode n) {
    return apps(sym("layoutNode"), bool_term(n.self_needs_full_layout),
                bool_term(n.child_needs_full_layout));
}
static Ptr chain_term(const LayoutChain& xs) {
    Ptr result = sym("nilLayout");
    for (auto it = xs.rbegin(); it != xs.rend(); ++it)
        result = apps(sym("consLayout"), node_term(*it), result);
    return result;
}
static Ptr case_term(size_t i) {
    Ptr result = sym("zero");
    while (i--) result = app(sym("succ"), result);
    return result;
}

int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto p = [](size_t i) { return pvar(i); };
    const auto boolean = sym("Bool"), node = sym("LayoutNode");
    const auto chain = sym("LayoutChain");
    const auto yes = sym("true"), no = sym("false");

    sig["Bool"] = type_sort(); sig["true"] = boolean; sig["false"] = boolean;
    sig["LayoutNode"] = type_sort();
    sig["layoutNode"] = fn(boolean, fn(boolean, node));
    sig["LayoutChain"] = type_sort(); sig["nilLayout"] = chain;
    sig["consLayout"] = fn(node, fn(chain, chain));
    sig["propagate"] = fn(chain, chain);
    sig["setNeedsLayout"] = fn(chain, chain);
    sig["ifChain"] = fn(boolean, fn(chain, fn(chain, chain)));
    sig["Nat"] = type_sort(); sig["zero"] = sym("Nat");
    sig["succ"] = fn(sym("Nat"), sym("Nat"));
    sig["inputAt"] = fn(sym("Nat"), chain);
    sig["nativeAt"] = fn(sym("Nat"), chain);
    sig["semanticAt"] = fn(sym("Nat"), chain);
    sig["eqBool"] = fn(boolean, fn(boolean, boolean));
    sig["andB"] = fn(boolean, fn(boolean, boolean));
    sig["eqNode"] = fn(node, fn(node, boolean));
    sig["eqChain"] = fn(chain, fn(chain, boolean));
    sig["checkAt"] = fn(sym("Nat"), boolean);

    rules.push_back(rule(apps(sym("ifChain"), yes, p(0), p(1)), p(0)));
    rules.push_back(rule(apps(sym("ifChain"), no, p(0), p(1)), p(1)));
    rules.push_back(rule(app(sym("propagate"), sym("nilLayout")),
                         sym("nilLayout")));
    // An ancestor that needs its own layout already covers this descendant.
    rules.push_back(rule(
        apps(sym("propagate"),
             apps(sym("consLayout"),
                  apps(sym("layoutNode"), yes, p(0)), p(1))),
        apps(sym("consLayout"), apps(sym("layoutNode"), yes, p(0)), p(1))));
    // Existing child dirtiness also stops propagation at this ancestor.
    rules.push_back(rule(
        apps(sym("propagate"),
             apps(sym("consLayout"),
                  apps(sym("layoutNode"), no, yes), p(0))),
        apps(sym("consLayout"), apps(sym("layoutNode"), no, yes), p(0))));
    // Otherwise mark this ancestor's child bit and continue toward its parent.
    rules.push_back(rule(
        apps(sym("propagate"),
             apps(sym("consLayout"),
                  apps(sym("layoutNode"), no, no), p(0))),
        apps(sym("consLayout"), apps(sym("layoutNode"), no, yes),
             app(sym("propagate"), p(0)))));

    rules.push_back(rule(app(sym("setNeedsLayout"), sym("nilLayout")),
                         sym("nilLayout")));
    // Idempotence: an already self-dirty target does not repeat propagation.
    rules.push_back(rule(
        app(sym("setNeedsLayout"),
            apps(sym("consLayout"), apps(sym("layoutNode"), yes, p(0)), p(1))),
        apps(sym("consLayout"), apps(sym("layoutNode"), yes, p(0)), p(1))));
    // A newly dirty target triggers container-chain propagation.
    rules.push_back(rule(
        app(sym("setNeedsLayout"),
            apps(sym("consLayout"), apps(sym("layoutNode"), no, p(0)), p(1))),
        apps(sym("consLayout"), apps(sym("layoutNode"), yes, p(0)),
             app(sym("propagate"), p(1)))));

    rules.push_back(rule(apps(sym("eqBool"), yes, yes), yes));
    rules.push_back(rule(apps(sym("eqBool"), yes, no), no));
    rules.push_back(rule(apps(sym("eqBool"), no, yes), no));
    rules.push_back(rule(apps(sym("eqBool"), no, no), yes));
    rules.push_back(rule(apps(sym("andB"), yes, yes), yes));
    rules.push_back(rule(apps(sym("andB"), yes, no), no));
    rules.push_back(rule(apps(sym("andB"), no, p(0)), no));
    rules.push_back(rule(apps(sym("eqNode"),
                              apps(sym("layoutNode"), p(0), p(1)),
                              apps(sym("layoutNode"), p(2), p(3))),
                         apps(sym("andB"), apps(sym("eqBool"), p(0), p(2)),
                              apps(sym("eqBool"), p(1), p(3)))));
    rules.push_back(rule(apps(sym("eqChain"), sym("nilLayout"),
                              sym("nilLayout")), yes));
    rules.push_back(rule(apps(sym("eqChain"), sym("nilLayout"),
                              apps(sym("consLayout"), p(0), p(1))), no));
    rules.push_back(rule(apps(sym("eqChain"),
                              apps(sym("consLayout"), p(0), p(1)),
                              sym("nilLayout")), no));
    rules.push_back(rule(apps(sym("eqChain"),
                              apps(sym("consLayout"), p(0), p(1)),
                              apps(sym("consLayout"), p(2), p(3))),
                         apps(sym("andB"), apps(sym("eqNode"), p(0), p(2)),
                              apps(sym("eqChain"), p(1), p(3)))));
    rules.push_back(rule(app(sym("semanticAt"), p(0)),
                         app(sym("setNeedsLayout"), app(sym("inputAt"), p(0)))));
    rules.push_back(rule(app(sym("checkAt"), p(0)),
                         apps(sym("eqChain"), app(sym("nativeAt"), p(0)),
                              app(sym("semanticAt"), p(0)))));

    // Chains are target-first, then its parent containers. All nodes are
    // rooted, in normal flow, and outside PerformLayout for these cases.
    const std::vector<LayoutChain> cases = {
        {{false, false}, {false, false}, {false, false}},
        {{true, false}, {false, false}, {false, false}},
        {{false, false}, {true, false}, {false, false}},
        {{false, false}, {false, true}, {false, false}},
        {{false, false}, {false, false}, {true, false}},
        {{false, true}, {false, false}},
        {{false, false}},
        {},
    };
    sig["inputAt"] = fn(sym("Nat"), chain);
    sig["nativeAt"] = fn(sym("Nat"), chain);
    for (size_t i = 0; i < cases.size(); ++i) {
        const Ptr id = case_term(i);
        const Ptr input = chain_term(cases[i]);
        const Ptr output = chain_term(cpp_set_needs_layout(cases[i]));
        rules.push_back(rule(app(sym("inputAt"), id), input));
        rules.push_back(rule(app(sym("nativeAt"), id), output));
        const Ptr check = app(sym("checkAt"), id);
        auto inferred = infer({}, check, sig, rules);
        assert(inferred && conv(*inferred, boolean, rules));
        assert(conv(nf(check, rules), yes, rules));
    }

    std::println("Blink layout dirty propagation: {} finite chains agree between C++ and typed λΠ", cases.size());
    std::println("Scope: self-dirty idempotence and basic container child-dirty propagation only.");
}
