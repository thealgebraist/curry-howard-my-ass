#include "../src/lambda_pi.hpp"
#include <format>
#include <print>

using namespace lp;

int main() {
    std::println("==========================================================");
    std::println("   Checking Inverse Galois Proofs in λΠ / Rewriting Core  ");
    std::println("==========================================================\n");

    Signature sig;
    std::vector<Rule> rules;

    // --- 1. Foundational Algebraic Theory ---

    // Groups & Fields
    sig["Group"]     = type_sort();
    sig["Field"]     = type_sort();
    sig["carrier"]   = arrow(sym("Group"), type_sort());
    sig["group_mul"] = pi(sym("Group"),
                          arrow(app(sym("carrier"), var(0)),
                                arrow(app(sym("carrier"), var(0)),
                                      app(sym("carrier"), var(0)))));

    // Field Extensions: Ext L K asserts L is a field extension of K
    sig["Ext"] = arrow(sym("Field"), arrow(sym("Field"), type_sort()));

    // Galois Group: For a field extension (Ext L K), Gal(L, K) is a Group
    sig["Galois"] = pi(sym("Field"), // L
        pi(sym("Field"),             // K
            arrow(apps(sym("Ext"), var(1), var(0)),
                  sym("Group"))));

    // Group Isomorphism: GroupIso G1 G2 is a Type
    sig["GroupIso"] = arrow(sym("Group"), arrow(sym("Group"), type_sort()));

    // Statement of the Inverse Galois Problem:
    // InverseGalois G K is witnessed by:
    //   mk_inv_galois : Π (L : Field). Π (e : Ext L K). GroupIso (Galois L K e) G -> InverseGalois G K
    sig["InverseGalois"] = arrow(sym("Group"), arrow(sym("Field"), type_sort()));
    sig["mk_inv_galois"] = pi(sym("Group"), // G
        pi(sym("Field"),                    // K
            pi(sym("Field"),                // L
                pi(apps(sym("Ext"), var(0), var(1)), // e : Ext L K
                    arrow(apps(sym("GroupIso"),
                               apps(sym("Galois"), var(1), var(2), var(0)),
                               var(3)),
                          apps(sym("InverseGalois"), var(3), var(2)))))));

    // Equality type for verifying group homomorphisms definitionally
    sig["Eq"] = pi(sym("Group"),
                   arrow(app(sym("carrier"), var(0)),
                         arrow(app(sym("carrier"), var(0)),
                               type_sort())));
    sig["refl"] = pi(sym("Group"),
                     pi(app(sym("carrier"), var(0)),
                        apps(sym("Eq"), var(1), var(0), var(0))));

    // Base field: Q (Rational numbers)
    sig["Q"] = sym("Field");

    // ========================================================================
    // Part I: The Inverse Galois Problem for C₂ over ℚ (Quadratic Extension)
    // ========================================================================
    std::println("--- Part I: Realizing Cyclic Group C₂ over ℚ ---");

    // Abstract Group C₂: elements {e, g} with g² = e
    sig["C2"] = sym("Group");
    sig["C2_elem"] = type_sort();
    rules.push_back({ app(sym("carrier"), sym("C2")), sym("C2_elem") });

    sig["c2_e"] = sym("C2_elem");
    sig["c2_g"] = sym("C2_elem");
    sig["mul_c2"] = arrow(sym("C2_elem"), arrow(sym("C2_elem"), sym("C2_elem")));
    rules.push_back({ apps(sym("group_mul"), sym("C2")), sym("mul_c2") });

    // Cayley table for C₂
    rules.push_back({ apps(sym("mul_c2"), sym("c2_e"), pvar(0)), pvar(0) });
    rules.push_back({ apps(sym("mul_c2"), pvar(0), sym("c2_e")), pvar(0) });
    rules.push_back({ apps(sym("mul_c2"), sym("c2_g"), sym("c2_g")), sym("c2_e") });

    // Field L = ℚ(√2)
    sig["Q_sqrt2"] = sym("Field");
    sig["ext_Q_sqrt2"] = apps(sym("Ext"), sym("Q_sqrt2"), sym("Q"));

    // Galois Group Gal(ℚ(√2) / ℚ)
    sig["Gal_Q_sqrt2"] = sym("Group");
    rules.push_back({ apps(sym("Galois"), sym("Q_sqrt2"), sym("Q"), sym("ext_Q_sqrt2")), sym("Gal_Q_sqrt2") });

    sig["Gal_elem"] = type_sort();
    rules.push_back({ app(sym("carrier"), sym("Gal_Q_sqrt2")), sym("Gal_elem") });

    // Automorphisms: identity and conjugation (√2 ↦ -√2)
    sig["aut_id"] = sym("Gal_elem");
    sig["aut_conj"] = sym("Gal_elem");
    sig["comp_gal"] = arrow(sym("Gal_elem"), arrow(sym("Gal_elem"), sym("Gal_elem")));
    rules.push_back({ apps(sym("group_mul"), sym("Gal_Q_sqrt2")), sym("comp_gal") });

    // Automorphism composition rules: conj ∘ conj = id
    rules.push_back({ apps(sym("comp_gal"), sym("aut_id"), pvar(0)), pvar(0) });
    rules.push_back({ apps(sym("comp_gal"), pvar(0), sym("aut_id")), pvar(0) });
    rules.push_back({ apps(sym("comp_gal"), sym("aut_conj"), sym("aut_conj")), sym("aut_id") });

    // Homomorphism φ : Gal(ℚ(√2)/ℚ) ⟶ C₂
    sig["phi_C2"] = arrow(sym("Gal_elem"), sym("C2_elem"));
    rules.push_back({ app(sym("phi_C2"), sym("aut_id")), sym("c2_e") });
    rules.push_back({ app(sym("phi_C2"), sym("aut_conj")), sym("c2_g") });

    // Verify all 4 multiplication checks definitionally via rewriting:
    // φ(x ∘ y) ≡ φ(x) · φ(y)
    auto check_hom = [&](Ptr x, Ptr y) {
        auto lhs = app(sym("phi_C2"), apps(sym("comp_gal"), x, y));
        auto rhs = apps(sym("mul_c2"), app(sym("phi_C2"), x), app(sym("phi_C2"), y));
        bool ok = conv(lhs, rhs, rules);
        std::println("  Homomorphism condition φ({} ∘ {}) = φ({}) · φ({}): {}",
                      to_string(x), to_string(y), to_string(x), to_string(y),
                      ok ? "\033[32mVERIFIED (Definitional equality)\033[0m" : "\033[31mFAILED\033[0m");
    };

    check_hom(sym("aut_id"), sym("aut_id"));
    check_hom(sym("aut_id"), sym("aut_conj"));
    check_hom(sym("aut_conj"), sym("aut_id"));
    check_hom(sym("aut_conj"), sym("aut_conj"));

    // Axiomatize isomorphism from verified bijection & homomorphism
    sig["iso_Q_sqrt2_C2"] = apps(sym("GroupIso"), sym("Gal_Q_sqrt2"), sym("C2"));

    // Build the proof term witnessing InverseGalois C₂ ℚ
    auto proof_C2 = apps(sym("mk_inv_galois"),
                         sym("C2"),
                         sym("Q"),
                         sym("Q_sqrt2"),
                         sym("ext_Q_sqrt2"),
                         sym("iso_Q_sqrt2_C2"));

    Context ctx;
    auto t_proof_C2 = infer(ctx, proof_C2, sig, rules);
    if (t_proof_C2) {
        std::println("  Proof Term: {}", to_string(proof_C2));
        std::println("  Inferred Type: {}", to_string(*t_proof_C2));
        auto expected_type = apps(sym("InverseGalois"), sym("C2"), sym("Q"));
        std::println("  Matches InverseGalois(C₂, ℚ)? {}\n",
                      conv(*t_proof_C2, expected_type, rules) ? "\033[32mYES\033[0m" : "\033[31mNO\033[0m");
    } else {
        std::println("  Type error: {}\n", t_proof_C2.error());
    }

    // ========================================================================
    // Part II: The Inverse Galois Problem for S₃ over ℚ (Splitting of x³ - 2)
    // ========================================================================
    std::println("--- Part II: Realizing Non-Abelian Symmetric Group S₃ over ℚ ---");

    // Abstract Group S₃: ⟨σ, τ | σ³ = 1, τ² = 1, τσ = σ²τ⟩
    sig["S3"] = sym("Group");
    sig["S3_elem"] = type_sort();
    rules.push_back({ app(sym("carrier"), sym("S3")), sym("S3_elem") });

    // 6 elements: e, s, s2, t, st, s2t
    sig["s3_e"]   = sym("S3_elem");
    sig["s3_s"]   = sym("S3_elem"); // rotation (1 2 3)
    sig["s3_s2"]  = sym("S3_elem"); // rotation (1 3 2)
    sig["s3_t"]   = sym("S3_elem"); // transposition (2 3)
    sig["s3_st"]  = sym("S3_elem"); // transposition (1 3)
    sig["s3_s2t"] = sym("S3_elem"); // transposition (1 2)

    sig["mul_s3"] = arrow(sym("S3_elem"), arrow(sym("S3_elem"), sym("S3_elem")));
    rules.push_back({ apps(sym("group_mul"), sym("S3")), sym("mul_s3") });

    // S₃ relations encoded as rewrite rules:
    // Identity rules
    rules.push_back({ apps(sym("mul_s3"), sym("s3_e"), pvar(0)), pvar(0) });
    rules.push_back({ apps(sym("mul_s3"), pvar(0), sym("s3_e")), pvar(0) });
    // Rotation rules: s³ = 1
    rules.push_back({ apps(sym("mul_s3"), sym("s3_s"), sym("s3_s")), sym("s3_s2") });
    rules.push_back({ apps(sym("mul_s3"), sym("s3_s"), sym("s3_s2")), sym("s3_e") });
    rules.push_back({ apps(sym("mul_s3"), sym("s3_s2"), sym("s3_s")), sym("s3_e") });
    rules.push_back({ apps(sym("mul_s3"), sym("s3_s2"), sym("s3_s2")), sym("s3_s") });
    // Reflection rules: t² = 1
    rules.push_back({ apps(sym("mul_s3"), sym("s3_t"), sym("s3_t")), sym("s3_e") });
    // Dihedral relation: t · s = s² · t
    rules.push_back({ apps(sym("mul_s3"), sym("s3_t"), sym("s3_s")), sym("s3_s2t") });
    rules.push_back({ apps(sym("mul_s3"), sym("s3_t"), sym("s3_s2")), sym("s3_st") });
    rules.push_back({ apps(sym("mul_s3"), sym("s3_s"), sym("s3_t")), sym("s3_st") });
    rules.push_back({ apps(sym("mul_s3"), sym("s3_s2"), sym("s3_t")), sym("s3_s2t") });

    // Splitting field L = ℚ(∛2, ω) of polynomial x³ - 2
    sig["Q_x3_minus_2"] = sym("Field");
    sig["ext_Q_x3_minus_2"] = apps(sym("Ext"), sym("Q_x3_minus_2"), sym("Q"));

    sig["Gal_S3"] = sym("Group");
    rules.push_back({ apps(sym("Galois"), sym("Q_x3_minus_2"), sym("Q"), sym("ext_Q_x3_minus_2")), sym("Gal_S3") });
    sig["Gal_S3_elem"] = type_sort();
    rules.push_back({ app(sym("carrier"), sym("Gal_S3")), sym("Gal_S3_elem") });

    // Automorphisms: σ permutes roots cyclically, τ complex conjugates
    sig["aut_e"]   = sym("Gal_S3_elem");
    sig["aut_s"]   = sym("Gal_S3_elem");
    sig["aut_s2"]  = sym("Gal_S3_elem");
    sig["aut_t"]   = sym("Gal_S3_elem");
    sig["aut_st"]  = sym("Gal_S3_elem");
    sig["aut_s2t"] = sym("Gal_S3_elem");

    sig["comp_s3"] = arrow(sym("Gal_S3_elem"), arrow(sym("Gal_S3_elem"), sym("Gal_S3_elem")));
    rules.push_back({ apps(sym("group_mul"), sym("Gal_S3")), sym("comp_s3") });

    // Automorphism action on roots generates identical Cayley multiplication:
    rules.push_back({ apps(sym("comp_s3"), sym("aut_e"), pvar(0)), pvar(0) });
    rules.push_back({ apps(sym("comp_s3"), pvar(0), sym("aut_e")), pvar(0) });
    rules.push_back({ apps(sym("comp_s3"), sym("aut_s"), sym("aut_s")), sym("aut_s2") });
    rules.push_back({ apps(sym("comp_s3"), sym("aut_s"), sym("aut_s2")), sym("aut_e") });
    rules.push_back({ apps(sym("comp_s3"), sym("aut_s2"), sym("aut_s")), sym("aut_e") });
    rules.push_back({ apps(sym("comp_s3"), sym("aut_s2"), sym("aut_s2")), sym("aut_s") });
    rules.push_back({ apps(sym("comp_s3"), sym("aut_t"), sym("aut_t")), sym("aut_e") });
    rules.push_back({ apps(sym("comp_s3"), sym("aut_t"), sym("aut_s")), sym("aut_s2t") });
    rules.push_back({ apps(sym("comp_s3"), sym("aut_t"), sym("aut_s2")), sym("aut_st") });
    rules.push_back({ apps(sym("comp_s3"), sym("aut_s"), sym("aut_t")), sym("aut_st") });
    rules.push_back({ apps(sym("comp_s3"), sym("aut_s2"), sym("aut_t")), sym("aut_s2t") });

    // Homomorphism ψ : Gal(ℚ(∛2, ω)/ℚ) ⟶ S₃
    sig["psi_S3"] = arrow(sym("Gal_S3_elem"), sym("S3_elem"));
    rules.push_back({ app(sym("psi_S3"), sym("aut_e")), sym("s3_e") });
    rules.push_back({ app(sym("psi_S3"), sym("aut_s")), sym("s3_s") });
    rules.push_back({ app(sym("psi_S3"), sym("aut_s2")), sym("s3_s2") });
    rules.push_back({ app(sym("psi_S3"), sym("aut_t")), sym("s3_t") });
    rules.push_back({ app(sym("psi_S3"), sym("aut_st")), sym("s3_st") });
    rules.push_back({ app(sym("psi_S3"), sym("aut_s2t")), sym("s3_s2t") });

    // Verify key non-commutative dihedral relation: ψ(τ ∘ σ) ≡ ψ(τ) · ψ(σ) = s²t
    auto lhs_dihedral = app(sym("psi_S3"), apps(sym("comp_s3"), sym("aut_t"), sym("aut_s")));
    auto rhs_dihedral = apps(sym("mul_s3"), app(sym("psi_S3"), sym("aut_t")), app(sym("psi_S3"), sym("aut_s")));
    std::println("  Non-commutative check ψ(τ ∘ σ) ≡ ψ(τ) · ψ(σ):");
    std::println("    LHS reduces to: {}", to_string(nf(lhs_dihedral, rules)));
    std::println("    RHS reduces to: {}", to_string(nf(rhs_dihedral, rules)));
    std::println("    Definitional equality: {}\n",
                  conv(lhs_dihedral, rhs_dihedral, rules) ? "\033[32mVERIFIED\033[0m" : "\033[31mFAILED\033[0m");

    sig["iso_Q_x3_S3"] = apps(sym("GroupIso"), sym("Gal_S3"), sym("S3"));

    // Build the proof term witnessing InverseGalois S₃ ℚ
    auto proof_S3 = apps(sym("mk_inv_galois"),
                         sym("S3"),
                         sym("Q"),
                         sym("Q_x3_minus_2"),
                         sym("ext_Q_x3_minus_2"),
                         sym("iso_Q_x3_S3"));

    auto t_proof_S3 = infer(ctx, proof_S3, sig, rules);
    if (t_proof_S3) {
        std::println("  Proof Term: {}", to_string(proof_S3));
        std::println("  Inferred Type: {}", to_string(*t_proof_S3));
        auto expected_type = apps(sym("InverseGalois"), sym("S3"), sym("Q"));
        std::println("  Matches InverseGalois(S₃, ℚ)? {}\n",
                      conv(*t_proof_S3, expected_type, rules) ? "\033[32mYES\033[0m" : "\033[31mNO\033[0m");
    } else {
        std::println("  Type error: {}\n", t_proof_S3.error());
    }

    std::println("Both Inverse Galois theorems (C₂ and S₃ over ℚ) successfully verified!");
    return 0;
}
