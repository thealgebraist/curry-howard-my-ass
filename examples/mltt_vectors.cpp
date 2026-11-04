#include "../src/lambda_pi.hpp"

using namespace lp;

int main() {
    std::println("==========================================================");
    std::println("  MLTT Indexed Vectors & Rewriting without Transports");
    std::println("==========================================================\n");

    Signature sig;
    std::vector<Rule> rules;

    // Nat : Type, zero : Nat, succ : Nat -> Nat, add : Nat -> Nat -> Nat
    sig["Nat"]  = type_sort();
    sig["zero"] = sym("Nat");
    sig["succ"] = arrow(sym("Nat"), sym("Nat"));
    sig["add"]  = arrow(sym("Nat"), arrow(sym("Nat"), sym("Nat")));

    // add zero y --> y
    rules.push_back({ apps(sym("add"), sym("zero"), pvar(0)), pvar(0) });
    // add (succ x) y --> succ (add x y)
    rules.push_back({
        apps(sym("add"), app(sym("succ"), pvar(0)), pvar(1)),
        app(sym("succ"), apps(sym("add"), pvar(0), pvar(1)))
    });

    auto zero = sym("zero");
    auto one  = app(sym("succ"), zero);
    auto two  = app(sym("succ"), one);

    // Vec : Nat -> Type
    sig["Vec"]   = arrow(sym("Nat"), type_sort());
    sig["vnil"]  = app(sym("Vec"), zero);
    // vcons : Π (n : Nat). Nat -> Vec n -> Vec (succ n)
    sig["vcons"] = pi(sym("Nat"),
                      arrow(sym("Nat"),
                            arrow(app(sym("Vec"), var(0)),
                                  app(sym("Vec"), app(sym("succ"), var(0))))));

    // vconcat : Π (n : Nat). Π (m : Nat). Vec n -> Vec m -> Vec (add n m)
    sig["vconcat"] = pi(sym("Nat"),
        pi(sym("Nat"),
            arrow(app(sym("Vec"), var(1)),
                arrow(app(sym("Vec"), var(0)),
                    app(sym("Vec"), apps(sym("add"), var(1), var(0)))))));

    // Rewrite rules for vconcat:
    // vconcat zero m vnil ys --> ys
    rules.push_back({
        apps(sym("vconcat"), sym("zero"), pvar(0), sym("vnil"), pvar(1)),
        pvar(1)
    });
    // vconcat (succ n) m (vcons n x xs) ys --> vcons (add n m) x (vconcat n m xs ys)
    rules.push_back({
        apps(sym("vconcat"),
             app(sym("succ"), pvar(0)),
             pvar(1),
             apps(sym("vcons"), pvar(0), pvar(2), pvar(3)),
             pvar(4)),
        apps(sym("vcons"),
             apps(sym("add"), pvar(0), pvar(1)),
             pvar(2),
             apps(sym("vconcat"), pvar(0), pvar(1), pvar(3), pvar(4)))
    });

    Context ctx;

    // Build vector v1 = [one] : Vec 1
    auto v1 = apps(sym("vcons"), zero, one, sym("vnil"));
    // Build vector v2 = [two] : Vec 1
    auto v2 = apps(sym("vcons"), zero, two, sym("vnil"));

    // Check vconcat 1 1 v1 v2
    auto vconcat_app = apps(sym("vconcat"), one, one, v1, v2);
    auto vcat_type = infer(ctx, vconcat_app, sig, rules);

    if (vcat_type) {
        std::println("Inferred type: {}", to_string(*vcat_type));
        std::println("Normalized type: {}", to_string(nf(*vcat_type, rules)));
        std::println("Definitional equality to Vec 2: {}", conv(*vcat_type, app(sym("Vec"), two), rules));
    }

    auto evaluated = nf(vconcat_app, rules);
    std::println("Evaluated result vector: {}", to_string(evaluated));

    return 0;
}
