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

* **Single-header core**: The $\lambda\Pi/\mathcal{R}$ engine is implemented in [`src/lambda_pi.hpp`](src/lambda_pi.hpp).
* **Compiled rewrite lookup**: `RuleIndex` precomputes left-hand-side spines and groups rules by head symbol for large immutable rewrite systems. It preserves source order for overlapping rules; the original vector-based evaluator remains available.
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
   * Models `ShouldRecalcStyleFor` suppression, all three forcing propagation levels (`kIndependentInherit`, `kRecalcChildren`, `kRecalcDescendants`), dirty-node and style-container-query-dependent branches, plus the selected `ForChildren` propagation/flag projection. A C++ transcription agrees on 320 gate states and 20 child-change states, including an Element with no computed style; other query flags and real DOM traversal are omitted.

20. **`examples/blink_style_recalc_dependencies.cpp`**:
   * Models `RecalcFlagDependent` and the full `ShouldRecalcStyleFor` boolean decision for all 64 combinations of grouped size/style/scroll-state/anchored/content-visibility/hover flags, seven old-style dependency categories, Element/Text node kinds, computed-style present/absent, suppression, dirty state, and five propagation levels.
   * The C++ transcription and typed λΠ agree on 1,792 flag/dependency/node states and 35,840 gate states. Size flags match either size-query dependencies or highlight styles dependent on container units; content-visibility alone makes the generic recalc-flag predicate true, but no per-node dependency branch matches it. Child/descendant flag variants are grouped by family; full child flag propagation, pseudo-element decisions, layout reattachment, and Element traversal are outside scope.

21. **`examples/blink_style_recalc_children.cpp`**:
   * Models `StyleRecalcChange::FlagsForChildren` over all 8,192 13-bit flag masks and both old-style/can-match-size-query booleans. Typed λΠ agrees with the C++ transcription on 32,768 states and checks the combined propagation-plus-flag transform on five propagation levels.
   * The model clears the child-only style-container and suppress-recalc flags, conditionally clears current-size-container recalc, and preserves mark-reattach only when suppression was set. This is the exact flag transform; Element traversal and recalculation scheduling are outside scope.

22. **`examples/blink_style_recalc_traversal.cpp`**:
   * Models `TraverseChildren`, `TraversePseudoElements`, `TraverseChild`, `MarkReattachLayoutTree`, `ShouldUpdatePseudoElement`, and `IndependentInherit` from `StyleRecalcChange`. It checks 640 states each for the first two predicates, 17,920 pseudo-element states, 1,280 independent-inherit states, all eight mark-reattach combinations, and the 64-input OR boundary for `TraverseChild`.
   * It composes `TraverseChild` with `ShouldRecalcStyleFor` over 320 representative node/change states. `ShouldUpdatePseudoElement` has a distinct dependency check: size/style/scroll/anchored queries can match, but highlight container-unit styles, cached hover, and content visibility do not. `IndependentInherit` is allowed only at that propagation level and is disabled by old-style size/style query dependencies when their flag families are active. Query-family flags are grouped as in the dependency model; recursive Element traversal and style-engine scheduling remain outside scope.

23. **`examples/blink_style_recalc_copy.cpp`**:
   * Models the `RecalcOwnStyle` independent-inheritance caller guards and `PropagateInheritedProperties` fast-path eligibility as a typed boolean function. A C++ transcription and λΠ agree on all 32,768 combinations of three caller guards and twelve veto/availability conditions. The success result means only that Blink can take the copy shortcut; a declined shortcut reaches the resolver only when the separate parent/root condition holds. Generated property copying, `DCHECK(parent_style)`, and later style diff handling are outside the model.

24. **`examples/blink_computed_style_difference.cpp`**:
   * Models the ordered `ComputedStyle::ComputeDifferenceIgnoringInheritedFirstLineStyle` branch classifier across 19 abstract Boolean relations for scroll animation, descendant-affecting display/container changes, inherited and non-inherited equality, variable dependencies, pseudo styles, overscroll, and display-list transitions. Typed λΠ agrees with a C++ transcription on all 524,288 predicate assignments.
   * Also models `ComputeDifference` pointer identity/null-style handling and the maximum severity of cached inherited first-line and ordinary differences (576 states), plus `Element::ApplyComputedStyleDiff` propagation updates for all 30 current-propagation/difference pairs. Also models `NeedsReinsertLayoutTree` (16 position/float states) and the `LayoutObject::ApplyStyleChanges` selection (32 states) for diff severity, HTML root, anchor evaluator/functions, and first-letter initial-letter cases. Concrete generated-field equality, reachable-state constraints, cache lookup/computation, and style side effects are omitted.

25. **`examples/blink_needs_reattach_layout_tree.cpp`**:
   * Models the ordered `ComputedStyle::NeedsReattachLayoutTree` predicate as 19 named source-condition booleans: pointer identity, missing styles, display/content/marker changes, element-specific appearance and legend behavior, text-combine mode, first-line pseudo creation, overlay, and list-style position. Typed λΠ agrees with a C++ transcription on all 524,288 Boolean assignments. The text-combine clause is additionally refined to all 100 combinations of old/new text-combine presence and the five concrete writing modes, including composition into the reattach decision. `ScrollMarkerGroupEqual` is modeled over `none` and four position/mode values; typed λΠ agrees on all 25 old/new pairs and composes value inequality into the reattach decision.
   * Compound style comparisons and element-type conjunctions are supplied as already-evaluated conditions; independent assignments can include unreachable combinations. The model chooses whether reattachment is needed, but does not rebuild the layout tree.

