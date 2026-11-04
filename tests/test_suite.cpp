#include "../src/lambda_pi.hpp"
#include "../src/automath.hpp"
#include <cassert>

using namespace lp;
using namespace automath;

// Test framework utilities
static size_t tests_run = 0;
static size_t tests_passed = 0;

#define TEST_CASE(name) void name(); \
    struct Register_##name { \
        Register_##name() { \
            std::println("--- Running Test: {} ---", #name); \
            tests_run++; \
            try { \
                name(); \
                tests_passed++; \
                std::println("  \033[32m[PASS]\033[0m {}\n", #name); \
            } catch (const std::exception& e) { \
                std::println("  \033[31m[FAIL]\033[0m {}: {}\n", #name, e.what()); \
            } \
        } \
    } reg_##name; \
    void name()

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) throw std::runtime_error(std::format("Assertion failed: '{}' at line {}", #expr, __LINE__)); \
} while(0)

#define ASSERT_CONV(t1, t2, rules) do { \
    if (!conv((t1), (t2), (rules))) { \
        throw std::runtime_error(std::format("Conversion failed: '{}' ≢ '{}' at line {}", \
                                             to_string(t1), to_string(t2), __LINE__)); \
    } \
} while(0)

#define ASSERT_NOT_CONV(t1, t2, rules) do { \
    if (conv((t1), (t2), (rules))) { \
        throw std::runtime_error(std::format("Expected distinct terms: '{}' ≡ '{}' at line {}", \
                                             to_string(t1), to_string(t2), __LINE__)); \
    } \
} while(0)

// Helper to construct natural number terms
inline Ptr nat_term(size_t n) {
    Ptr t = sym("zero");
    for (size_t i = 0; i < n; ++i) t = app(sym("succ"), t);
    return t;
}

// ----------------------------------------------------------------------------
// Test 1: Basic λ-calculus (β-reduction, substitution, Church Booleans)
// ----------------------------------------------------------------------------
TEST_CASE(test_lambda_pure) {
    Signature sig;
    std::vector<Rule> rules;

    sig["A"] = type_sort();
    sig["B"] = type_sort();
    sig["a"] = sym("A");
    sig["b"] = sym("B");

    // Identity function: (λ x : A. x) a ⟶ a
    auto id_A = lam(sym("A"), var(0));
    auto id_app = app(id_A, sym("a"));
    ASSERT_CONV(id_app, sym("a"), rules);

    // K combinator: (λ x : A. λ y : B. x) a b ⟶ a
    auto k_comb = lam(sym("A"), lam(sym("B"), var(1)));
    auto k_app = apps(k_comb, sym("a"), sym("b"));
    ASSERT_CONV(k_app, sym("a"), rules);

    // S combinator: (λ x. λ y. λ z. (x z) (y z))
    // S K K x ⟶ x
    auto s_comb = lam(arrow(sym("A"), arrow(sym("B"), sym("A"))), // x
        lam(arrow(sym("A"), sym("B")),                             // y
            lam(sym("A"),                                         // z
                apps(app(var(2), var(0)), app(var(1), var(0))))));

    auto skk = apps(s_comb, k_comb, lam(sym("A"), sym("b")), sym("a"));
    ASSERT_CONV(skk, sym("a"), rules);
}

