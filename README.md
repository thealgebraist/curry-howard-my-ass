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

24. **`examples/blink_tree_builder_p_start_dom_insert.cpp`**:
   * Composes the block-start p-in-button-scope check/pop with attributed element insertion. Four finite states cover a top-p hit, a hit with an ordinary stack entry above p, a button-marker miss with a lower p, and no p; resulting DOM parent, node, and open stack agree between a C++ transcription and typed λΠ. Parse-error/fake-start reprocessing details and full Blink DOM behavior are abstracted.

25. **`examples/blink_paint_chunker_bisimulation.cpp`**:
   * Models PaintChunker grouping by property state, including forced chunk boundaries before and after foreign-layer/scrollbar items. Eight finite item sequences agree between the C++ transcription and typed λΠ; display-item construction and full paint artifact behavior are excluded.

26. **`examples/blink_css_cascade_priority.cpp`**:
   * Compares a semantic λΠ lexicographic key against Blink's packed `CascadePriority` comparison over 18 precedence, equality, and field-boundary pairs. It includes encoded origin/importance, tree and layer order, style flags, rule/declaration indices, and the already-applied bit; CSS rule collection and application remain outside scope.

27. **`examples/blink_css_cascade_map.cpp`**:
   * Composes the fixed-prefix packed priority fields with per-property list insertion and first-eligible `FindRevertLayer`. Eleven insertion-prefix states and 99 prefix/bound lookups agree with the C++ transcription, including weaker insertion and new-before-old equal-priority ties.

28. **`examples/blink_tokenizer_attribute_name_bisimulation.cpp`**:
   * Models AttributeName accumulation from an existing name prefix, including lowercase normalization, whitespace, slash, `=`, `>`, EOF, and quote/`<` parse-error append-and-continue behavior.
   * Fourteen finite input sequences compare a C++ transcription with typed λΠ. Duplicate attributes, source-range tracking, null/preprocessor behavior, attribute values, and token/DOM effects remain unmodeled.

29. **`examples/blink_tokenizer_attribute_value_bisimulation.cpp`**:
   * Models before-value, double-quoted, single-quoted, unquoted, after-quoted, and self-closing transitions with explicit value accumulation, parse-error state, and token-emission outcomes.
   * Twenty finite traces compare C++ and typed λΠ. Ampersands terminate at an opaque character-reference handoff that records the quoted or unquoted return mode; entity decoding, null preprocessing, range tracking, and token/DOM internals are outside the model.

30. **`examples/blink_css_revert_color.cpp`**:
   * Composes strict `revert-layer` candidate selection, `ResolveRevertTo` fallback to `unset`, and inherited `color` handling. Twelve finite C++/λΠ cases cover candidate hits/misses, equal-layer skips, ties, CSS-wide values, parent/root behavior, and a selected priority rejected as not weaker.

31. **`examples/blink_css_match_winner.cpp`**:
   * Composes conjunctive tag/id/class matching with collection of only matching declarations, then winner selection for one property using a three-field abstract lexicographic priority.
   * Nine finite rule sets compare the C++ transcription with typed λΠ, including a no-match result, compound selectors, the second and third priority fields, and equal-priority later-input tie selection.
   * The priority fields are a named projection, not Blink's complete `MatchedRule` sort key or `CascadePriority`; parsing, combinators, attribute/pseudo selectors, layers, origins, importance, property application, and full stylesheet ordering remain outside scope.

32. **`examples/blink_numeric_html_entity_bisimulation.cpp`**:
   * Models numeric reference prefix/decimal/hex scanning, finite radix transitions, malformed-prefix rollback, semicolon and terminator outcomes, and exhausted-input rollback. Forty-nine per-character traces compare C++ and typed λΠ, including the U+10FFFF boundary, overflow-to-invalid latch, and later-digit retention.
   * Termination connects tested accumulators to replacement, an independent λΠ transcription of all 32 pinned U+0080–U+009F mapping entries (including unchanged C1 controls), U+007F/U+00A0 pass-through, and ordinary output. Radix transitions remain finite to the tested traces.

33. **`examples/blink_html_named_entity.cpp`**:
   * Retains a compact 11-entry λΠ trie for focused longest-prefix and attribute-guard cases.
   * Exact cursor rewind and `SegmentedString` mutations are not modeled; numeric references are handled separately.

