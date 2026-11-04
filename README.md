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

4. **`examples/scheme_interpreter.cpp`**:
   * A typed object-language evaluator built from λΠ signatures and term-rewriting equations.
   * The Scheme source subset includes tagged literals, quote, variables, lambdas, unary application, conditionals, and primitive list operations.
   * The evaluator is constructed as Scheme syntax, reified as data, and evaluated by the source-defined interpreter. The resulting interpreted closure then runs an identity application to `7`.
   * The example verifies both self-interpretation and execution by the resulting interpreter closure through the λΠ kernel.

5. **`examples/cpp_template_evaluator.cpp`**:
   * Mirrors a restricted C++ type-level vocabulary (`std::integral_constant`, addition, multiplication, equality, and `std::conditional_t`) as syntax in λΠ.
   * Evaluates the syntax with λΠ rewrite rules, checks its inferred type, and compares the result with a native C++ compile-time alias-template expression.
   * This is a paired example for the listed template subset; it does not parse arbitrary C++ source or evaluate general C++ templates.

6. **`examples/blink_rendering_model.cpp`**:
   * Encodes a small, typed HTML-to-paint semantic path in λΠ: HTML input, DOM, computed style, layout tree, and paint artifact.
   * The model uses explicit rewrite rules for one `<p>hello</p>` case with a block/black style. It is an initial semantic abstraction, not a translation of Blink's implementation or a browser.
   * A pinned, partial source snapshot and provenance notes are in [`vendor/blink-renderer-core`](vendor/blink-renderer-core/SOURCE.md); the first source-to-model map is in [`SEMANTIC_MAP.md`](vendor/blink-renderer-core/SEMANTIC_MAP.md). It contains selected upstream docs and C++ files, not the full Blink renderer or its dependencies.

7. **`examples/blink_parser_lifecycle.cpp`**:
   * Encodes the stopped/paused gate and decision order of selected branches in Blink's `HTMLDocumentParser::PumpTokenizerIfPossible` as rewrite rules returning typed parser actions.
   * It models branch outcomes for fragment abort, yielded tokenization, EOF completion, and delayed completion. It does not model the tokenizer, tree builder, scheduling machinery, or script execution.
   * The shallow sparse checkout at [`vendor/chromium-src`](vendor/chromium-src) contains the full `third_party/blink/renderer` subtree at a pinned Chromium commit. [`RENDERER_INVENTORY.md`](vendor/blink-renderer-core/RENDERER_INVENTORY.md) records path counts and current semantic coverage.

8. **`examples/blink_parser_bisimulation.cpp`**:
   * Models the selected parser decision boundary as a finite deterministic coalgebra, with six control flags as states, parser actions as observations, and a terminal successor.
   * An explicit relation covers all 64 flag valuations plus terminal. The λΠ checker verifies observation equality and successor closure for that relation; a native C++ transcription is also compared with both λΠ observers over all 64 inputs.
   * This checks only the extracted one-step branch fragment. It does not execute Blink's linked `HTMLDocumentParser`, and the current λΠ kernel does not certify rewrite-rule termination.

9. **`examples/blink_css_cascade_bisimulation.cpp`**:
   * Models winner selection for one property among already matched declarations, using an order-preserving priority tuple for specificity, proximity, stylesheet position, rule position, and declaration position.
   * Computes a finite relation between an imperative C++ scan and the λΠ maximum-priority semantics for five cases; origin, importance, tree order, inline style, and layer are fixed.
   * Selectors and CSS parsing are assumed complete before this boundary. The example does not execute Blink's `StyleCascade` and does not establish coverage of the full CSS cascade.

10. **`examples/blink_css_selector_bisimulation.cpp`**:
   * Models the tag, id, and class tests used by Blink's easy selector checker, plus conjunctions of those simple selectors on one element.
   * A finite coalgebra relates nine selector/element cases and a terminal state; the λΠ relation check and native C++ matcher agree on all cases.
   * It assumes selectors are already parsed and uses one namespace and case-sensitive names. Combinators, attributes, pseudo-classes, shadow scopes, and the general selector algorithm are not modeled.