26. **`examples/blink_diff_affects_scroll_animations.cpp`**:
   * Models `DiffAffectsScrollAnimations` as the disjunction of six old/new field inequalities: scroll-timeline name/axis, view-timeline name/axis/inset, and timeline scope. Typed λΠ agrees with a C++ transcription on all 64 states.
   * Composes the predicate into the first-priority `ComputeDifference` route for a baseline style relation, also checking 64 states. Field values and equality implementations remain opaque.

27. **`examples/blink_content_data_equivalence.cpp`**:
   * Models the `ContentData::operator==` linked-list traversal with typed nodes for text, alt-text, quote, none, image, counter, and alt-counter data. It compares 11,449 directed node pairs, preserving the base-counter-to-alt-counter asymmetry, then checks 34,348 bounded list pairs and the resulting `ContentDataEquivalent` reattach trigger.
   * Strings, counter-style, and tree-scope values are finite atoms; the linked-list fixtures cover lengths through two, with `NoneContentData` constrained to have no successor. `ImageContentData` payload equality is additionally refined over all six `StyleImage` kinds and 12 abstract field relations (147,456 states). A relation-parameterized recursive list path composes the payload comparison for singleton lists, image nodes at the head or tail of a two-node list, and two repeated image-kind nodes (589,824 C++/λΠ comparison cases); underlying field equalities remain unevaluated.

28. **`examples/blink_style_image_equality.cpp`**:
   * Models the six pinned `StyleImage` subclass equality branches, including directional pending-image comparisons, and composes inequality into the `ImageContentData` reattach trigger. The relation model is also composed directly into `blink_content_data_equivalence.cpp` as the `ImageContentData` payload equality. Subclass field relations remain abstract Boolean inputs; their underlying CSS values, resources, and image computations are not evaluated.

29. **`examples/blink_tree_builder_block_end_bisimulation.cpp`**:
   * Models the shared in-body end-tag branch for all 28 distinct block-element target tags, including the general scope scan, implied-end-tag pass, and pop-through-target transition.
   * Compares 35 finite C++/λΠ configurations: one scope hit for each target plus nested implied-tag, unrelated-top, marker-miss, and absent-target cases. Parse-error and DOM effects, arbitrary stack mixtures, and full tokenizer/parser control flow remain unmodeled.

30. **`examples/blink_tokenizer_open_tag_bisimulation.cpp`**:
   * Models the Data, TagOpen, and EndTagOpen dispatch branches over ten character classes, with seven tokenizer states and 70 finite transitions. It abstracts later tag-name, markup-declaration, bogus-comment, and character-reference processing, and assumes processing-instruction support is disabled.

31. **`examples/blink_tokenizer_tag_name_bisimulation.cpp`**:
   * Models bounded `HTMLTokenizer::NextTokenImpl` TagName scanning with explicit lowercase name accumulation, including representative uppercase and lowercase letters and unchanged nonalphabetic name characters.
   * Nine finite sequences compare a C++ transcription with typed λΠ across whitespace, slash, `>`, EOF, and exhausted-chunk boundaries. It omits the input-stream preprocessor, null replacement, full Unicode case handling, attributes, and token/DOM side effects.

32. **`examples/blink_dom_insert_html_element.cpp`**:
   * Models normal `HTMLConstructionSite::InsertHTMLElement` attachment with explicit DOM parent IDs, open-element IDs, fresh node identity, and ordered finite token attribute name/value pairs copied onto the new node. Four finite DOM/stack/attribute states agree between the C++ transcription and typed λΠ. Custom-element creation, sanitization, foster parenting, insertion-location adjustment, and DOM callbacks are excluded.

33. **`examples/blink_tree_builder_p_start_dom_insert.cpp`**:
   * Composes the block-start p-in-button-scope check/pop with attributed element insertion. Four finite states cover a top-p hit, a hit with an ordinary stack entry above p, a button-marker miss with a lower p, and no p; resulting DOM parent, node, and open stack agree between a C++ transcription and typed λΠ. Parse-error/fake-start reprocessing details and full Blink DOM behavior are abstracted.

34. **`examples/blink_paint_chunker_bisimulation.cpp`**:
   * Models PaintChunker grouping by property state, including forced chunk boundaries before and after foreign-layer/scrollbar items. Eight finite item sequences agree between the C++ transcription and typed λΠ; display-item construction and full paint artifact behavior are excluded.

35. **`examples/blink_css_cascade_priority.cpp`**:
   * Compares a semantic λΠ lexicographic key against Blink's packed `CascadePriority` comparison over 18 precedence, equality, and field-boundary pairs. It includes encoded origin/importance, tree and layer order, style flags, rule/declaration indices, and the already-applied bit; CSS rule collection and application remain outside scope.

36. **`examples/blink_css_cascade_map.cpp`**:
   * Composes the fixed-prefix packed priority fields with per-property list insertion and first-eligible `FindRevertLayer`. Eleven insertion-prefix states and 99 prefix/bound lookups agree with the C++ transcription, including weaker insertion and new-before-old equal-priority ties.

37. **`examples/blink_tokenizer_attribute_name_bisimulation.cpp`**:
   * Models AttributeName accumulation from an existing name prefix, including lowercase normalization, whitespace, slash, `=`, `>`, EOF, and quote/`<` parse-error append-and-continue behavior.
   * Fourteen finite input sequences compare a C++ transcription with typed λΠ. Duplicate attributes, source-range tracking, null/preprocessor behavior, attribute values, and token/DOM effects remain unmodeled.