// ----------------------------------------------------------------------------
// Test 2: λN Arithmetic & System T (Primitive Recursion on Nat)
// ----------------------------------------------------------------------------
TEST_CASE(test_lambda_n_arithmetic) {
    Signature sig;
    std::vector<Rule> rules;

    sig["Nat"]  = type_sort();
    sig["zero"] = sym("Nat");
    sig["succ"] = arrow(sym("Nat"), sym("Nat"));
    sig["add"]  = arrow(sym("Nat"), arrow(sym("Nat"), sym("Nat")));
    sig["mult"] = arrow(sym("Nat"), arrow(sym("Nat"), sym("Nat")));
    sig["pred"] = arrow(sym("Nat"), sym("Nat"));
    sig["fact"] = arrow(sym("Nat"), sym("Nat"));

    // add 0 y ⟶ y
    rules.push_back({ apps(sym("add"), sym("zero"), pvar(0)), pvar(0) });
    // add (S x) y ⟶ S (add x y)
    rules.push_back({
        apps(sym("add"), app(sym("succ"), pvar(0)), pvar(1)),
        app(sym("succ"), apps(sym("add"), pvar(0), pvar(1)))
    });

    // mult 0 y ⟶ 0
    rules.push_back({ apps(sym("mult"), sym("zero"), pvar(0)), sym("zero") });
    // mult (S x) y ⟶ add y (mult x y)
    rules.push_back({
        apps(sym("mult"), app(sym("succ"), pvar(0)), pvar(1)),
        apps(sym("add"), pvar(1), apps(sym("mult"), pvar(0), pvar(1)))
    });

    // pred 0 ⟶ 0
    rules.push_back({ app(sym("pred"), sym("zero")), sym("zero") });
    // pred (S x) ⟶ x
    rules.push_back({ app(sym("pred"), app(sym("succ"), pvar(0))), pvar(0) });

    // fact 0 ⟶ 1
    rules.push_back({ app(sym("fact"), sym("zero")), nat_term(1) });
    // fact (S x) ⟶ mult (S x) (fact x)
    rules.push_back({
        app(sym("fact"), app(sym("succ"), pvar(0))),
        apps(sym("mult"), app(sym("succ"), pvar(0)), app(sym("fact"), pvar(0)))
    });

    // Addition: 3 + 4 == 7
    ASSERT_CONV(apps(sym("add"), nat_term(3), nat_term(4)), nat_term(7), rules);

    // Multiplication: 3 * 5 == 15
    ASSERT_CONV(apps(sym("mult"), nat_term(3), nat_term(5)), nat_term(15), rules);

    // Predecessor: pred 5 == 4, pred 0 == 0
    ASSERT_CONV(app(sym("pred"), nat_term(5)), nat_term(4), rules);
    ASSERT_CONV(app(sym("pred"), nat_term(0)), nat_term(0), rules);

    // Factorial: 4! = 24
    ASSERT_CONV(app(sym("fact"), nat_term(4)), nat_term(24), rules);
}

// ----------------------------------------------------------------------------
// Test 2b: Compiled rewrite-rule lookup preserves vector semantics
// ----------------------------------------------------------------------------
TEST_CASE(test_indexed_rewrite_rules) {
    Signature sig;
    std::vector<Rule> rules;
    sig["Nat"] = type_sort();
    sig["zero"] = sym("Nat");
    sig["succ"] = arrow(sym("Nat"), sym("Nat"));
    sig["add"] = arrow(sym("Nat"), arrow(sym("Nat"), sym("Nat")));
    rules.push_back({apps(sym("add"), sym("zero"), pvar(0)), pvar(0)});
    rules.push_back({apps(sym("add"), app(sym("succ"), pvar(0)), pvar(1)),
                     app(sym("succ"), apps(sym("add"), pvar(0), pvar(1)))});

    // Unrelated rule heads exercise dispatch through the compiled index.
    for (unsigned i = 0; i < 128; ++i) {
        auto name = "noise" + std::to_string(i);
        rules.push_back({app(sym(name), pvar(0)), pvar(0)});
    }
    RuleIndex indexed(rules);
    Ptr term = apps(sym("add"), nat_term(3), nat_term(4));
    Ptr expected = nat_term(7);
    ASSERT_CONV(nf(term, rules), expected, rules);
    ASSERT_CONV(nf(term, indexed), expected, indexed);

    auto inferred = infer({}, term, sig, indexed);
    ASSERT_TRUE(inferred.has_value());
    ASSERT_CONV(*inferred, sym("Nat"), indexed);

    // The index preserves source order when rules overlap.
    sig["pick"] = arrow(sym("Nat"), sym("Nat"));
    std::vector<Rule> ordered = {
        {app(sym("pick"), pvar(0)), sym("zero")},
        {app(sym("pick"), sym("zero")), app(sym("succ"), sym("zero"))}
    };
    RuleIndex ordered_index(ordered);
    ASSERT_CONV(app(sym("pick"), sym("zero")), sym("zero"), ordered_index);
}

// ----------------------------------------------------------------------------
// Test 3: Ackermann Function (Higher-order primitive recursion beyond PR)
// ----------------------------------------------------------------------------
TEST_CASE(test_ackermann) {
    Signature sig;
    std::vector<Rule> rules;

    sig["Nat"]  = type_sort();
    sig["zero"] = sym("Nat");
    sig["succ"] = arrow(sym("Nat"), sym("Nat"));
    sig["ack"]  = arrow(sym("Nat"), arrow(sym("Nat"), sym("Nat")));

    // ack 0 n ⟶ S n
    rules.push_back({
        apps(sym("ack"), sym("zero"), pvar(0)),
        app(sym("succ"), pvar(0))
    });
    // ack (S m) 0 ⟶ ack m 1
    rules.push_back({
        apps(sym("ack"), app(sym("succ"), pvar(0)), sym("zero")),
        apps(sym("ack"), pvar(0), nat_term(1))
    });
    // ack (S m) (S n) ⟶ ack m (ack (S m) n)
    rules.push_back({
        apps(sym("ack"), app(sym("succ"), pvar(0)), app(sym("succ"), pvar(1))),
        apps(sym("ack"), pvar(0), apps(sym("ack"), app(sym("succ"), pvar(0)), pvar(1)))
    });

    // ack(1, 2) = 4
    ASSERT_CONV(apps(sym("ack"), nat_term(1), nat_term(2)), nat_term(4), rules);
    // ack(2, 2) = 7
    ASSERT_CONV(apps(sym("ack"), nat_term(2), nat_term(2)), nat_term(7), rules);
    // ack(2, 3) = 9
    ASSERT_CONV(apps(sym("ack"), nat_term(2), nat_term(3)), nat_term(9), rules);
}