11. **`examples/blink_css_combinator_bisimulation.cpp`**:
   * Models right-to-left compounds of tag, id, and class selectors with child and descendant relations over finite element trees, following Blink's `CSSSelector` representation and easy-matcher path.
   * The finite relation checks nine selector/tree cases and a terminal state against a native C++ matcher.
   * Cases obey `EasySelectorChecker::IsEasy`'s restriction against child combinators after descendant combinators. Sibling relations, namespaces, and general selector matching remain unmodeled.
12. **`examples/blink_tree_builder_p_start_bisimulation.cpp`**:
   * Models the shared in-body start-tag path for `<p>` and the 24 other tags in its block-start case set: when `<p>` is in button scope, Blink processes a fake `</p>` before inserting the incoming element.
   * A finite coalgebra checks `<p>` against eleven stack configurations and each distinct block-start tag against both an in-scope stack and a stack with `<p>` hidden under a marker: 59 token/stack cases plus a terminal state.
   * Each start-tag name has its own λΠ constructor and remains distinct in the resulting stack. Scope markers are grouped into button, HTML, foreign-content, and document-fragment categories. The model also checks one non-top removal of `<p>`; general stack mutations, parse-error effects, and DOM mutation remain unmodeled.

13. **`examples/blink_tree_builder_p_end_bisimulation.cpp`**:
   * Models the final open-element-stack effect of the `</p>` branch over ten finite stacks. The native projection includes represented `li`, `dt`, and `option` implied-end-tag pops; seven cases check that λΠ's direct pop-through-`p` projection gives the same suffix. Fake-token flow, parse errors, unrepresented implied candidates, and DOM effects remain outside the model.

14. **`examples/blink_css_revert_layer.cpp`**:
   * Models Blink's first-eligible search over an already sorted strongest-first priority list for one `revert-layer` declaration. A separate C++ scan agrees with λΠ reduction on twenty finite inputs, including same-layer ties. Construction of the full priority map, origin comparisons, and recursive CSS value resolution are not included.

15. **`examples/blink_layout_dirty_bisimulation.cpp`**:
   * Models `SetNeedsLayout` idempotence and propagation of child-dirty bits through a bounded rooted container chain. It assumes no relayout-boundary early return and excludes display locks, out-of-flow container jumps, orthogonal writing modes, view scheduling, and paint invalidation.

16. **`examples/blink_css_priority_layer_normalization.cpp`**:
   * Checks the layer-field normalization performed by `CascadePriority::ForLayerComparison` against a C++ transcription of the packed bit operations. The λΠ field model covers all 16 four-bit values with importance on and off (32 cases); the other priority fields are fixed or omitted.

17. **`examples/blink_tree_builder_li_end_bisimulation.cpp`**:
   * Models the `</li>` stack transition, including list-item-scope hits and no-op misses. Eleven finite stacks are compared; five hit cases check that represented implied-end-tag popping has the same final stack as the λΠ pop-through-`li` abstraction.

18. **`examples/blink_css_color_defaulting.cpp`**:
   * Models `color` defaulting for missing, `inherit`, `initial`, and `unset`, with or without a parent style. A native C++ branch transcription agrees with λΠ on 24 finite cases. Color values are abstract IDs and initial black is encoded as zero.

19. **`examples/blink_style_recalc_gate.cpp`**:
   * Models `ShouldRecalcStyleFor` suppression, propagation, dirty-node and style-container-query-dependent branches, plus the selected `ForChildren` propagation/flag projection. A C++ transcription agrees on 160 gate states and 20 child-change states; other flags and real DOM traversal are omitted.