38. **`examples/blink_tokenizer_attribute_value_bisimulation.cpp`**:
   * Models before-value, double-quoted, single-quoted, unquoted, after-quoted, and self-closing transitions with explicit value accumulation, parse-error state, and token-emission outcomes.
   * Twenty finite traces compare C++ and typed λΠ. Ampersands terminate at an opaque character-reference handoff that records the quoted or unquoted return mode; entity decoding, null preprocessing, range tracking, and token/DOM internals are outside the model.

39. **`examples/blink_css_revert_color.cpp`**:
   * Composes strict `revert-layer` candidate selection, `ResolveRevertTo` fallback to `unset`, and inherited `color` handling. Twelve finite C++/λΠ cases cover candidate hits/misses, equal-layer skips, ties, CSS-wide values, parent/root behavior, and a selected priority rejected as not weaker.

40. **`examples/blink_css_match_winner.cpp`**:
   * Composes conjunctive tag/id/class matching with collection of only matching declarations, then winner selection for one property using a three-field abstract lexicographic priority.
   * Nine finite rule sets compare the C++ transcription with typed λΠ, including a no-match result, compound selectors, the second and third priority fields, and equal-priority later-input tie selection.
   * The priority fields are a named projection, not Blink's complete `MatchedRule` sort key or `CascadePriority`; parsing, combinators, attribute/pseudo selectors, layers, origins, importance, property application, and full stylesheet ordering remain outside scope.

41. **`examples/blink_numeric_html_entity_bisimulation.cpp`**:
   * Models numeric reference prefix/decimal/hex scanning, finite radix transitions, malformed-prefix rollback, semicolon and terminator outcomes, and exhausted-input rollback. Forty-nine per-character traces compare C++ and typed λΠ, including the U+10FFFF boundary, overflow-to-invalid latch, and later-digit retention.
   * Termination connects tested accumulators to replacement, an independent λΠ transcription of all 32 pinned U+0080–U+009F mapping entries (including unchanged C1 controls), U+007F/U+00A0 pass-through, and ordinary output. Radix transitions remain finite to the tested traces.

42. **`examples/blink_html_named_entity.cpp`**:
   * Retains a compact 11-entry λΠ trie for focused longest-prefix and attribute-guard cases.
   * Exact cursor rewind and `SegmentedString` mutations are not modeled; numeric references are handled separately.

43. **`examples/blink_html_named_entity_states.cpp`**:
   * Generates 9,854 named states from all 2,231 pinned CSV rows, with a distinct `walk_<id>` and `prefix_<id>` rewrite head for each state. `RuleIndex` dispatches their character transitions without scanning unrelated rewrite rules.
   * Twelve finite C++/typed-λΠ result comparisons cover semicolon-terminated matches, longest-prefix misses, semicolonless attribute acceptance/rejection, two-codepoint output, EOF prefixes, and no match. The latest `make` run generated 59,258 rules in 18 ms, indexed them in 6 ms, typechecked the observer by 24 ms total, and reduced cases in 0–4 ms.
   * This uses the full table to generate the semantic machine, but compares only selected inputs. It abstracts exact source rewind, `SegmentedString` chunk suspension, and tokenizer mutation; the C++ comparison helper is a standalone reference, not a direct execution of Blink's parser.

44. **`examples/blink_process_entity_bisimulation.cpp`**:
   * Composes finite decoder outcomes with `HTMLTokenizer::ProcessEntity` for Data/RCDATA: success appends one or two codepoints and sets the entity flag, failure appends literal `&`, and incomplete input suspends with the prior token buffer retained. Eight finite outcomes agree between the C++ transcription and typed λΠ.
   * Decoder outcomes are fixture constructors rather than direct calls into the numeric scanner or named trie; exact stream rewind and tokenizer runtime behavior remain outside the model.

45. **`examples/blink_tokenizer_attribute_entity_bisimulation.cpp`**:
   * Composes bounded named/numeric decoder observations with the attribute-value character-reference state. Ten finite cases cover decoded append/entity flag, literal ampersand fallback, incomplete-input suspension, and restoration to double-quoted, single-quoted, or unquoted state.
   * The C++ decoder uses selected names and numeric fixtures; typed rules encode those fixture outcomes. Full-table reduction, arbitrary codepoint arithmetic, exact cursor rewind, and stream lifecycle remain outside scope.

46. **`examples/blink_css_selector_color.cpp`**:
   * Composes restricted tag/id/class matching and matched-only declaration collection with three-field abstract priority selection, then applies `color` explicit/initial/inherit/unset/missing behavior using parent or root context.
   * Twelve finite C++/typed-λΠ cases compare match/miss, winner and tie behavior, CSS-wide keywords, and inherited/root outcomes. It does not cover full selector syntax, priority ordering, cascade origins/layers, actual CSS color values, or computed-style lifecycle.

47. **`examples/blink_css_revert_origin.cpp`**:
   * Models `TargetOriginForRevert`, `CascadeMap::Find(property, origin)` candidate filtering, and `ResolveRevertTo` fallback using the origin/importance priority field plus rule index, with other `CascadePriority` fields fixed.
   * Eleven finite C++/typed-λΠ comparisons cover UA/user/hint/author/animation target origins, absent and no-origin candidates, weaker resolution, and equal/stronger priority fallback. It does not model CSS-wide value recursion, declaration collection, or full packed priority ordering.

48. **`examples/blink_html_named_entity_remainder.cpp`**:
   * Extends the all-table named-reference state model with `decoded`, `noEntity`, `guardReject`, and `needMore` outcomes, consumed spelling, numeric cursor offset, and remaining text. λΠ computes accepted remainder with generic list-prefix operations.
   * Twelve finite C++/typed-λΠ comparisons and one two-chunk continuation cover successful matches, overlong-prefix rollback, attribute guard rejection, incomplete prefixes, and preserved attribute context. Offsets are relative to candidate text after `&`; `SegmentedString` segment mutation and tokenizer state/token mutation remain outside scope.

