#include "../src/lambda_pi.hpp"
#include <cassert>
#include <functional>
#include <type_traits>

// Native C++ template vocabulary for the subset mirrored in the λΠ evaluator.
template<int N> using ic = std::integral_constant<int, N>;
template<class A, class B>
using plus_c = std::integral_constant<int, std::plus<>{}(A::value, B::value)>;
template<class A, class B>
using multiplies_c = std::integral_constant<int, std::multiplies<>{}(A::value, B::value)>;
template<class A, class B>
using equal_c = std::bool_constant<std::equal_to<>{}(A::value, B::value)>;
template<class Cond, class Then, class Else>
using conditional_c = std::conditional_t<Cond::value, Then, Else>;

using CppProgram = conditional_c<
    equal_c<plus_c<ic<2>, ic<3>>, ic<5>>,
    plus_c<ic<2>, multiplies_c<ic<3>, ic<4>>>,
    ic<0>>;
static_assert(CppProgram::value == 14);

using namespace lp;

static Ptr c(std::string f, std::initializer_list<Ptr> xs = {}) {
    Ptr t = sym(std::move(f));
    for (const auto& x : xs) t = app(t, x);
    return t;
}
static Rule rw(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr nat(size_t n) { Ptr t = sym("zero"); while (n--) t = app(sym("succ"), t); return t; }

static size_t nat_value(Ptr t) {
    size_t n = 0;
    while (auto a = std::get_if<App>(&t->v)) {
        auto h = std::get_if<Sym>(&a->fn->v);
        if (!h || h->name != "succ") break;
        ++n;
        t = a->arg;
    }
    return n;
}

static size_t vnat_value(Ptr t) {
    auto [head, args] = unroll(t);
    auto h = std::get_if<Sym>(&head->v);
    assert(h && h->name == "vnat" && args.size() == 1);
    return nat_value(args[0]);
}

static std::string render_template_term(Ptr t) {
    auto [head, args] = unroll(t);
    auto h = std::get_if<Sym>(&head->v);
    if (!h) return "<invalid>";
    if (h->name == "ic" && args.size() == 1)
        return std::format("ic<{}>", nat_value(args[0]));
    if (h->name == "plus_c" && args.size() == 2)
        return std::format("plus_c<{}, {}>", render_template_term(args[0]), render_template_term(args[1]));
    if (h->name == "multiplies_c" && args.size() == 2)
        return std::format("multiplies_c<{}, {}>", render_template_term(args[0]), render_template_term(args[1]));
    if (h->name == "equal_c" && args.size() == 2)
        return std::format("equal_c<{}, {}>", render_template_term(args[0]), render_template_term(args[1]));
    if (h->name == "conditional_c" && args.size() == 3)
        return std::format("conditional_c<{}, {}, {}>", render_template_term(args[0]),
                           render_template_term(args[1]), render_template_term(args[2]));
    return "<invalid>";
}

int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto p = [](size_t i) { return pvar(i); };
    auto arrow_t = [](Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); };
    auto nat_t = sym("Nat"), cxx_t = sym("Cxx"), value_t = sym("Value");

    sig["Nat"] = type_sort();
    sig["zero"] = nat_t;
    sig["succ"] = arrow_t(nat_t, nat_t);
    sig["add"] = arrow_t(nat_t, arrow_t(nat_t, nat_t));
    sig["mul"] = arrow_t(nat_t, arrow_t(nat_t, nat_t));
    rules.push_back(rw(apps(sym("add"), sym("zero"), p(0)), p(0)));
    rules.push_back(rw(apps(sym("add"), app(sym("succ"), p(0)), p(1)),
                       app(sym("succ"), apps(sym("add"), p(0), p(1)))));
    rules.push_back(rw(apps(sym("mul"), sym("zero"), p(0)), sym("zero")));
    rules.push_back(rw(apps(sym("mul"), app(sym("succ"), p(0)), p(1)),
                       apps(sym("add"), p(1), apps(sym("mul"), p(0), p(1)))));

    sig["Cxx"] = type_sort();
    sig["ic"] = arrow_t(nat_t, cxx_t);
    sig["plus_c"] = arrow_t(cxx_t, arrow_t(cxx_t, cxx_t));
    sig["multiplies_c"] = arrow_t(cxx_t, arrow_t(cxx_t, cxx_t));
    sig["equal_c"] = arrow_t(cxx_t, arrow_t(cxx_t, cxx_t));
    sig["conditional_c"] = arrow_t(cxx_t, arrow_t(cxx_t, arrow_t(cxx_t, cxx_t)));

    sig["Value"] = type_sort();
    sig["vnat"] = arrow_t(nat_t, value_t);
    sig["vtrue"] = value_t;
    sig["vfalse"] = value_t;
    sig["evalCpp"] = arrow_t(cxx_t, value_t);
    sig["addValues"] = arrow_t(value_t, arrow_t(value_t, value_t));
    sig["multiplyValues"] = arrow_t(value_t, arrow_t(value_t, value_t));
    sig["equalValues"] = arrow_t(value_t, arrow_t(value_t, value_t));
    sig["equalNats"] = arrow_t(nat_t, arrow_t(nat_t, value_t));
    sig["selectType"] = arrow_t(value_t, arrow_t(cxx_t, arrow_t(cxx_t, value_t)));

    // Evaluate C++ integral_constant, arithmetic metafunction aliases, and
    // std::conditional_t lazily. Only the selected branch is evaluated.
    rules.push_back(rw(app(sym("evalCpp"), app(sym("ic"), p(0))), c("vnat", {p(0)})));
    rules.push_back(rw(app(sym("evalCpp"), apps(sym("plus_c"), p(0), p(1))),
                       apps(sym("addValues"), app(sym("evalCpp"), p(0)), app(sym("evalCpp"), p(1)))));
    rules.push_back(rw(app(sym("evalCpp"), apps(sym("multiplies_c"), p(0), p(1))),
                       apps(sym("multiplyValues"), app(sym("evalCpp"), p(0)), app(sym("evalCpp"), p(1)))));
    rules.push_back(rw(app(sym("evalCpp"), apps(sym("equal_c"), p(0), p(1))),
                       apps(sym("equalValues"), app(sym("evalCpp"), p(0)), app(sym("evalCpp"), p(1)))));
    rules.push_back(rw(app(sym("evalCpp"), apps(sym("conditional_c"), p(0), p(1), p(2))),
                       apps(sym("selectType"), app(sym("evalCpp"), p(0)), p(1), p(2))));
    rules.push_back(rw(apps(sym("addValues"), c("vnat", {p(0)}), c("vnat", {p(1)})),
                       c("vnat", {apps(sym("add"), p(0), p(1))})));
    rules.push_back(rw(apps(sym("multiplyValues"), c("vnat", {p(0)}), c("vnat", {p(1)})),
                       c("vnat", {apps(sym("mul"), p(0), p(1))})));
    rules.push_back(rw(apps(sym("equalValues"), c("vnat", {p(0)}), c("vnat", {p(1)})),
                       apps(sym("equalNats"), p(0), p(1))));
    rules.push_back(rw(apps(sym("equalNats"), sym("zero"), sym("zero")), sym("vtrue")));
    rules.push_back(rw(apps(sym("equalNats"), sym("zero"), app(sym("succ"), p(0))), sym("vfalse")));
    rules.push_back(rw(apps(sym("equalNats"), app(sym("succ"), p(0)), sym("zero")), sym("vfalse")));
    rules.push_back(rw(apps(sym("equalNats"), app(sym("succ"), p(0)), app(sym("succ"), p(1))),
                       apps(sym("equalNats"), p(0), p(1))));
    rules.push_back(rw(apps(sym("selectType"), sym("vtrue"), p(0), p(1)), app(sym("evalCpp"), p(0))));
    rules.push_back(rw(apps(sym("selectType"), sym("vfalse"), p(0), p(1)), app(sym("evalCpp"), p(1))));

    // Same program in the C++ template AST and λΠ syntax:
    // conditional(equal(2 + 3, 5), 2 + 3 * 4, 0) == 14.
    Ptr cxx_program = apps(sym("conditional_c"),
        apps(sym("equal_c"), apps(sym("plus_c"), app(sym("ic"), nat(2)), app(sym("ic"), nat(3))), app(sym("ic"), nat(5))),
        apps(sym("plus_c"), app(sym("ic"), nat(2)),
             apps(sym("multiplies_c"), app(sym("ic"), nat(3)), app(sym("ic"), nat(4)))),
        app(sym("ic"), nat(0)));
    Ptr kernel_result = nf(app(sym("evalCpp"), cxx_program), rules);
    Ptr expected = c("vnat", {nat(14)});
    auto inferred = infer({}, app(sym("evalCpp"), cxx_program), sig, rules);
    assert(inferred && conv(*inferred, value_t, rules));
    assert(conv(kernel_result, expected, rules));
    const size_t kernel_value = vnat_value(kernel_result);
    assert(CppProgram::value == static_cast<int>(kernel_value));

    std::println("C++ template AST: {}", render_template_term(cxx_program));
    std::println("λΠ kernel result: {}", kernel_value);
    std::println("Native C++ template result: {}", CppProgram::value);
    std::println("Kernel term type checks as Value: {}", static_cast<bool>(inferred));
}
