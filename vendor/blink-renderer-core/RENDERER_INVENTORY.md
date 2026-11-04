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