49. **`examples/blink_css_revert_origin_color.cpp`**:
   * Composes the `revert` target-origin scan and weaker-priority fallback with CSS-wide defaulting for inherited `color`, including explicit, initial, inherit, unset, missing-candidate, parent, and root cases.
   * Twelve finite C++/typed-λΠ comparisons use the origin/importance plus rule-index priority projection from the separate revert-origin model. Other `CascadePriority` fields, actual color-scheme state, `currentColor`, visited/animated color behavior, and style-tree effects are omitted.

50. **`examples/blink_segmented_entity_source_bisimulation.cpp`**:
   * Models bounded `SegmentedString` source chunks, cursor advancement, consumed offset, the `PushIfPossible` rewind and prepend fallback, and `Prepend(kUnconsume)` rollback.
   * Per-operation C++/typed-λΠ checks cover one/two-character rewind, segment-boundary and mismatch fallback, a longer rollback followed by longest-prefix replay, and Data/attribute suspension, append, resume, and completion. The traces are bounded ASCII cases; general trie lookup and entity decoding are separate.

51. **`examples/blink_css_revert_origin_full_priority.cpp`**:
   * Composes revert-origin selection and inherited `color` defaulting with a typed semantic key for every `CascadePriority` field in pinned high/low significance order; the C++ transcription independently compares the packed high and low words.
   * Eighteen composition cases and twelve priority boundary pairs cover origin/importance, tree and layer reversal, try/inline flags, rule/declaration indices, and already-applied. The λΠ field domain is finite; packed 96-bit arithmetic and full renderer behavior remain outside scope.

52. **`examples/blink_named_entity_segmented_composition.cpp`**:
   * Its public `observe(CursorSource, InputEnd, context)` path generates `sourceWalk_<state>` rules that consume the typed segmented Source directly, including cross-segment transitions, unconsumed mismatch lookahead, longest-terminal correction, attribute guarding, and `streamingExhausted` versus `finalEOF`. Need-more preserves token/context and suspends; completion appends one/two codepoints and sets the entity flag, or appends literal `&` on failure.
   * Ten direct cursor-source cases cover split edges, overread and exact attribute guards, no-match, one/two-character Push rollback, longer Prepend rollback, and both exhaustion modes. Fourteen legacy finite comparisons remain as a separate observer cross-check. A typed context wrapper derives the named-entity attribute guard from Data/double-quoted/single-quoted/unquoted mode, then the result is applied directly to a cursor-preserving tokenizer state. Seven handoff fixtures and pending/append/resume in all four contexts compare output, entity flag, restored state, and exact cursor source. Double-quoted `amp;"`, single-quoted `NotEqualTilde;'`, and unquoted `notin;>` fixtures retain their value delimiters in the returned cursor. The latest build generated 177,709 rules. The typed `enterCharacterReference` transition consumes `&`, maps the originating Data/double-quoted/single-quoted/unquoted value state to a cursor context, and sends the remaining CursorSource into the direct walker. Direct checks compare the full CursorSource constructor, current segment before/remaining lists, queued segment partition, and global offset after rollback; backing String identity and intermediate cursor positions are omitted.


53. **`examples/blink_tokenized_p_start_dom.cpp`**:
   * Scans bounded raw `<p ...>` source into a typed ordered `StartTagToken` (including quoted/unquoted values, reference decoding, and ASCII-lowercased attribute names), then passes the ordered token attributes into an explicit first-occurrence DOM filter before p-scope pop and attributed DOM insertion. Twelve finite inputs compare token and DOM/stack projections against a C++ transcription. Duplicate-class cases retain both attributes in the token and only the first value in the DOM projection, including `CLASS`/`class`. The target compiles; its finite harness was not run in this update.
   * The same normalized token-to-DOM state feeds typed selector/color/layout/paint projections. Cases exercise fixed `p`, `.beta`, and `#alpha` selectors, modeled priority, color defaults, whitespace-separated class matching, and rejection of `betamax`. Body text before the next `<` is derived from raw input into one typed CharacterToken; the Data-state decoder handles five named references and eleven selected decimal/hex numeric spellings, including an attribute case that decodes a numeric reference to a class name and reaches the `.beta` style rule. The p-end transition closes the open-element stack. Text child IDs and the node counter are compared.
   * This is a bounded tokenizer transcription mapped to Blink source branches, not execution of Blink's tokenizer. Body coverage is plain Data-state text with those five named references and eleven selected numeric spellings in body text and attributes, without streaming boundaries, general numeric parsing, other named references, preprocessing, other tokenizer states, body markup beyond a following `</p>`, insertion-location changes, or actual Blink Node pointers. Attribute-name folding covers ASCII A-Z in the selected input path; duplicate filtering therefore compares normalized exact names. It does not model foreign-content namespaces or Blink's duplicate-attribute diagnostics/use counter. Class membership scans HTML ASCII whitespace tokens, following `selector_checker-inl.h:203-215` and `space_split_string.cc:37-70`. Selector syntax, full `CascadePriority`/`StyleCascade`, color-scheme selection, `currentColor`, style lifecycle, glyph shaping/geometry, actual text display-item creation, Unicode, parse-error reconsumption, custom/sanitizer paths, and general layout/paint remain outside scope. Other source anchors include `html_tokenizer.cc:278-293,173-189,1799-1846,852-1044`, `atomic_html_token.h:335-371`, `html_construction_site.cc:96-110,245-271,986-988,1078-1118,1240-1248,1458-1462`, `html_tree_builder.cc:875-881,2690-2872,2217-2310`, `computed_style.cc:800-803`, `layout_object.cc:3133-3156`, and `text_fragment_painter.cc:329-355`.