// ----------------------------------------------------------------------------
// Test 4: Generic Recursor rec_nat for System T
// ----------------------------------------------------------------------------
TEST_CASE(test_system_t_recursor) {
    Signature sig;
    std::vector<Rule> rules;

    sig["Nat"]     = type_sort();
    sig["zero"]    = sym("Nat");
    sig["succ"]    = arrow(sym("Nat"), sym("Nat"));

    // rec_nat : Π (P : Nat -> Type). P 0 -> (Π (n : Nat). P n -> P (succ n)) -> Π (n : Nat). P n
    sig["rec_nat"] = pi(arrow(sym("Nat"), type_sort()), // P
        arrow(app(var(0), sym("zero")),                  // P 0
            arrow(pi(sym("Nat"), arrow(app(var(2), var(0)), app(var(2), app(sym("succ"), var(0))))), // step
                pi(sym("Nat"), app(var(3), var(0))))));   // Π n. P n

    // rec_nat P z s zero ⟶ z
    rules.push_back({
        apps(sym("rec_nat"), pvar(0), pvar(1), pvar(2), sym("zero")),
        pvar(1)
    });
    // rec_nat P z s (succ n) ⟶ s n (rec_nat P z s n)
    rules.push_back({
        apps(sym("rec_nat"), pvar(0), pvar(1), pvar(2), app(sym("succ"), pvar(3))),
        apps(pvar(2), pvar(3), apps(sym("rec_nat"), pvar(0), pvar(1), pvar(2), pvar(3)))
    });

    // Define double using rec_nat:
    // double = rec_nat (λ n. Nat) 0 (λ n r. succ (succ r))
    auto const_nat = lam(sym("Nat"), sym("Nat"));
    auto step_double = lam(sym("Nat"), lam(sym("Nat"), app(sym("succ"), app(sym("succ"), var(0)))));
    auto double_fn = lam(sym("Nat"), apps(sym("rec_nat"), const_nat, sym("zero"), step_double, var(0)));

    ASSERT_CONV(app(double_fn, nat_term(0)), nat_term(0), rules);
    ASSERT_CONV(app(double_fn, nat_term(3)), nat_term(6), rules);
    ASSERT_CONV(app(double_fn, nat_term(5)), nat_term(10), rules);
}

// ----------------------------------------------------------------------------
// Test 5: λP / Dependent Types (Vectors, Def-Equality, Type-Errors)
// ----------------------------------------------------------------------------
TEST_CASE(test_lambda_p_dependent_types) {
    Signature sig;
    std::vector<Rule> rules;

    sig["Nat"]  = type_sort();
    sig["zero"] = sym("Nat");
    sig["succ"] = arrow(sym("Nat"), sym("Nat"));
    sig["add"]  = arrow(sym("Nat"), arrow(sym("Nat"), sym("Nat")));

    rules.push_back({ apps(sym("add"), sym("zero"), pvar(0)), pvar(0) });
    rules.push_back({
        apps(sym("add"), app(sym("succ"), pvar(0)), pvar(1)),
        app(sym("succ"), apps(sym("add"), pvar(0), pvar(1)))
    });

    sig["Vec"]   = arrow(sym("Nat"), type_sort());
    sig["vnil"]  = app(sym("Vec"), sym("zero"));
    sig["vcons"] = pi(sym("Nat"),
                      arrow(sym("Nat"),
                            arrow(app(sym("Vec"), var(0)),
                                  app(sym("Vec"), app(sym("succ"), var(0))))));

    Context ctx;

    // Check type of vnil : Vec 0
    auto t_vnil = infer(ctx, sym("vnil"), sig, rules);
    ASSERT_TRUE(t_vnil.has_value());
    ASSERT_CONV(*t_vnil, app(sym("Vec"), sym("zero")), rules);

    // v1 = vcons 0 42 vnil : Vec 1
    auto v1 = apps(sym("vcons"), sym("zero"), nat_term(42), sym("vnil"));
    auto t_v1 = infer(ctx, v1, sig, rules);
    ASSERT_TRUE(t_v1.has_value());
    ASSERT_CONV(*t_v1, app(sym("Vec"), nat_term(1)), rules);

    // Verify that Vec (add 1 1) ≡ Vec 2
    auto vec_1_plus_1 = app(sym("Vec"), apps(sym("add"), nat_term(1), nat_term(1)));
    auto vec_2 = app(sym("Vec"), nat_term(2));
    ASSERT_CONV(vec_1_plus_1, vec_2, rules);

    // Negative Test: Type mismatch must fail inference!
    // Try passing a Vec 1 where Vec 0 is expected
    auto bad_vcons = apps(sym("vcons"), sym("zero"), nat_term(99), v1);
    auto t_bad = infer(ctx, bad_vcons, sig, rules);
    ASSERT_TRUE(!t_bad.has_value()); // Must fail!

    // Negative Test: Unbound variable index must fail
    auto unbound_term = var(999);
    auto t_unbound = infer(ctx, unbound_term, sig, rules);
    ASSERT_TRUE(!t_unbound.has_value());

    // Negative Test: Applying non-function must fail
    auto bad_app = app(nat_term(0), nat_term(0));
    auto t_bad_app = infer(ctx, bad_app, sig, rules);
    ASSERT_TRUE(!t_bad_app.has_value());
}