34. **`examples/blink_html_named_entity_states.cpp`**:
   * Generates 9,854 named states from all 2,231 pinned CSV rows, with a distinct `walk_<id>` and `prefix_<id>` rewrite head for each state. `RuleIndex` dispatches their character transitions without scanning unrelated rewrite rules.
   * Twelve finite C++/typed-λΠ result comparisons cover semicolon-terminated matches, longest-prefix misses, semicolonless attribute acceptance/rejection, two-codepoint output, EOF prefixes, and no match. The latest `make` run generated 59,258 rules in 18 ms, indexed them in 6 ms, typechecked the observer by 24 ms total, and reduced cases in 0–4 ms.
   * This uses the full table to generate the semantic machine, but compares only selected inputs. It abstracts exact source rewind, `SegmentedString` chunk suspension, and tokenizer mutation; the C++ comparison helper is a standalone reference, not a direct execution of Blink's parser.

35. **`examples/blink_process_entity_bisimulation.cpp`**:
   * Composes finite decoder outcomes with `HTMLTokenizer::ProcessEntity` for Data/RCDATA: success appends one or two codepoints and sets the entity flag, failure appends literal `&`, and incomplete input suspends with the prior token buffer retained. Eight finite outcomes agree between the C++ transcription and typed λΠ.
   * Decoder outcomes are fixture constructors rather than direct calls into the numeric scanner or named trie; exact stream rewind and tokenizer runtime behavior remain outside the model.

36. **`examples/blink_tokenizer_attribute_entity_bisimulation.cpp`**:
   * Composes bounded named/numeric decoder observations with the attribute-value character-reference state. Ten finite cases cover decoded append/entity flag, literal ampersand fallback, incomplete-input suspension, and restoration to double-quoted, single-quoted, or unquoted state.
   * The C++ decoder uses selected names and numeric fixtures; typed rules encode those fixture outcomes. Full-table reduction, arbitrary codepoint arithmetic, exact cursor rewind, and stream lifecycle remain outside scope.

37. **`examples/blink_css_selector_color.cpp`**:
   * Composes restricted tag/id/class matching and matched-only declaration collection with three-field abstract priority selection, then applies `color` explicit/initial/inherit/unset/missing behavior using parent or root context.
   * Twelve finite C++/typed-λΠ cases compare match/miss, winner and tie behavior, CSS-wide keywords, and inherited/root outcomes. It does not cover full selector syntax, priority ordering, cascade origins/layers, actual CSS color values, or computed-style lifecycle.

38. **`examples/blink_css_revert_origin.cpp`**:
   * Models `TargetOriginForRevert`, `CascadeMap::Find(property, origin)` candidate filtering, and `ResolveRevertTo` fallback using the origin/importance priority field plus rule index, with other `CascadePriority` fields fixed.
   * Eleven finite C++/typed-λΠ comparisons cover UA/user/hint/author/animation target origins, absent and no-origin candidates, weaker resolution, and equal/stronger priority fallback. It does not model CSS-wide value recursion, declaration collection, or full packed priority ordering.

39. **`examples/blink_html_named_entity_remainder.cpp`**:
   * Extends the all-table named-reference state model with `decoded`, `noEntity`, `guardReject`, and `needMore` outcomes, consumed spelling, numeric cursor offset, and remaining text. λΠ computes accepted remainder with generic list-prefix operations.
   * Twelve finite C++/typed-λΠ comparisons and one two-chunk continuation cover successful matches, overlong-prefix rollback, attribute guard rejection, incomplete prefixes, and preserved attribute context. Offsets are relative to candidate text after `&`; `SegmentedString` segment mutation and tokenizer state/token mutation remain outside scope.

40. **`examples/blink_css_revert_origin_color.cpp`**:
   * Composes the `revert` target-origin scan and weaker-priority fallback with CSS-wide defaulting for inherited `color`, including explicit, initial, inherit, unset, missing-candidate, parent, and root cases.
   * Twelve finite C++/typed-λΠ comparisons use the origin/importance plus rule-index priority projection from the separate revert-origin model. Other `CascadePriority` fields, actual color-scheme state, `currentColor`, visited/animated color behavior, and style-tree effects are omitted.