54. **`examples/blink_pending_text_bisimulation.cpp`**:
   * Models the finite state transition in `HTMLConstructionSite::PendingText::Append` and `FlushPendingText`: appends at equal parent/next-child locations concatenate chunks and retain the minimum whitespace mode; a changed location flushes before starting a new pending buffer. Flush effects distinguish empty/invalid discard, insertion of a new Text node, and appending data to a previous Text sibling.
   * Seven finite C++/typed-λΠ cases cover same-location accumulation, parent and next-child boundaries, all three previous-sibling categories, valid/invalid locations, and whitespace-mode minimization. Source anchors are `html_construction_site.h:264-303`, `html_construction_site.cc:245-271,1081-1118`.
   * This is an extracted transition model. Text chunks are opaque finite atoms; it does not model Blink String canonicalization, actual Node identity or mutation, insertion-location adjustment for templates/foster parenting, or task-queue ordering. Same-location accumulation, insertion, and Text-sibling coalescing are also connected to the raw `<p>…</p>` fixture above; this example's complete seven-case location/mode transition set remains separate.

55. **`examples/blink_dom_text_flush_bisimulation.cpp`**:
   * Models the child-list mutation at `HTMLConstructionSite::FlushPendingText` for a null `next_child`: an empty/invalid buffer leaves children and next ID unchanged; a valid non-empty buffer inserts a new identified Text child unless the last child is already Text, in which case its data grows and its identity is preserved.
   * Seven finite C++/typed-λΠ cases cover empty lists, trailing Text children, Text after element children, non-Text tails, invalid locations, and empty buffers. Source evidence is `html_construction_site.cc:245-271`, where Blink constructs a Text node, examines `lastChild()` when there is no next child, appends to a prior Text node, or inserts the new child.
   * This models visible child-list identities rather than Blink pointer identity. It fixes insertion at the end of one parent; adjusted/template/foster locations, String canonicalization, task ordering, and all other DOM mutation paths are omitted.

56. **`examples/blink_generated_image_equality.cpp`**:
   * Refines `StyleGeneratedImage::IsEqual` into equality of optional container width/height, six viewport dimensions, and an explicit generator-value-equivalence input. The C++/λΠ comparison loop spans 663,552 finite states and composes inequality into the generated-image reattach predicate.
   * Numeric dimensions are restricted to absent/0/1 for container sizes and 0/1 for viewport components. The paint-specific branch constructs six valid finite values from simple names (`alpha`, `beta`) and zero or one serialized argument (`red`, `blue`), derives each `paint(...)` CSS text, and compares `GetName()` plus `CustomCSSText()`. It checks 36 value pairs and composes them with representative size relations in 144 cases. The model omits NaN, signed zero, arbitrary doubles, lazy container lookup/cache side effects, general string serialization, and multiple paint arguments. A shared `CSSGradientValue::Equals` model covers repeat, interpolation space, hue method, and ordered color-stop lists through two stops over 254,016 finite base pairs. A targeted `CSSLinearGradientValue::Equals` model compares 144 finite fixture pairs across deprecated, prefixed, and standard linear-gradient branches, including nullable coordinates and angle cases. Stop colors now use the structural CSSColor relation for finite red/blue fixtures; offsets, coordinates, and angles remain finite CSSValue atoms, and null hint stop components are excluded. A targeted `CSSRadialGradientValue::Equals` model compares 121 fixture pairs across deprecated/prefixed/standard types, covering legacy center/radius fields, modern explicit sizes, and defaulted shape/sizing identifiers. It preserves the observed direction-dependent gradient-type check in the deprecated branch versus no modern type check; one fixture exercises the resulting asymmetric cross-type comparison. A targeted `CSSConicGradientValue::Equals` path compares 64 fixture pairs over common fields and nullable x/y/from-angle values. `CSSConstantGradientValue::Equals` and `CSSColorImageValue::Equals` use a structural CSS color relation over two color spaces, four “none” flags, and four exact components in {0,1}; the latter delegates to the former. The direct color wrappers compare four fixtures. Gradient-stop color fixtures use the same structural CSSColor relation; offsets and the complete CSSValue dispatch remain finite or outside scope. A tagged `CSSImageGeneratorValue` relation now composes the paint, linear, radial, conic, constant-gradient, and color-image equalities with container and viewport size equality. It checks 6,724 generated-image states, including cross-subclass comparisons and the resulting inequality decision. Constant-gradient, color-image, and gradient-stop color fixtures use structural CSSColor fields over the bounded domain above.


57. **`examples/blink_css_color_equality.cpp`**:
   * Models `cssvalue::CSSColor::Equals` through Blink `Color::operator==`, comparing color space, four “none” flags, and four component values. Its executable compares 262,144 ordered pairs over two color spaces, all Boolean flag combinations, and exact components in `{0,1}`.
   * The field-level relation is integrated into constant-gradient, color-image, and gradient-stop color equality in item 56. Other color spaces, arbitrary float values, and color-construction semantics are not modeled.


58. **`examples/blink_crossfade_value_equality.cpp`**:
   * Models `CSSCrossfadeValue::Equals` as equal-length ordered lists of image/percentage pairs, with separate finite equality relations for each CSSValue field. It composes that relation with `StyleCrossfadeImage::IsEqual`’s crossfade-kind guard.
   * Five finite CSSCrossfadeValue fixtures (empty, singleton, and length-two lists) yield 25 ordered value pairs; both crossfade and non-crossfade right-hand kinds yield 50 composed states. The pinned method compares each image and percentage with `base::ValuesEquivalent`; the model abstracts those nested value relations to finite atoms.
   * This does not model arbitrary CSS image/percentage values, other crossfade payload structures, construction, resolution, weight calculation, or rendering.


