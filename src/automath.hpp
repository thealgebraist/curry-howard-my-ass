#pragma once

#include "lambda_pi.hpp"

namespace automath {

using namespace lp;

struct CtxItem {
    std::string name;
    Ptr type;
};

struct ContextNode {
    std::string var_name;
    Ptr type;
    std::shared_ptr<ContextNode> parent;
};

enum class LineKind { EB, PN, DEF };

class AutomathBook {
public:
    Signature sig;
    std::vector<Rule> rules;
    std::map<std::string, std::shared_ptr<ContextNode>> indicators;

    AutomathBook() {
        sig["type"] = kind_sort();
    }

    std::vector<CtxItem> get_telescope(const std::string& indicator) {
        std::vector<CtxItem> tel;
        if (indicator == "0" || indicator.empty()) return tel;

        auto it = indicators.find(indicator);
        if (it == indicators.end()) {
            throw std::runtime_error(std::format("Unknown context indicator: '{}'", indicator));
        }

        auto curr = it->second;
        while (curr) {
            tel.push_back({curr->var_name, curr->type});
            curr = curr->parent;
        }
        std::reverse(tel.begin(), tel.end());
        return tel;
    }

    Context build_typing_context(const std::vector<CtxItem>& tel) {
        Context ctx;
        for (const auto& item : tel) {
            ctx.insert(ctx.begin(), item.type);
        }
        return ctx;
    }

    Ptr term_to_pattern_rhs(Ptr t, size_t k) {
        auto replace_indices = [&](this auto&& self, Ptr node, size_t depth) -> Ptr {
            return std::visit(match_visitor{
                [&](const BVar& v) -> Ptr {
                    if (v.idx >= depth && v.idx < depth + k) {
                        size_t pvar_id = (k - 1) - (v.idx - depth);
                        return pvar(pvar_id);
                    }
                    return node;
                },
                [&](const Pi& p)  -> Ptr { return pi(self(p.dom, depth), self(p.cod, depth + 1)); },
                [&](const Lam& l) -> Ptr { return lam(self(l.dom, depth), self(l.body, depth + 1)); },
                [&](const App& a) -> Ptr { return app(self(a.fn, depth), self(a.arg, depth)); },
                [&](const auto&)  -> Ptr { return node; }
            }, node->v);
        };
        return replace_indices(t, 0);
    }

    Ptr abstract_telescope(const std::vector<CtxItem>& tel, Ptr body) {
        for (auto it = tel.rbegin(); it != tel.rend(); ++it) {
            body = pi(it->type, body);
        }
        return body;
    }

    void add_line(const std::string& indicator,
                  const std::string& identifier,
                  LineKind kind,
                  Ptr definiens,
                  Ptr category) {
        auto tel = get_telescope(indicator);
        Context ctx = build_typing_context(tel);

        auto cat_t = infer(ctx, category, sig, rules);
        if (!cat_t) {
            throw std::runtime_error(std::format("In line '{}': category error: {}",
                                                 identifier, cat_t.error()));
        }

        if (kind == LineKind::EB) {
            std::shared_ptr<ContextNode> parent = (indicator == "0" || indicator.empty())
                ? nullptr : indicators[indicator];
            auto node = std::make_shared<ContextNode>(ContextNode{
                .var_name = identifier,
                .type = category,
                .parent = parent
            });
            indicators[identifier] = node;
            std::println("  [EB]  {} * {} := EB : {}", indicator, identifier, to_string(category));
        }
        else if (kind == LineKind::PN) {
            Ptr closed_type = abstract_telescope(tel, category);
            sig[identifier] = closed_type;
            std::println("  [PN]  {} * {} := PN : {}", indicator, identifier, to_string(category));
        }
        else if (kind == LineKind::DEF) {
            auto def_t = infer(ctx, definiens, sig, rules);
            if (!def_t) {
                throw std::runtime_error(std::format("In line '{}': definiens error: {}",
                                                     identifier, def_t.error()));
            }
            if (!conv(*def_t, category, rules)) {
                throw std::runtime_error(std::format("In line '{}': type mismatch. Expected '{}', got '{}'",
                                                     identifier, to_string(category), to_string(*def_t)));
            }

            Ptr closed_type = abstract_telescope(tel, category);
            sig[identifier] = closed_type;

            size_t k = tel.size();
            Ptr lhs = sym(identifier);
            for (size_t i = 0; i < k; ++i) lhs = app(lhs, pvar(i));
            Ptr rhs = term_to_pattern_rhs(definiens, k);
            rules.push_back({lhs, rhs});

            std::println("  [DEF] {} * {} := {} : {}", indicator, identifier, to_string(definiens), to_string(category));
            std::println("        δ-rule: {} --> {}", to_string(lhs), to_string(rhs));
        }
    }
};

} // namespace automath
