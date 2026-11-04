#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <map>
#include <algorithm>
#include <expected>
#include <format>
#include <print>
#include <cassert>
#include <unordered_map>

namespace lp {

template<class... Ts> struct match_visitor : Ts... { using Ts::operator()...; };

struct Term;
using Ptr = std::shared_ptr<const Term>;

enum class SortKind { Type, Kind };

struct Sort { SortKind k; };
struct BVar { size_t idx; };       // De Bruijn index for bound variables
struct Sym  { std::string name; }; // Global constant / symbol
struct Pi   { Ptr dom, cod; };     // Dependent product: Π (x : A). B
struct Lam  { Ptr dom, body; };    // Abstraction: λ (x : A). t
struct App  { Ptr fn, arg; };      // Application: t u
struct PVar { size_t id; };        // Pattern variable for rewrite rules: ?i

struct Term { std::variant<Sort, BVar, Sym, Pi, Lam, App, PVar> v; };

// Smart constructors
inline Ptr make_sort(SortKind k) { return std::make_shared<Term>(Sort{k}); }
inline Ptr type_sort() { return make_sort(SortKind::Type); }
inline Ptr kind_sort() { return make_sort(SortKind::Kind); }
inline Ptr var(size_t idx) { return std::make_shared<Term>(BVar{idx}); }
inline Ptr sym(std::string name) { return std::make_shared<Term>(Sym{std::move(name)}); }
inline Ptr pi(Ptr dom, Ptr cod) { return std::make_shared<Term>(Pi{std::move(dom), std::move(cod)}); }
inline Ptr lam(Ptr dom, Ptr body) { return std::make_shared<Term>(Lam{std::move(dom), std::move(body)}); }
inline Ptr app(Ptr fn, Ptr arg) { return std::make_shared<Term>(App{std::move(fn), std::move(arg)}); }
inline Ptr pvar(size_t id) { return std::make_shared<Term>(PVar{id}); }

// De Bruijn operations
inline Ptr shift(Ptr t, int d, size_t cutoff = 0) {
    return std::visit(match_visitor{
        [&](const Sort&) -> Ptr { return t; },
        [&](const BVar& v) -> Ptr {
            if (v.idx >= cutoff) return var(static_cast<size_t>(static_cast<int>(v.idx) + d));
            return t;
        },
        [&](const Sym&)  -> Ptr { return t; },
        [&](const Pi& p)  -> Ptr { return pi(shift(p.dom, d, cutoff), shift(p.cod, d, cutoff + 1)); },
        [&](const Lam& l) -> Ptr { return lam(shift(l.dom, d, cutoff), shift(l.body, d, cutoff + 1)); },
        [&](const App& a) -> Ptr { return app(shift(a.fn, d, cutoff), shift(a.arg, d, cutoff)); },
        [&](const PVar&) -> Ptr { return t; }
    }, t->v);
}

inline Ptr subst(Ptr t, Ptr s, size_t i = 0) {
    return std::visit(match_visitor{
        [&](const Sort&) -> Ptr { return t; },
        [&](const BVar& v) -> Ptr {
            if (v.idx == i) return shift(s, static_cast<int>(i), 0);
            if (v.idx > i) return var(v.idx - 1);
            return t;
        },
        [&](const Sym&)  -> Ptr { return t; },
        [&](const Pi& p)  -> Ptr { return pi(subst(p.dom, s, i), subst(p.cod, s, i + 1)); },
        [&](const Lam& l) -> Ptr { return lam(subst(l.dom, s, i), subst(l.body, s, i + 1)); },
        [&](const App& a) -> Ptr { return app(subst(a.fn, s, i), subst(a.arg, s, i)); },
        [&](const PVar&) -> Ptr { return t; }
    }, t->v);
}

inline Ptr arrow(Ptr a, Ptr b) { return pi(a, shift(b, 1, 0)); }

template<typename... Args>
inline Ptr apps(Ptr fn, Args... args) {
    for (auto arg : {args...}) fn = app(fn, arg);
    return fn;
}

// Application spine
struct Spine {
    Ptr head;
    std::vector<Ptr> args;
};

inline Spine unroll(Ptr t) {
    std::vector<Ptr> args;
    while (auto a = std::get_if<App>(&t->v)) {
        args.push_back(a->arg);
        t = a->fn;
    }
    std::reverse(args.begin(), args.end());
    return {t, std::move(args)};
}

inline Ptr roll(Ptr head, const std::vector<Ptr>& args, size_t start = 0) {
    for (size_t i = start; i < args.size(); ++i) head = app(head, args[i]);
    return head;
}

struct Rule { Ptr lhs, rhs; };
// Optional compiled index for a rewrite system that has many rules. Keep the
// source vector immutable for the lifetime of this index. Candidate ordering
// follows the original vector, preserving first-match rewrite semantics.
struct RuleIndex {
    const std::vector<Rule>* rules;
    const Rule* data;
    size_t size;
    std::vector<Spine> lhs_spines;
    std::unordered_map<std::string, std::vector<size_t>> by_head;