59. **`examples/blink_image_set_value_equality.cpp`**:
   * Refines the `StyleImageSet::IsEqual` branch into right-hand subclass check, best-fit image equivalence, and `CSSImageSetValue` equality. The CSS value relation compares ordered options, and each option compares image, resolution, and optional type.
   * Six finite values (empty, singleton and length-two lists) yield 36 CSS value pairs. With both right-hand kinds and both best-fit outcomes, the StyleImageSet composition covers 144 states. The source constructor fixes the value-list separator to comma; the model therefore treats it as equal on both sides.
   * Nested image, resolution, and type CSSValue relations use finite atoms. Best-fit image equality remains an explicit Boolean input; unbounded option lists, CSSValue dispatch, resource selection, and image rendering are not modeled.


60. **`examples/blink_image_set_selection.cpp`**:
   * Models the first-call `CSSImageSetValue::GetBestOption` pipeline: remove unsupported/nonpositive options, stable-sort by resolution, keep the first option at each duplicate resolution, then choose the first resolution at or above the device scale or fall back to the last option.
   * Twenty-one finite option lists with up to three options are compared at four device scales and all eight AVIF/JXL feature configurations (672 states). The λΠ model expresses optional MIME normalization/support, positive-resolution filtering, insertion sort, stable deduplication, and threshold/fallback selection as separate reductions.
   * Resolution values are 0–3 and device scales 1–4. MIME support uses representative ASCII strings; it includes the finite support relation detailed in item 63. The model omits floating-point resolution computation, MIME values outside the represented set, `options_` cache mutation, and the subsequent image-load dispatch.


61. **`examples/blink_image_set_load_dispatch.cpp`**:
   * Models the consumer path in `StyleImageLoader::ResolveImageSet`: no selected option returns null; selected `CSSImageValue` and `CSSGradientValue` options are accepted; other CSS value kinds are rejected. The CSS image route carries the selected computed resolution as an override, while the gradient route produces a generated-image result without retaining that override.
   * Fifteen finite selected-option states compare a C++ branch transcription with typed λΠ. Image identity and resolution use finite atoms.
   * Resource fetching, the CSSImageValue cache implementation, gradient container/viewport evaluation, and the outer CSSImageSetValue cache call are not modeled.


62. **`examples/blink_image_set_cache.cpp`**:
   * Models `CSSImageSetValue::IsCachePending`, `CacheImage`, and permitted `CachedImage` reads for both shared and fetcher-keyed cache modes. A cache update is composed with pending checks for either fetcher and each represented scale.
   * Seven shared-cache states and 49 two-fetcher cache states cover 336 pending queries, 672 cache updates, 4,032 update-then-pending queries, and permitted cache reads. Scale fixtures include 1.0, its next representable `float`, and 2.0 to expose the strict `abs(a-b) < epsilon` boundary.
   * Effective cache mode is an input standing for `fetcher != nullptr && StyleResourceFetcherIdentityCheckEnabled()`. Weak-key expiration, GC identity, concurrent mutation, and general float values are outside scope.


63. **`examples/blink_image_set_type_support.cpp`**:
   * Refines the type-support and positive-resolution predicate used by `CSSImageSetOptionValue::IsSupported`. It models ASCII case normalization, representative supported and unsupported image MIME types, AVIF build-flag gating, and JPEG XL build/runtime-feature gating.
   * Twenty finite option inputs, including all twelve static supported MIME types and case variants, are compared under all eight feature-flag combinations (160 states). The λΠ relation first normalizes all twelve static supported MIME atoms and case variants, then evaluates support and composes it with positive-resolution gating.
   * The same predicate is also composed into the option-list selection reduction in item 60. Other MIME types and the full Chromium build configuration remain outside scope.


64. **`examples/blink_image_set_resolve_values.cpp`**:
   * Models `CSSImageSetValue::ResolveValuesAndCreateCopyIfNeeded` and the `ResolveValuesIfNeeded` return choice. Each option receives an explicit `ResolveGradients` observation: same image reference or replacement image. The copied option preserves resolution and optional type; the list order is preserved.
   * Seven finite lists compare C++ and typed λΠ, including no-change identity, a single replacement, mixed unchanged/changed options, multiple replacements, and replacement with the same finite image ID.
   * Resolver internals, allocation behavior, and raw GC pointer identity are abstracted to the per-image same-reference/replacement observation and the final original/copy tag.


65. **`examples/blink_image_set_failed_subresources.cpp`**:
   * Models `CSSImageSetValue::HasFailedOrCanceledSubresources`: select the cached style image using shared versus fetcher-keyed mode, then classify absent image, absent resource content, loaded content, and failed/canceled content. The source intentionally does not compare device scale.
   * The typed λΠ model and C++ transcription cover 512 finite cache/status states across fetcher presence, feature gate, fetcher choice, and all three cached-state observations; the cache branch is derived from fetcher presence and the feature gate.
   * Fetcher-key identity, resource loading, DSF values, and `StyleImage`/`ImageResourceContent` internals are abstracted to status constructors.


66. **`examples/blink_html_fastpath_gate.cpp`**:
   * Models Blink’s ordered `CanUseFastPath` fragment-parser gates: shadow-root behavior, tracing, parser content policy, and form ancestry, including the template-context exception.
   * The C++ transcription and typed λΠ decision cover all 32 combinations of those five Boolean inputs. The model covers eligibility only; context-tag dispatch, fragment parsing, and DOM effects remain outside scope.


