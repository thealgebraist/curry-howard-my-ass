#include "../src/automath.hpp"

using namespace automath;

int main() {
    std::println("=========================================================");
    std::println("     Automath (AUT-68 / AUT-QE) on λΠ Rewriting Core      ");
    std::println("=========================================================\n");

    AutomathBook aut;

    std::println("--- Segment 1: Foundations of Propositional Logic ---");

    // 0 * prop := PN : type
    aut.add_line("0", "prop", LineKind::PN, nullptr, type_sort());

    // 0 * p := EB : prop
    aut.add_line("0", "p", LineKind::EB, nullptr, sym("prop"));

    // p * proof := PN : type
    aut.add_line("p", "proof", LineKind::PN, nullptr, type_sort());

    // Context for implication: a : prop, b : prop
    aut.add_line("0", "a", LineKind::EB, nullptr, sym("prop"));
    aut.add_line("a", "b", LineKind::EB, nullptr, sym("prop"));

    // b * imp := PN : prop
    aut.add_line("b", "imp", LineKind::PN, nullptr, sym("prop"));

    // Modus Ponens: proof(imp(a, b)) -> proof(a) -> proof(b)
    auto imp_ab = apps(sym("imp"), var(1), var(0));
    auto proof_imp_ab = app(sym("proof"), imp_ab);
    auto proof_a = app(sym("proof"), var(1));
    auto proof_b = app(sym("proof"), var(0));

    aut.add_line("b", "mp", LineKind::PN, nullptr, arrow(proof_imp_ab, arrow(proof_a, proof_b)));

    // Implication Introduction: (proof(a) -> proof(b)) -> proof(imp(a, b))
    aut.add_line("b", "imp_intro", LineKind::PN, nullptr, arrow(arrow(proof_a, proof_b), proof_imp_ab));

    std::println("\n--- Segment 2: Theorems Verified with δ-Reduction ---");

    // Theorem 1: Self-Implication: for all a : prop, proof(imp(a, a))
    auto proof_a_in_a = app(sym("proof"), var(0));
    auto id_proof = lam(proof_a_in_a, var(0));
    auto call_imp_intro_aa = apps(sym("imp_intro"), var(0), var(0), id_proof);
    auto expected_type_aa = app(sym("proof"), apps(sym("imp"), var(0), var(0)));

    aut.add_line("a", "id_imp", LineKind::DEF, call_imp_intro_aa, expected_type_aa);

    // Context for hypothetical syllogism: a : prop, b : prop, c : prop, h1 : proof(imp a b), h2 : proof(imp b c)
    aut.add_line("b", "c", LineKind::EB, nullptr, sym("prop"));

    auto proof_imp_ab_in_abc = app(sym("proof"), apps(sym("imp"), var(2), var(1)));
    aut.add_line("c", "h1", LineKind::EB, nullptr, proof_imp_ab_in_abc);

    auto proof_imp_bc_in_abch1 = app(sym("proof"), apps(sym("imp"), var(2), var(1)));
    aut.add_line("h1", "h2", LineKind::EB, nullptr, proof_imp_bc_in_abch1);

    // In context [a, b, c, h1, h2]: prove proof(imp(a, c))
    auto proof_a_in_5 = app(sym("proof"), var(4));
    auto step1 = apps(sym("mp"), var(5), var(4), var(2), var(0)); // mp a b h1 x
    auto step2 = apps(sym("mp"), var(4), var(3), var(1), step1);  // mp b c h2 step1
    auto comp_fn = lam(proof_a_in_5, step2);                       // [x : proof a] step2

    auto trans_proof = apps(sym("imp_intro"), var(4), var(2), comp_fn);
    auto trans_type = app(sym("proof"), apps(sym("imp"), var(4), var(2)));

    aut.add_line("h2", "syllogism", LineKind::DEF, trans_proof, trans_type);

    std::println("\n--- Segment 3: Executing δ-Reductions ---");

    aut.add_line("0", "P", LineKind::PN, nullptr, sym("prop"));
    aut.add_line("0", "Q", LineKind::PN, nullptr, sym("prop"));
    aut.add_line("0", "R", LineKind::PN, nullptr, sym("prop"));
    aut.add_line("0", "hPQ", LineKind::PN, nullptr, app(sym("proof"), apps(sym("imp"), sym("P"), sym("Q"))));
    aut.add_line("0", "hQR", LineKind::PN, nullptr, app(sym("proof"), apps(sym("imp"), sym("Q"), sym("R"))));

    Context empty_ctx;
    auto syl_term = apps(sym("syllogism"), sym("P"), sym("Q"), sym("R"), sym("hPQ"), sym("hQR"));
    std::println("Syllogism term: {}", to_string(syl_term));

    auto syl_type = infer(empty_ctx, syl_term, aut.sig, aut.rules);
    if (syl_type) {
        std::println("Inferred type: {}", to_string(*syl_type));
        auto expected_PR = app(sym("proof"), apps(sym("imp"), sym("P"), sym("R")));
        std::println("Matches proof(imp(P, R))? {}", conv(*syl_type, expected_PR, aut.rules));
    }

    auto syl_reduced = nf(syl_term, aut.rules);
    std::println("Syllogism unfolded via δ-rewriting:\n  {}", to_string(syl_reduced));

    return 0;
}