20. **`examples/blink_tree_builder_block_end_bisimulation.cpp`**:
   * Models the shared in-body end-tag branch for all 28 distinct block-element target tags, including the general scope scan, implied-end-tag pass, and pop-through-target transition.
   * Compares 35 finite C++/λΠ configurations: one scope hit for each target plus nested implied-tag, unrelated-top, marker-miss, and absent-target cases. Parse-error and DOM effects, arbitrary stack mixtures, and full tokenizer/parser control flow remain unmodeled.

21. **`examples/blink_tokenizer_open_tag_bisimulation.cpp`**:
   * Models the Data, TagOpen, and EndTagOpen dispatch branches over ten character classes, with seven tokenizer states and 70 finite transitions. It abstracts later tag-name, markup-declaration, bogus-comment, and character-reference processing, and assumes processing-instruction support is disabled.

22. **`examples/blink_tokenizer_tag_name_bisimulation.cpp`**:
   * Models bounded `HTMLTokenizer::NextTokenImpl` TagName scanning with explicit lowercase name accumulation, including representative uppercase and lowercase letters and unchanged nonalphabetic name characters.
   * Nine finite sequences compare a C++ transcription with typed λΠ across whitespace, slash, `>`, EOF, and exhausted-chunk boundaries. It omits the input-stream preprocessor, null replacement, full Unicode case handling, attributes, and token/DOM side effects.

23. **`examples/blink_dom_insert_html_element.cpp`**:
   * Models normal `HTMLConstructionSite::InsertHTMLElement` attachment with explicit DOM parent IDs, open-element IDs, fresh node identity, and ordered finite token attribute name/value pairs copied onto the new node. Four finite DOM/stack/attribute states agree between the C++ transcription and typed λΠ. Custom-element creation, sanitization, foster parenting, insertion-location adjustment, and DOM callbacks are excluded.

24. **`examples/blink_paint_chunker_bisimulation.cpp`**:
   * Models PaintChunker grouping by property state, including forced chunk boundaries before and after foreign-layer/scrollbar items. Eight finite item sequences agree between the C++ transcription and typed λΠ; display-item construction and full paint artifact behavior are excluded.

25. **`examples/blink_css_cascade_priority.cpp`**:
   * Compares a semantic λΠ lexicographic key against Blink's packed `CascadePriority` comparison over 18 precedence, equality, and field-boundary pairs. It includes encoded origin/importance, tree and layer order, style flags, rule/declaration indices, and the already-applied bit; CSS rule collection and application remain outside scope.

26. **`examples/blink_css_cascade_map.cpp`**:
   * Composes the fixed-prefix packed priority fields with per-property list insertion and first-eligible `FindRevertLayer`. Eleven insertion-prefix states and 99 prefix/bound lookups agree with the C++ transcription, including weaker insertion and new-before-old equal-priority ties.

27. **`examples/blink_tokenizer_attribute_name_bisimulation.cpp`**:
   * Models AttributeName accumulation from an existing name prefix, including lowercase normalization, whitespace, slash, `=`, `>`, EOF, and quote/`<` parse-error append-and-continue behavior.
   * Fourteen finite input sequences compare a C++ transcription with typed λΠ. Duplicate attributes, source-range tracking, null/preprocessor behavior, attribute values, and token/DOM effects remain unmodeled.

28. **`examples/blink_tokenizer_attribute_value_bisimulation.cpp`**:
   * Models before-value, double-quoted, single-quoted, unquoted, after-quoted, and self-closing transitions with explicit value accumulation, parse-error state, and token-emission outcomes.
   * Twenty finite traces compare C++ and typed λΠ. Ampersands terminate at an opaque character-reference handoff that records the quoted or unquoted return mode; entity decoding, null preprocessing, range tracking, and token/DOM internals are outside the model.

29. **`examples/blink_css_revert_color.cpp`**:
   * Composes strict `revert-layer` candidate selection, `ResolveRevertTo` fallback to `unset`, and inherited `color` handling. Twelve finite C++/λΠ cases cover candidate hits/misses, equal-layer skips, ties, CSS-wide values, parent/root behavior, and a selected priority rejected as not weaker.

---

## License

MIT
