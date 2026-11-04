# Blink renderer source inventory and semantic-model coverage

Snapshot: Chromium `99d77c2af39f60ee06a5516ad5428e4d2aa61da4`, checked out at
[`vendor/chromium-src`](../chromium-src).

These counts were computed from `git ls-tree -r --name-only HEAD
third_party/blink/renderer` and compared with files present in the sparse
checkout. They describe path coverage, not whether each file was semantically
understood or converted.

| Renderer area | Tracked paths | C/C++ source/header paths |
|---|---:|---:|
| `core` | 9,226 | 7,071 |
| `modules` | 5,141 | 3,139 |
| `platform` | 3,755 | 2,748 |
| `bindings` | 591 | 240 |
| `controller` | 62 | 53 |
| `extensions` | 22 | 11 |
| **Renderer total** | **18,986** | **13,266** |

Within `core`, the largest directly relevant directories include:

| Core directory | Tracked paths |
|---|---:|
| `css` | 1,167 |
| `html` | 1,064 |
| `dom` | 432 |
| `layout` | 863 |
| `paint` | 307 |
| `style` | 184 |

Current λΠ work is small compared with that source surface:

* `examples/blink_rendering_model.cpp` reduces one paragraph case through a
  typed HTML/DOM/style/layout/paint abstraction.
* `examples/blink_parser_lifecycle.cpp` models five branches from
  `HTMLDocumentParser::PumpTokenizerIfPossible` as typed `ParserAction` terms.
* `examples/blink_parser_bisimulation.cpp` models a finite deterministic
  coalgebra for that branch boundary, checks an explicit relation over 64
  control-flag states and terminal state, and compares a C++ transcription
  with two λΠ observers over the same 64 valuations.
* `examples/blink_tree_builder_p_start_bisimulation.cpp` models the shared
  `HTMLTreeBuilder::ProcessStartTagForInBody` path for `<p>` and the 24 other
  tags in its block-start case set, each with a distinct λΠ tag constructor.
  It checks 59 token/stack configurations plus a terminal state: `<p>` over
  eleven stacks, and each of the 24 other tags over both an in-scope stack and
  a stack where `<p>` is hidden under a scope marker. It models the `Push`
  cached-scope recurrence and checks it against extensional lookup for grouped
  HTML, foreign-content, document-fragment, and button markers. A separate
  finite case removes a non-top `p` and checks the recomputed cache bit.
  Resulting stacks are compared between the C++ transcription and λΠ.
* `examples/blink_tree_builder_p_end_bisimulation.cpp` models the resulting
  open-element stack for the `</p>` branch over ten finite stacks, including
  several consecutive implied-end-tag candidates and intervening unrelated
  tags. Its C++ reference explicitly applies the represented implied-tag pops;
  λΠ checks the resulting pop-through-`p` stack projection, and seven
  in-scope cases check that the explicit native sequence equals direct
  pop-through-`p`. It still omits parse-error reporting, namespace identity,
  unrepresented implied candidates, and DOM effects.
* `examples/blink_tree_builder_li_end_bisimulation.cpp` models the distinct
  `</li>` stack transition over eleven finite stacks: five list-item-scope
  hits and six misses that remain unchanged. It includes `ol`/`ul`, HTML,
  non-HTML, and document-fragment scope markers, plus a `button` case showing
  that button alone does not stop list-item scope. Five hit cases compare
  explicit implied-end-tag popping followed by pop-through-`li` with the
  final-stack abstraction used in λΠ.
* `examples/blink_tree_builder_block_end_bisimulation.cpp` models the shared
  in-body end-tag branch for all 28 block-element target names as distinct
  λΠ constructors. Thirty-five finite configurations compare the C++ and λΠ
  stack projections, including a scope hit for each target, implied-tag
  sequences, unrelated stack items, and marker/absent-target misses. It omits
  parse-error/DOM effects and arbitrary mixtures of multiple target tags.
* `examples/blink_css_cascade_bisimulation.cpp` models a restricted
  per-property cascade over five already-matched declaration sets, with a
  finite closure check between an imperative C++ scan and a λΠ priority
  semantics.
* `examples/blink_css_selector_bisimulation.cpp` covers tag, id, class, and
  compound-subselector matching on one element for nine finite cases, with a
  relation check between native and λΠ observations.
* `examples/blink_css_combinator_bisimulation.cpp` covers right-to-left
  compounds of tag/id/class tests with child/descendant relations over nine
  finite tree cases.
* `examples/blink_css_revert_layer.cpp` models the first eligible entry in an
  already sorted, strongest-first per-property priority list over twenty
  finite list/bound cases, including same-layer ties. It does not construct
  Blink's packed priority map.
* `examples/blink_css_priority_layer_normalization.cpp` checks a four-bit
  projection of `CascadePriority::ForLayerComparison` for both importance
  values across 32 field cases. Origin, tree order, and the rest of the packed
  priority remain fixed or omitted.
* `examples/blink_css_color_defaulting.cpp` models missing, `inherit`,
  `initial`, and `unset` outcomes for Blink's inherited `color` longhand, with
  parent-present and root cases. A C++ branch transcription and typed λΠ
  reduction agree on 24 finite cases; values are abstract color IDs.
* `examples/blink_style_recalc_gate.cpp` models the suppression-first
  `ShouldRecalcStyleFor` decision and the `ForChildren` propagation projection
  for `kRecalcChildren`/`kRecalcDescendants` plus the style-container-query
  flag. Its C++ transcription agrees with λΠ on 160 node/change inputs and 20
  child-change inputs; other flags and Element traversal remain outside scope.
* `examples/blink_tokenizer_open_tag_bisimulation.cpp` models the Data,
  TagOpen, and EndTagOpen dispatch branches of `HTMLTokenizer::NextTokenImpl`
  over ten character classes and seven states (70 finite transitions).
  Tag-name, markup, bogus-comment, and character-reference states are opaque;
  this is not full tokenizer behavior.
* `examples/blink_tokenizer_tag_name_bisimulation.cpp` extends the tokenizer
  boundary into bounded TagName accumulation and normalization. Nine finite
  sequences compare upper/lower letters and a preserved nonalphabetic name
  character across whitespace, slash, `>`, EOF, and exhausted-chunk outcomes.
  Unicode case mapping and input-stream preprocessing remain outside the model.
* `examples/blink_dom_insert_html_element.cpp` models normal HTML element
  attachment using explicit DOM node parent IDs, open-element IDs, and a fresh
  ID counter. Four finite DOM/stack states agree between the C++ reference and
  λΠ. Foster parenting, insertion adjustment, sanitizer, and callbacks are not
  represented.
* `examples/blink_paint_chunker_bisimulation.cpp` models PaintChunker grouping
  by property state and forced boundaries around foreign-layer/scrollbar
  display items. Eight finite sequences agree between the C++ transcription
  and λΠ. Display-item creation, chunk property payloads, and full paint artifact
  lifecycle are outside the model.
* `examples/blink_layout_dirty_bisimulation.cpp` models the clean-to-dirty
  transition of one layout object and propagation of child-dirty flags through
  a finite rooted container chain. Fixtures assume no relayout-boundary early
  return. Display locks, out-of-flow jumps, view scheduling, simplified layout,
  and paint invalidation remain outside the model.
* None of these programs is generated from the C++ source. None models the full
  parser, style cascade, layout algorithms, paint structures, invalidation,
  script interactions, or incremental lifecycle.

The inventory is an accounting aid for the remaining conversion work, not a
claim that the renderer can be represented by a single small rewrite system.