41. **`examples/blink_segmented_entity_source_bisimulation.cpp`**:
   * Models bounded `SegmentedString` source chunks, cursor advancement, consumed offset, the `PushIfPossible` rewind and prepend fallback, and `Prepend(kUnconsume)` rollback.
   * Per-operation C++/typed-λΠ checks cover one/two-character rewind, segment-boundary and mismatch fallback, a longer rollback followed by longest-prefix replay, and Data/attribute suspension, append, resume, and completion. The traces are bounded ASCII cases; general trie lookup and entity decoding are separate.

42. **`examples/blink_css_revert_origin_full_priority.cpp`**:
   * Composes revert-origin selection and inherited `color` defaulting with a typed semantic key for every `CascadePriority` field in pinned high/low significance order; the C++ transcription independently compares the packed high and low words.
   * Eighteen composition cases and twelve priority boundary pairs cover origin/importance, tree and layer reversal, try/inline flags, rule/declaration indices, and already-applied. The λΠ field domain is finite; packed 96-bit arithmetic and full renderer behavior remain outside scope.

43. **`examples/blink_named_entity_segmented_composition.cpp`**:
   * Its public `observe(CursorSource, InputEnd, context)` path generates `sourceWalk_<state>` rules that consume the typed segmented Source directly, including cross-segment transitions, unconsumed mismatch lookahead, longest-terminal correction, attribute guarding, and `streamingExhausted` versus `finalEOF`. Need-more preserves token/context and suspends; completion appends one/two codepoints and sets the entity flag, or appends literal `&` on failure.
   * Ten direct cursor-source cases cover split edges, overread and exact attribute guards, no-match, one/two-character Push rollback, longer Prepend rollback, and both exhaustion modes. Fourteen legacy finite comparisons remain as a separate observer cross-check. A typed context wrapper derives the named-entity attribute guard from Data/double-quoted/single-quoted/unquoted mode, then the result is applied directly to a cursor-preserving tokenizer state. Seven handoff fixtures and pending/append/resume in all four contexts compare output, entity flag, restored state, and exact cursor source. Double-quoted `amp;"`, single-quoted `NotEqualTilde;'`, and unquoted `notin;>` fixtures retain their value delimiters in the returned cursor. The latest build generated 177,709 rules. The typed `enterCharacterReference` transition consumes `&`, maps the originating Data/double-quoted/single-quoted/unquoted value state to a cursor context, and sends the remaining CursorSource into the direct walker. Direct checks compare the full CursorSource constructor, current segment before/remaining lists, queued segment partition, and global offset after rollback; backing String identity and intermediate cursor positions are omitted.


44. **`examples/blink_tokenized_p_start_dom.cpp`**:
   * Scans bounded raw `<p ...>` source into a typed ordered `StartTagToken` (including quoted/unquoted values and a typed `&amp;` decode), then passes that token directly into p-scope pop and attributed DOM insertion. Three finite inputs include no attributes, multiple ordered attributes with a decoded reference, and reversed attribute order; p-scope hit/miss states are compared as full token and DOM/stack projections against an independent C++ tokenizer/insertion transcription.
   * The same normalized token-to-DOM state feeds a typed selector/color/layout/paint projection. Three cases compare matching fixed `p`, `.beta`, and `#alpha` selectors, priority selection, and color defaulting with a C++ transcription; a case with both class and ID matches also reverses declaration order and checks that the higher modeled priority still wins. The final artifact is an abstract empty-block projection carrying the computed color, not visible text paint.
   * This is a bounded tokenizer transcription mapped to Blink source branches, not execution of Blink's tokenizer. Selector syntax, full `CascadePriority`/`StyleCascade`, color-scheme selection, `currentColor`, style lifecycle, non-empty layout/paint, Unicode, duplicate-attribute behavior, parse-error reconsumption, and custom/sanitizer paths remain outside scope. Source anchors include `html_tokenizer.cc:852-1044`, `html_tree_builder.cc:875-881`, `html_construction_site.cc:986-988,1240-1248,1458-1462`, `computed_style.cc:800-803`, and `layout_object.cc:3133-3156`.

---

## License

MIT