    explicit RuleIndex(const std::vector<Rule>& source)
        : rules(&source), data(source.data()), size(source.size()) {
        lhs_spines.reserve(source.size());
        for (size_t i = 0; i < source.size(); ++i) {
            lhs_spines.push_back(unroll(source[i].lhs));
            if (const auto* head = std::get_if<Sym>(&lhs_spines.back().head->v))
                by_head[head->name].push_back(i);
        }
    }

    void validate() const {
        assert(rules && rules->data() == data && rules->size() == size &&
               "rewrite rules must remain unchanged after RuleIndex construction");
    }
};
using Env = std::map<size_t, Ptr>;

Ptr whnf(Ptr t, const std::vector<Rule>& rules);
bool conv(Ptr t1, Ptr t2, const std::vector<Rule>& rules);
Ptr whnf(Ptr t, const RuleIndex& rules);
bool conv(Ptr t1, Ptr t2, const RuleIndex& rules);

inline Ptr instantiate(Ptr t, const Env& env, size_t depth = 0) {
    return std::visit(match_visitor{
        [&](const PVar& p) -> Ptr {
            auto it = env.find(p.id);
            if (it == env.end()) return t;
            return shift(it->second, static_cast<int>(depth), 0);
        },
        [&](const Pi& p)  -> Ptr { return pi(instantiate(p.dom, env, depth), instantiate(p.cod, env, depth + 1)); },
        [&](const Lam& l) -> Ptr { return lam(instantiate(l.dom, env, depth), instantiate(l.body, env, depth + 1)); },
        [&](const App& a) -> Ptr { return app(instantiate(a.fn, env, depth), instantiate(a.arg, env, depth)); },
        [&](const auto&)  -> Ptr { return t; }
    }, t->v);
}

inline bool match_pat(Ptr pat, Ptr t, Env& env, const std::vector<Rule>& rules) {
    if (auto p = std::get_if<PVar>(&pat->v)) {
        auto it = env.find(p->id);
        if (it != env.end()) return conv(it->second, t, rules);
        env[p->id] = t;
        return true;
    }
    t = whnf(t, rules);
    if (pat->v.index() != t->v.index()) return false;

    return std::visit(match_visitor{
        [&](const Sort& s1) { return s1.k == std::get<Sort>(t->v).k; },
        [&](const BVar& v1) { return v1.idx == std::get<BVar>(t->v).idx; },
        [&](const Sym& s1)  { return s1.name == std::get<Sym>(t->v).name; },
        [&](const App& a1)  {
            const auto& a2 = std::get<App>(t->v);
            return match_pat(a1.fn, a2.fn, env, rules) && match_pat(a1.arg, a2.arg, env, rules);
        },
        [&](const Pi& p1)   {
            const auto& p2 = std::get<Pi>(t->v);
            return match_pat(p1.dom, p2.dom, env, rules) && match_pat(p1.cod, p2.cod, env, rules);
        },
        [&](const Lam& l1)  {
            const auto& l2 = std::get<Lam>(t->v);
            return match_pat(l1.dom, l2.dom, env, rules) && match_pat(l1.body, l2.body, env, rules);
        },
        [&](const PVar&) { return false; }
    }, pat->v);
}

inline bool match_pat(Ptr pat, Ptr t, Env& env, const RuleIndex& rules) {
    if (auto p = std::get_if<PVar>(&pat->v)) {
        auto it = env.find(p->id);
        if (it != env.end()) return conv(it->second, t, rules);
        env[p->id] = t;
        return true;
    }
    t = whnf(t, rules);
    if (pat->v.index() != t->v.index()) return false;

    return std::visit(match_visitor{
        [&](const Sort& s1) { return s1.k == std::get<Sort>(t->v).k; },
        [&](const BVar& v1) { return v1.idx == std::get<BVar>(t->v).idx; },
        [&](const Sym& s1)  { return s1.name == std::get<Sym>(t->v).name; },
        [&](const App& a1)  {
            const auto& a2 = std::get<App>(t->v);
            return match_pat(a1.fn, a2.fn, env, rules) &&
                   match_pat(a1.arg, a2.arg, env, rules);
        },
        [&](const Pi& p1)   {
            const auto& p2 = std::get<Pi>(t->v);
            return match_pat(p1.dom, p2.dom, env, rules) &&
                   match_pat(p1.cod, p2.cod, env, rules);
        },
        [&](const Lam& l1)  {
            const auto& l2 = std::get<Lam>(t->v);
            return match_pat(l1.dom, l2.dom, env, rules) &&
                   match_pat(l1.body, l2.body, env, rules);
        },
        [&](const PVar&) { return false; }
    }, pat->v);
}

inline Ptr whnf(Ptr t, const std::vector<Rule>& rules) {
    while (true) {
        if (auto a = std::get_if<App>(&t->v)) {
            Ptr f = whnf(a->fn, rules);
            if (auto l = std::get_if<Lam>(&f->v)) {
                t = subst(l->body, a->arg, 0);
                continue;
            }
        }

        auto [head, args] = unroll(t);
        bool rewritten = false;
        for (const auto& r : rules) {
            auto [r_head, r_args] = unroll(r.lhs);
            if (args.size() < r_args.size()) continue;

            auto s1 = std::get_if<Sym>(&head->v);
            auto s2 = std::get_if<Sym>(&r_head->v);
            if (!s1 || !s2 || s1->name != s2->name) continue;

            Env env;
            bool matched = true;
            for (size_t i = 0; i < r_args.size(); ++i) {
                if (!match_pat(r_args[i], args[i], env, rules)) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                t = roll(instantiate(r.rhs, env), args, r_args.size());
                rewritten = true;
                break;
            }
        }
        if (rewritten) continue;
        break;
    }
    return t;
}

inline Ptr whnf(Ptr t, const RuleIndex& rules) {
    rules.validate();
    while (true) {
        if (auto a = std::get_if<App>(&t->v)) {
            Ptr f = whnf(a->fn, rules);
            if (auto l = std::get_if<Lam>(&f->v)) {
                t = subst(l->body, a->arg, 0);
                continue;
            }
        }

        auto [head, args] = unroll(t);
        const auto* symbol = std::get_if<Sym>(&head->v);
        if (!symbol) break;
        auto found = rules.by_head.find(symbol->name);
        if (found == rules.by_head.end()) break;

        bool rewritten = false;
        for (size_t i : found->second) {
            const auto& lhs_args = rules.lhs_spines[i].args;
            if (args.size() < lhs_args.size()) continue;
            Env env;
            bool matched = true;
            for (size_t j = 0; j < lhs_args.size(); ++j) {
                if (!match_pat(lhs_args[j], args[j], env, rules)) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                t = roll(instantiate((*rules.rules)[i].rhs, env), args,
                         lhs_args.size());
                rewritten = true;
                break;
            }
        }
        if (!rewritten) break;
    }
    return t;
}

inline Ptr nf(Ptr t, const std::vector<Rule>& rules) {
    t = whnf(t, rules);
    return std::visit(match_visitor{
        [&](const Sort&) -> Ptr { return t; },
        [&](const BVar&) -> Ptr { return t; },
        [&](const Sym&)  -> Ptr { return t; },
        [&](const Pi& p)  -> Ptr { return pi(nf(p.dom, rules), nf(p.cod, rules)); },
        [&](const Lam& l) -> Ptr { return lam(nf(l.dom, rules), nf(l.body, rules)); },
        [&](const App& a) -> Ptr { return app(nf(a.fn, rules), nf(a.arg, rules)); },
        [&](const PVar&) -> Ptr { return t; }
    }, t->v);
}

inline Ptr nf(Ptr t, const RuleIndex& rules) {
    t = whnf(t, rules);
    return std::visit(match_visitor{
        [&](const Sort&) -> Ptr { return t; },
        [&](const BVar&) -> Ptr { return t; },
        [&](const Sym&)  -> Ptr { return t; },
        [&](const Pi& p)  -> Ptr { return pi(nf(p.dom, rules), nf(p.cod, rules)); },
        [&](const Lam& l) -> Ptr { return lam(nf(l.dom, rules), nf(l.body, rules)); },
        [&](const App& a)  -> Ptr { return app(nf(a.fn, rules), nf(a.arg, rules)); },
        [&](const PVar&) -> Ptr { return t; }
    }, t->v);
}

inline bool conv(Ptr t1, Ptr t2, const std::vector<Rule>& rules) {
    t1 = whnf(t1, rules);
    t2 = whnf(t2, rules);
    if (t1 == t2) return true;
    if (t1->v.index() != t2->v.index()) return false;

    return std::visit(match_visitor{
        [&](const Sort& s1) { return s1.k == std::get<Sort>(t2->v).k; },
        [&](const BVar& v1) { return v1.idx == std::get<BVar>(t2->v).idx; },
        [&](const Sym& s1)  { return s1.name == std::get<Sym>(t2->v).name; },
        [&](const App& a1)  {
            const auto& a2 = std::get<App>(t2->v);
            return conv(a1.fn, a2.fn, rules) && conv(a1.arg, a2.arg, rules);
        },
        [&](const Pi& p1)   {
            const auto& p2 = std::get<Pi>(t2->v);
            return conv(p1.dom, p2.dom, rules) && conv(p1.cod, p2.cod, rules);
        },
        [&](const Lam& l1)  {
            const auto& l2 = std::get<Lam>(t2->v);
            return conv(l1.dom, l2.dom, rules) && conv(l1.body, l2.body, rules);
        },
        [&](const PVar& p1) { return p1.id == std::get<PVar>(t2->v).id; }
    }, t1->v);
}

inline bool conv(Ptr t1, Ptr t2, const RuleIndex& rules) {
    t1 = whnf(t1, rules);
    t2 = whnf(t2, rules);
    if (t1 == t2) return true;
    if (t1->v.index() != t2->v.index()) return false;
    return std::visit(match_visitor{
        [&](const Sort& s1) { return s1.k == std::get<Sort>(t2->v).k; },
        [&](const BVar& v1) { return v1.idx == std::get<BVar>(t2->v).idx; },
        [&](const Sym& s1)  { return s1.name == std::get<Sym>(t2->v).name; },
        [&](const App& a1)  {
            const auto& a2 = std::get<App>(t2->v);
            return conv(a1.fn, a2.fn, rules) && conv(a1.arg, a2.arg, rules);
        },
        [&](const Pi& p1)   {
            const auto& p2 = std::get<Pi>(t2->v);
            return conv(p1.dom, p2.dom, rules) && conv(p1.cod, p2.cod, rules);
        },
        [&](const Lam& l1)  {
            const auto& l2 = std::get<Lam>(t2->v);
            return conv(l1.dom, l2.dom, rules) && conv(l1.body, l2.body, rules);
        },
        [&](const PVar& p1) { return p1.id == std::get<PVar>(t2->v).id; }
    }, t1->v);
}

using Context = std::vector<Ptr>;
using Signature = std::map<std::string, Ptr>;
using TypeResult = std::expected<Ptr, std::string>;

inline std::string to_string(const Ptr& t) {
    if (!t) return "<null>";
    return std::visit(match_visitor{
        [](const Sort& s) -> std::string { return s.k == SortKind::Type ? "Type" : "Kind"; },
        [](const BVar& v) -> std::string { return std::format("{}", v.idx); },
        [](const Sym& s)  -> std::string { return s.name; },
        [](const Pi& p)   -> std::string { return std::format("(Π : {}. {})", to_string(p.dom), to_string(p.cod)); },
        [](const Lam& l)  -> std::string { return std::format("(λ : {}. {})", to_string(l.dom), to_string(l.body)); },
        [](const App& a)  -> std::string { return std::format("({} {})", to_string(a.fn), to_string(a.arg)); },
        [](const PVar& p) -> std::string { return std::format("?{}", p.id); }
    }, t->v);
}

template<class Rules>
inline TypeResult infer(const Context& ctx, Ptr t, const Signature& sig, const Rules& rules) {
    if (auto s = std::get_if<Sort>(&t->v)) {
        if (s->k == SortKind::Type) return kind_sort();
        return std::unexpected("Kind has no sort");
    }
    if (auto v = std::get_if<BVar>(&t->v)) {
        if (v->idx >= ctx.size()) return std::unexpected(std::format("Unbound index: {}", v->idx));
        return shift(ctx[v->idx], static_cast<int>(v->idx + 1), 0);
    }
    if (auto s = std::get_if<Sym>(&t->v)) {
        auto it = sig.find(s->name);
        if (it == sig.end()) return std::unexpected(std::format("Undefined symbol: '{}'", s->name));
        return it->second;
    }
    if (auto p = std::get_if<Pi>(&t->v)) {
        auto dom_t = infer(ctx, p->dom, sig, rules);
        if (!dom_t) return dom_t;
        if (!conv(*dom_t, type_sort(), rules)) return std::unexpected("Pi domain must be Type");

        Context new_ctx = ctx;
        new_ctx.insert(new_ctx.begin(), p->dom);
        auto cod_t = infer(new_ctx, p->cod, sig, rules);
        if (!cod_t) return cod_t;

        Ptr cod_whnf = whnf(*cod_t, rules);
        if (auto s = std::get_if<Sort>(&cod_whnf->v)) return make_sort(s->k);
        return std::unexpected("Pi codomain must be Type or Kind");
    }
    if (auto l = std::get_if<Lam>(&t->v)) {
        auto dom_t = infer(ctx, l->dom, sig, rules);
        if (!dom_t) return dom_t;
        if (!conv(*dom_t, type_sort(), rules)) return std::unexpected("Lambda domain must be Type");

        Context new_ctx = ctx;
        new_ctx.insert(new_ctx.begin(), l->dom);
        auto body_t = infer(new_ctx, l->body, sig, rules);
        if (!body_t) return body_t;

        return pi(l->dom, *body_t);
    }
    if (auto a = std::get_if<App>(&t->v)) {
        auto fn_t = infer(ctx, a->fn, sig, rules);
        if (!fn_t) return fn_t;

        Ptr fn_whnf = whnf(*fn_t, rules);
        auto p = std::get_if<Pi>(&fn_whnf->v);
        if (!p) return std::unexpected(std::format("Expected function type, got: {}", to_string(fn_whnf)));

        auto arg_t = infer(ctx, a->arg, sig, rules);
        if (!arg_t) return arg_t;

        if (!conv(*arg_t, p->dom, rules)) {
            return std::unexpected(std::format("Type mismatch: expected {}, got {}",
                                               to_string(p->dom), to_string(*arg_t)));
        }
        return subst(p->cod, a->arg, 0);
    }
    return std::unexpected("Cannot infer type of pattern variable");
}

} // namespace lp
