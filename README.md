# curry-howard-my-ass

> *"The λΠ-calculus is an extension of the simply typed λ-calculus with dependent types. In this calculus, types can depend on terms."*

A minimal, zero-dependency C++23 implementation of:
1. **The $\lambda\Pi$-calculus modulo rewriting ($\lambda\Pi/\mathcal{R}$)**: The foundational micro-kernel of logical frameworks like Dedukti and Lambdapi.
2. **De Bruijn's Automath (AUT-68 / AUT-QE)**: Implemented directly on top of the rewriting core, where Automath's $\delta$-reduction is synthesized directly into term rewriting rules.

---

## Why Term Rewriting for Dependent Types?

In Martin-Löf Type Theory (MLTT) and Automath, definitional equality is computation:
$$A \equiv B \implies \frac{\Gamma \vdash t : A}{\Gamma \vdash t : B}$$

By framing computation as **user-defined rewrite rules** over a bare $\lambda\Pi$ base:
* **No hardcoded eliminators**: Natural numbers, vectors, identity types ($J$), and custom datatypes are just symbols and orthogonal rewrite rules.
* **No explicit transports / casts**: When working with indexed types like `Vec n`, `vconcat 1 1 xs ys` has type `Vec (add 1 1)`. Because `add 1 1` reduces definitionally to `2` via rewrite rules, the type matches `Vec 2` without any propositional equality proofs or transport lemmas!
* **Automath's $\delta$-reduction is just rewriting**: Every definition line in an Automath book (`context * id := def : type`) generates a rewrite rule:
  $$\text{id} \; ?0 \dots ?(k-1) \longrightarrow \text{def}[x_i \mapsto ?i]$$

---

## Features & C++23 Implementation

* **~230 lines core**: The whole $\lambda\Pi/\mathcal{R}$ engine fits in a single readable header ([`src/lambda_pi.hpp`](src/lambda_pi.hpp)).
* **Modern C++23**:
  * `std::expected<Ptr, std::string>` for monadic type error propagation without exceptions.
  * `std::variant` + `std::visit` with pattern matching for AST nodes.
  * Structured bindings for head/spine decomposition (`auto [head, args] = unroll(t)`).
  * `std::println` and `std::format` for clean output.
* **Automath Module** ([`src/automath.hpp`](src/automath.hpp)):
  * Supports **EB lines** (empty block / context extension), **PN lines** (primitive notions / axioms), and **DEF lines** (theorems / definitions).
  * Automatically compiles definition lines into $\delta$-reduction rewrite rules.

---

## Why is this so much easier than traditional proof assistants?

### 1. Eliminating "Transport Hell" (The Poincaré Principle)
Henri Poincaré noted that *"deduction should not do the work of computation."*
* In standard systems (Coq, Lean, Agda), non-structural algebraic equations ($x + 0 = x$, group relations $\tau \sigma = \sigma^2 \tau$) are only *propositionally* true.
* The moment a term of type `Vec (n + 0)` is passed where `Vec n` is expected, standard type theories reject it. You must construct an equality proof $p : n + 0 = n$ and manually invoke `transport (λ k. Vec k) p v`. In non-abelian algebra, this leads to an explosion of transport boilerplate.
* In $\lambda\Pi/\mathcal{R}$, equational laws are elevated to **definitional rewrite rules**. The typechecker evaluates them during conversion: `Vec (add n zero)` and `Vec n` are literally the **same type** at compile time. Zero transports, zero casts.

### 2. Micro-Kernel Architecture vs. Monolithic Megaliths
* Standard kernels (like Lean 4 or Coq) are tens of thousands of lines of C++ or OCaml baking in inductive families, positivity checkers, guardedness checkers, termination checkers, and quotient types.
* In $\lambda\Pi/\mathcal{R}$, the kernel knows **only 5 concepts**: Variables, Symbols, Sorts (`Type`/`Kind`), $\Pi$-types, and Applications/Abstractions.
* Everything else—inductive datatypes, arithmetic, recursion schemes, group presentations, and Automath books—is just **constants + rewrite rules declared in user space**.

### 3. Application Spine Representation
* Standard binary ASTs represent $f \; a \; b \; c$ as deeply nested trees: `App(App(App(f, a), b), c)`.
* By decomposing applications into an **application spine** (`Spine = { head: f, args: [a, b, c] }`), pattern matching becomes a flat linear scan over argument patterns, matching first-order term rewriting intuition directly.

---

## Building and Running

Requires a C++23 compiler (`g++-14+`, `g++-16`, or Apple Clang with C++23 support).

```bash
make test
```

### Examples Included

1. **`examples/mltt_vectors.cpp`**:
   * Peano arithmetic with rewrite rules for `add`.
   * Length-indexed `Vec : Nat -> Type`.
   * `vconcat : Π (n : Nat). Π (m : Nat). Vec n -> Vec m -> Vec (add n m)`.
   * Demonstrates concatenation of vectors computing definitionally.

2. **`examples/automath_logic.cpp`**:
   * Foundations of Propositional Logic in classic Automath line style.
   * Modus Ponens (`mp`) and Implication Introduction (`imp_intro`).
   * Proves Self-Implication (`id_imp`) and Hypothetical Syllogism (`syllogism`).
   * Demonstrates transparent unfolding of proofs via synthesized $\delta$-rules.

3. **`examples/inverse_galois.cpp`**:
   * Verification of the **Inverse Galois Problem** over $\mathbb{Q}$:
     * Realizing the cyclic group $C_2 \cong \mathbb{Z}/2\mathbb{Z}$ via the quadratic extension $\mathbb{Q}(\sqrt{2})/\mathbb{Q}$.
     * Realizing the non-abelian symmetric group $S_3$ via the splitting field of $x^3 - 2$ over $\mathbb{Q}$.
     * Definitional verification of group homomorphism conditions $\phi(\sigma_1 \circ \sigma_2) \equiv \phi(\sigma_1) \cdot \phi(\sigma_2)$ under the rewrite engine.
     * Fully typed proof terms witnessing `InverseGalois(C2, Q)` and `InverseGalois(S3, Q)`.

---

## License

MIT