// ----------------------------------------------------------------------------
// Test 6: Automath Book Engine & δ-Rewriting
// ----------------------------------------------------------------------------
TEST_CASE(test_automath_full_pipeline) {
    AutomathBook aut;

    // Set up propositional logic in Automath
    aut.add_line("0", "prop", LineKind::PN, nullptr, type_sort());
    aut.add_line("0", "p", LineKind::EB, nullptr, sym("prop"));
    aut.add_line("p", "proof", LineKind::PN, nullptr, type_sort());

    aut.add_line("0", "a", LineKind::EB, nullptr, sym("prop"));
    aut.add_line("a", "b", LineKind::EB, nullptr, sym("prop"));
    aut.add_line("b", "imp", LineKind::PN, nullptr, sym("prop"));

    auto proof_imp_ab = app(sym("proof"), apps(sym("imp"), var(1), var(0)));
    auto proof_a = app(sym("proof"), var(1));
    auto proof_b = app(sym("proof"), var(0));
    aut.add_line("b", "mp", LineKind::PN, nullptr, arrow(proof_imp_ab, arrow(proof_a, proof_b)));
    aut.add_line("b", "imp_intro", LineKind::PN, nullptr, arrow(arrow(proof_a, proof_b), proof_imp_ab));

    // Prove self-implication: a * id_imp := imp_intro(a, a, λ x. x)
    auto id_proof = lam(app(sym("proof"), var(0)), var(0));
    auto call_imp_intro = apps(sym("imp_intro"), var(0), var(0), id_proof);
    auto exp_type = app(sym("proof"), apps(sym("imp"), var(0), var(0)));
    aut.add_line("a", "id_imp", LineKind::DEF, call_imp_intro, exp_type);

    // Verify δ-reduction on a concrete proposition P : prop
    aut.add_line("0", "P", LineKind::PN, nullptr, sym("prop"));

    Context empty_ctx;
    auto test_call = apps(sym("id_imp"), sym("P"));
    auto res_type = infer(empty_ctx, test_call, aut.sig, aut.rules);
    ASSERT_TRUE(res_type.has_value());

    auto expected_P_imp_P = app(sym("proof"), apps(sym("imp"), sym("P"), sym("P")));
    ASSERT_CONV(*res_type, expected_P_imp_P, aut.rules);

    // Full normalization should unfold id_imp to imp_intro P P (λ x. x)
    auto unfolded = nf(test_call, aut.rules);
    auto expected_unfolded = apps(sym("imp_intro"), sym("P"), sym("P"), lam(app(sym("proof"), sym("P")), var(0)));
    ASSERT_CONV(unfolded, expected_unfolded, aut.rules);
}

// ----------------------------------------------------------------------------
// Test Runner Main
// ----------------------------------------------------------------------------
int main() {
    std::println("==========================================================");
    std::println("     λN / λP / Automath Comprehensive Test Suite          ");
    std::println("==========================================================\n");

    std::println("Results: {} / {} tests passed.", tests_passed, tests_run);

    if (tests_passed == tests_run) {
        std::println("\033[32mALL TESTS PASSED SUCCESSFULLY!\033[0m");
        return 0;
    } else {
        std::println("\033[31mSOME TESTS FAILED!\033[0m");
        return 1;
    }
}