67. **`examples/blink_html_fastpath_context.cpp`**:
   * Models `HTMLFastPathParser::Run` context dispatch: the `body` special case, all 15 non-void tags accepted as parser contexts, rejection of supported void tags, foreign namespaces, and unlisted tag names. Body initial-whitespace behavior is represented as a result distinction.
   * The typed λΠ model and C++ transcription cover 76 finite tag/namespace/behavior states across the 17 `SUPPORTED_TAGS` entries, `body`, and an unlisted-tag category.
   * Exact tag-name hash collisions, element creation, child parsing, and DOM effects are abstracted. The source’s post-hash exact QName check is reflected in the supported-tag classification.


68. **`examples/blink_html_fastpath_attributes.cpp`**:
   * Models the observable `ParseAttributes` decision after attribute names are scanned: reject `on*` event-handler attributes, reject `is`, reject malformed termination and duplicate canonical names, and otherwise accept `>` or `/>`.
   * A structural typed λΠ scan and duplicate detector are compared with a C++ transcription over canonical name lists of length zero through three, using six names and three terminators (777 finite states).
   * Attribute-name scanning/case normalization, attribute values, and DOM assignment are abstracted; the model covers the ordered guards and duplicate-name fallback only.


69. **`examples/blink_html_fastpath_attr_name.cpp`**:
   * Models `ScanAttrName` over ASCII-valid name characters: lowercase letters and hyphen use the fast path, while uppercase letters or digits take the mapping path, and uppercase ASCII is normalized to lowercase.
   * Typed λΠ recursive predicates and normalization are compared with a C++ transcription for 15 finite names at both boundary and end-of-input conditions (30 states). Lowercase-only names that end at input fail immediately; names that invoke the mapping path return a normalized span and leave subsequent EOF handling to the caller.
   * Invalid name characters and the subsequent `ParseAttributes` loop are outside scope.


70. **`examples/blink_html_fastpath_attr_boundary.cpp`**:
   * Models `ScanAttrName` at the scan frontier: end-of-input after a lowercase/hyphen prefix fails; delimiters return the prefix without consuming the delimiter; invalid characters return the prefix for caller rejection; uppercase/digit frontiers trigger full-name lowercase mapping.
   * Typed λΠ and C++ transcriptions cover 14 finite prefix/frontier states, including the distinct lowercase-fast and mapping-path EOF behaviors.
   * The subsequent `ParseAttributes` decision and arbitrary source strings remain outside this frontier model.


71. **`examples/blink_html_fastpath_child_dispatch.cpp`**:
   * Models the fast fragment parser’s parent-dependent child grammar: generic flow and phrasing rules, `ol`/`ul` accepting only `li`, `select` accepting only `option`, and `option` rejecting element children. It also represents flow/phrasing child policies, void tags, and nested `a`/`li` rejection.
   * The typed λΠ rules and C++ transcription cover 360 parent/tag/nesting-flag states over the 17 supported tags plus an unsupported category.
   * This models one child dispatch step; tag scanning, attributes, recursive child parsing, and DOM construction are abstracted.


72. **`examples/blink_html_fastpath_close_tag.cpp`**:
   * Models `ParseContainerElement` after child parsing: earlier child failure is preserved, clean EOF gets the container EOF result, short close-name input is rejected, names match exactly or by ASCII lowercase equivalence, and a matching name requires `>` after skipped whitespace.
   * The typed λΠ rules and C++ transcription cover 36 finite child-state/name-match/length/delimiter combinations.
   * Actual source-span slicing, cursor updates, and `FinishParsingChildren` effects are abstracted.


73. **`examples/blink_html_fastpath_children_step.cpp`**:
   * Models one `ParseChildren` loop iteration, preserving the order of text append before later exits. It distinguishes scan failure, plain/escaped text append, input end, closing-tag handoff, maximum-depth failure, child failure, and child append followed by continuation.
   * The C++ transcription and typed λΠ rules cover 64 finite scan/control combinations. Text payloads and child parser results are explicit observations.
   * Recursive parser calls, actual text/element node construction, and DOM mutation are outside scope.


74. **`examples/blink_html_fastpath_scan_text.cpp`**:
   * Models the core `ScanText` loop over symbolic code units: it accumulates plain text, leaves `<` unconsumed, delegates from the original start on `&` or carriage return, marks non-8-bit text, and fails on NUL.
   * Recursive typed λΠ rules compare with a C++ transcription on 11 finite code-unit sequences, including delimiter, escape, carriage-return, width, and NUL cases.
   * `ScanEscapedText`, newline-whitespace canonicalization, SIMD scanning, and source-string identity are not modeled.


75. **`examples/blink_html_fastpath_scan_escaped_text.cpp`**:
   * Models `ScanEscapedText` over a symbolic stream where character-reference decoding is an explicit input: append literal/decoded text, normalize CR and CRLF to LF, stop before `<`, and report NUL or reference failure.
   * Recursive typed λΠ rules and a C++ transcription cover 12 finite input sequences, including one- and two-code-unit reference output.
   * Entity parsing/consumption, concrete source offsets, string canonicalization, and buffer allocation remain abstract.


76. **`examples/blink_html_fastpath_reference_dispatch.cpp`**:
   * Models `ScanHTMLCharacterReference` after a semicolon-terminated scan observation: reject early EOF, NUL, overlong lookahead, and references shorter than two characters; dispatch numeric output, exact `amp`/`lt`/`gt`/`nbsp`, or uncommon-name fallback. A fallback remainder selects the cursor-rewind branch.
   * Typed λΠ dispatch and a C++ transcription cover 15 finite gate/decoder observations, including a semicolon found at the lookahead boundary.
   * The scan’s character-by-character loop, numeric conversion, entity table lookup, and concrete cursor offsets are abstracted as input observations.


77. **`examples/blink_html_fastpath_tag_name.cpp`**:
   * Models `ScanTagname`: lowercase ASCII names can return through the fast path; otherwise ASCII uppercase is mapped to lowercase. A post-name delimiter is required, and ASCII whitespace is skipped while retaining the resulting cursor category.
   * The typed λΠ scanner and C++ transcription cover 12 finite name/ending observations, including empty name, mixed case, slash/greater-than endings, whitespace endings, EOF, and invalid delimiter.
   * Actual cursor offsets and subsequent tag dispatch are outside this scanner model.


78. **`examples/blink_html_fastpath_wrapper.cpp`**:
   * Models `TryParsingHTMLFragment` orchestration: `CanUseFastPath` rejection returns before parser dispatch and leaves the optional unsupported-tag output unchanged; an allowed gate dispatches 8/16-bit parsing and reports success/abort.
   * It represents the optional output flag (true only for unsupported-tag parse failure) and the feature-gated unsupported-context log, which is suppressed for contexts classified as supported. The typed λΠ and C++ models cover 320 finite gate, width, parse-result, pointer, logging, and context states.
   * Parser internals and actual histogram samples, byte counts, DOM mutations, and timing values are abstracted.


79. **`examples/blink_image_set_load_cache.cpp`**:
   * Composes `ResolveImageSet` dispatch with the caller’s `CacheImage`: no selected or unsupported options produce a wrapper with no inner style image, CSS images preserve the selected resolution override, gradients produce generated-image payloads, and all outcomes are wrapped and cached.
   * The typed λΠ model and C++ transcription cover 120 finite selection, cache-mode/state, fetcher, and scale transitions, including overwriting one fetcher slot while preserving the other.
   * `GetBestOption` selection and actual resource/generator loading are explicit observations; GC pointer identity and image contents are abstracted.

80. **`examples/blink_image_set_selection_load_cache.cpp`**:
   * Puts the MIME-aware `GetBestOption` reduction, selected CSS value-kind dispatch, `StyleImageSet` wrapper creation, and caller-side cache update into one typed λΠ rewrite system. The selection rules are shared with item 60; a finite `imageValueKind` observation bridges the chosen option to loader dispatch.
   * The source includes a C++ comparison harness over three option lists, four device scales, both AVIF support settings, both cache modes, and two fetchers (96 transitions), with fixtures for CSS image load, gradient generation, selected-but-unsupported values, and no supported option. It has been compiled, but the harness has not been run in this change.
   * Resource fetching, generator evaluation, lazy `options_` mutation, nonempty initial cache contents, arbitrary floating-point resolutions, pointer/GC identity, and unbounded MIME strings remain abstract. The model's value-kind mapping is explicit fixture input, not inferred from MIME type.

81. **`examples/blink_image_set_options_cache.cpp`**:
   * Models the lazy `CSSImageSetValue::options_` behavior in `GetBestOption`: initialize from MIME/resolution-supported options, store a null-result sentinel if none survive, and reuse the cached sorted/deduplicated list on later calls.
   * The source contains a two-call λΠ/C++ comparison harness over two bounded lists, first and second device scales 1–4, and AVIF support on/off at each call (128 transitions). Changing AVIF support between calls represents the fact that later calls reuse the initialized option list rather than refiltering it.
   * The finite lists represent at most two options. Resolver-derived floating-point resolution changes, concurrent access, option pointer identity, and mutation of other CSS image-set state remain outside this model. The harness is compile-checked but was not executed in this change.

82. **`examples/blink_image_set_computed_resolution.cpp`**:
   * Models `ComputedResolution` dispatch between a non-calculated numeric literal and a calculated resolution expression, followed by Blink's result clamping. The literal branch applies a finite canonical-unit conversion; the calculated branch consumes an explicit expression-evaluation observation.
   * A bounded harness covers 24 literal value/unit combinations and five calculated-expression observations, including zero, finite values, NaN, and infinities. This is a representation of the source's dispatch and clamp boundaries, not a transcription of CSS math expression evaluation.
   * Unit coverage is limited to representative resolution units, and floating-point behavior is reduced to an explicitly bounded domain. Parser validation, random CSS math evaluation, full unit conversion tables, and arbitrary resolver states remain outside scope; the harness has only been compile-checked.

83. **`examples/blink_html_entity_search_bisimulation.cpp`**:
   * Models `HTMLEntitySearch` construction and `Advance` with a typed state containing the current character offset, surviving ordered entity range, and most recent full-name match. It loads the complete 2,231-name pinned `html_entity_names.csv`; generic λΠ rules narrow the range by the first character or the character at the current offset and update the terminal match only when the narrowed range's first entity ends at the new offset.
   * Eight finite prefixes are wired to observer terms, including `not=` which exercises range failure after retaining the prior `not` match. The source behavior is mapped to `html_entity_search.cc:33-62` and `html_entity_search.h:40-57`. The generated entity data comes from `html_entity_names.csv`; `make_html_entity_table.py:121-123` sorts the rows lexically before `HTMLEntityTable::AllEntries()` exposes them, and the C++ loader mirrors that ordering.
   * This models the search state transition over the full name table. It does not model `ConsumeNamedEntity` rollback, `SegmentedString` mutation/suspension, numeric references, or Unicode output decoding. The target has been compile-checked; its typed observer comparisons have not been executed in this update.


---

## License

MIT
