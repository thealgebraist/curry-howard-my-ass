# Initial source-to-model map

This map records direct observations from the pinned files listed in
[`SOURCE.md`](SOURCE.md) and the full renderer checkout at that commit. The λΠ terms below are abstractions of stage outputs;
they do not model Blink object identity, incremental invalidation, caches,
streaming, layout constraints, or painting internals yet.

| Model stage | Blink evidence in this snapshot | What is represented so far |
|---|---|---|
| HTML input → DOM | `core/html/parser/html_document_parser.cc`: `HTMLDocumentParser::PumpTokenizerIfPossible` and `PrepareToStopParsing`; `core/README.md` identifies DOM as a core rendering stage | One already-structured `<p>hello</p>` input is mapped to a DOM constructor. Tokenization, tree construction rules, scripts, and malformed markup are absent. |
| DOM + CSS → styled DOM | `core/css/style_engine.cc`: `StyleEngine::RecalcStyle` and `UpdateStyleAndLayoutTree`; `core/css/README.md` links the style-calculation and invalidation design docs | The rendering example uses one `p` rule. Separate models cover simple tag/id/class matching and priority winners for five already-matched declaration sets. Selector parsing, combinators, inheritance, custom properties, and invalidation are absent. |
| Styled DOM → layout tree | `core/layout/README.md` names LayoutSubtreeChange, PreLayout, PerformLayout, AfterPerformLayout, and LayoutClean; it describes nested margin/border/padding/content boxes | One abstract block box containing a text run. Geometry, fragmentation, intrinsic sizing, writing modes, and lifecycle transitions are absent. |
| Layout tree → paint artifact | `platform/graphics/paint/paint_artifact.h` defines `PaintArtifact` as paint chunks plus display items and describes caching/immutability | One abstract block paint item and text paint item. Display items, chunks, property trees, cache validity, and raster/compositor behavior are absent. |

The correspondence is a working semantic hypothesis derived from the named
files, not a proof that the model is equivalent to Blink. In particular, the
four rewrite rules in `examples/blink_rendering_model.cpp` encode a single
closed input, while Blink's implementation is stateful and incremental.

`examples/blink_parser_lifecycle.cpp` models a second slice of
`HTMLDocumentParser::PumpTokenizerIfPossible`: the stopped/paused gate for
calling `PumpTokenizer`, followed by selected outcomes for fragment abort,
yield, EOF, and delayed completion. Its Bool inputs are abstracted state
observations, and the model omits stylesheet blocking, budgets, task scheduling
effects, nested parser sessions except for the abort guard, and parser/tree
construction state.

`examples/blink_parser_bisimulation.cpp` takes the next step: it gives the
selected decision boundary a finite deterministic coalgebra view. A state is
the six observed control flags; the observation is the selected parser action;
the modeled successor is a terminal boundary state. An explicit 65-pair
relation covers the 64 flag valuations and terminal pair. The λΠ checker
reduces a closure condition that compares observations and checks that
successors remain related. A C++ transcription is also compared against both
λΠ observers for all 64 flag valuations.

This establishes agreement only for that extracted, single-step boundary. The
native helper is a transcription, not execution of Blink's linked
`HTMLDocumentParser`; it omits the surrounding methods and runtime. The
current λΠ implementation also does not verify rewrite-rule termination. The
rules in this example recurse over finite Peano values and a finite pair list,
but that property is reviewed manually rather than certified by the kernel.

The full checkout shows why CSS style resolution cannot be treated as a single
rule lookup. `core/css/resolver/style_cascade.h` describes collecting CSS rule
and animation declarations, then deciding which declarations to skip or apply.
`core/css/resolver/cascade_priority.h` encodes origin/importance,
shadow-including tree order, inline style, cascade layer order, and rule and
declaration indices. Its comments also describe matched-rule ordering by layer,
specificity, proximity, stylesheet index, and rule index. The initial λΠ
rendering example abstracts all of these dimensions away; the separate cascade
model below implements only a restricted priority projection.

That next slice is now implemented in
`examples/blink_css_cascade_bisimulation.cpp`. It assumes declarations have
already matched one property and restricts all candidates to author-normal
origin, one tree scope, one layer, and non-inline style. The rank tuple keeps
only specificity order, proximity order, stylesheet position, rule position,
and declaration position. These are directly supported by
`core/css/element_rule_collector.h` (`MatchedRule::SortKey`),
`core/css/element_rule_collector.cc` (`SortMatchedRules`),
`core/css/resolver/cascade_priority.h`, and `core/css/resolver/style_cascade.h`.
The C++ scan and λΠ maximum-priority observer agree for five finite cases; the
finite relation checks observation equality and terminal-successor closure.
Selector matching, CSS parsing, property-specific application, origins,
importance, layers, shadow-tree order, animations, and CSS-wide keywords remain
outside this model. This is a semantic projection, not a translation of the
complete `StyleCascade` class.

`examples/blink_css_selector_bisimulation.cpp` models another upstream path:
`ElementRuleCollector::CollectMatchingRulesForListInternal` selects
`EasySelectorChecker::Match` for easy selectors, and
`selector_checker-inl.h` handles tag, id, and class selectors in `MatchOne`.
The λΠ selector model checks the subject element's tag/id/class set and
conjoins simple selectors in one compound. Its finite relation covers nine
selector/element inputs plus a terminal state. It assumes one namespace and
case-sensitive names, and it does not implement Blink's parsed selector
representation or general recursive/combinator matching.

`examples/blink_css_combinator_bisimulation.cpp` extends the selector path
with child and descendant chains whose compounds combine tag, id, and class
tests over a finite tree. Its chain is
stored subject-first/right-to-left, matching the order described by
`core/css/css_selector.h`. The native matcher and λΠ rules both follow parent
links; descendant relations search ancestors, while child relations inspect
only the immediate parent. The nine finite cases follow the easy-matcher
constraint in `selector_checker-inl.h` that rejects a child combinator after
a descendant combinator. The model omits attribute selectors, pseudo-classes,
namespaces, sibling relations, and the general selector matcher.

## Tree-builder boundary: paragraph start tags

`core/html/parser/html_tree_builder.cc` provides a concrete tree-construction
boundary beyond the parser pump model. In `ProcessStartTagForInBody`, `<p>`
and the 24 other tags in its block-start case set share the branch that calls
`ProcessFakePEndTagIfPInButtonScope` before `tree_.InsertHTMLElement`.
The helper checks `OpenElements()->InButtonScope(kP)` and, if true, creates a
fake `</p>` token routed through `ProcessEndTag`. `InButtonScope(kP)` returns
the cached `has_p_element_in_button_scope_` bit on the current stack item;
`Push` sets it for a `p`, otherwise inherits the lower item's bit unless the
new item is a button-scope marker. The model in
`examples/blink_tree_builder_p_start_bisimulation.cpp` gives each of the 24
non-`p` names its own constructor, represents `<p>` on eleven top-first stacks,
and checks every other name on both a stack with `p` in scope and one with `p`
hidden below a marker. It constructs the cache bottom-up with this recurrence,
and checks that it agrees with an extensional scan for `p` above the nearest
button-scope marker. It groups all concrete HTML scope markers into one
constructor, all non-HTML scope markers into another, and represents document
fragments separately; it does not distinguish names within those categories.
The finite relation contains 59 token/stack pairs and a terminal state. For
each input, the C++ transcription and λΠ semantics compare the resulting
open-element stack: close through the existing `p` when it is in button scope,
then push the incoming element; otherwise just push the incoming element.
A separate transition models removing a non-top `p` below one ordinary top
element, then recomputes that affected top cache bit using the same recurrence.
Its λΠ result agrees with extensional lookup on the resulting stack. Other
pop/remove/replace sequences, parse-error effects, and DOM mutation are
omitted. The result-stack transition models the final stack effect of implied
end-tag processing, not its diagnostics or other side effects.
The C++ side is a transcription, not execution of Blink's `HTMLTreeBuilder`.

`examples/blink_css_revert_layer.cpp` models the ordered lookup used for
`revert-layer`. `CascadeMap::FindRevertLayer` iterates a per-property list and
returns the first entry whose `ForLayerComparison()` is below the reverting
priority. `CascadePriorityList::InsertKeepingSorted` maintains strongest-first
order, inserting equal priorities before older entries; `CascadeMap::Add`
pushes a new stronger priority and inserts weaker/equal priorities into that
ordered list. The λΠ transition now scans an explicitly strongest-first list
and returns its first eligible declaration, preserving the list's represented
tie order. The C++ transcription uses the same scan. Twenty finite
(declaration-list, reverting-layer) inputs were type-checked and reduced in
λΠ, and agreed with the operational scan. The source definitions are in the
pinned checkout's `third_party/blink/renderer/core/css/resolver/cascade_map.cc`
(`FindRevertLayer`, `Add`), `cascade_map.h`
(`CascadePriorityList::InsertKeepingSorted`),
`cascade_priority.h` (`ForLayerComparison`), and `style_cascade.cc`
(`ResolveRevertLayer`, `ResolveRevertTo`).

The model assumes the input priorities are already in Blink's sorted order;
it does not formalize `CascadePriority`'s packed bit encoding or all
origin/importance/tree-order/layer comparisons. Its natural-number layer field
only supplies the eligibility projection, while earlier list position
represents other priority and tie-order information. This covers `Find`'s
ordered search shape, not construction of the real map. No eligible candidate
maps to the model's unset sentinel (numeric zero), corresponding to
`ResolveRevertTo`'s missing-candidate branch. CSS `revert`, `revert-rule`,
recursive substitution, and property application remain outside scope.

## Tree-builder boundary: paragraph end tags

`examples/blink_tree_builder_p_end_bisimulation.cpp` models the open-element
stack projection of the `HTMLTag::kP` branch in
`core/html/parser/html_tree_builder.cc:2296-2308` (line numbers refer to the
pinned checkout). `HTMLElementStack::InButtonScope` reads the cached bit for
`p` at `core/html/parser/html_element_stack.cc:493-497`; the bit's update
recurrence is at lines 570-578. `IsButtonScopeMarker` is defined at lines
145-152: it uses HTML scope-marker tags, document-fragment roots, `button`, and
the non-HTML scope-marker predicate. The model uses grouped marker constructors
for these categories; its `foreignMarker` stands only for a non-HTML scope
marker, not every foreign-content element. When `InButtonScope(kP)` fails, Blink
reports a parse error and processes a fake `<p>` start tag before reprocessing
the end tag. The model captures the resulting stack effect: a synthetic top
`p` is popped, leaving the original stack. When `p` is in scope, it models
`GenerateImpliedEndTagsWithExclusion(p)` followed by `PopUntilPopped(p)` as
discarding the stack prefix through that `p`. `HTMLConstructionSite::HasImpliedEndTag`
at `core/html/parser/html_construction_site.cc:113-130` recognizes `dd`, `dt`,
`li`, `option`, `optgroup`, `p`, `rb`, `rp`, `rt`, and `rtc` in the HTML
namespace. The C++ reference represents four of those candidates (`li`, `dt`,
`option`, and the excluded `p`) plus unrelated tags, and explicitly performs
the eligible top pops before `PopUntilPopped`. Typed λΠ intentionally captures
the final pop-through-`p` abstraction. Ten finite stacks were compared between
the native projection and λΠ; seven in-scope configurations also assert that
the explicit implied-pop-then-pop algorithm returns the same suffix as direct
pop-through-`p`. Cases include several consecutive implied candidates,
intervening unrelated tags, nested `p`, and scope markers. This stack
projection does not model the fake-token control flow itself; it abstracts
parse-error reporting, candidates outside the represented subset, namespace
identity for ordinary tags, and DOM effects. It does not establish full `</p>`
processing equivalence.

## Tree-builder boundary: list-item end tags

`examples/blink_tree_builder_li_end_bisimulation.cpp` adds a distinct typed
λΠ `endLi` transition for the `HTMLTag::kLi` branch at
`core/html/parser/html_tree_builder.cc:2309-2319`. Blink first checks
`HTMLElementStack::InListItemScope(kLi)` (`core/html/parser/html_element_stack.cc:480-481`);
on a miss it reports a parse error and returns without changing the stack. On
a hit it calls `GenerateImpliedEndTagsWithExclusion(li)`, optionally reports a
parse error if the current item is not `li`, then calls `PopUntilPopped(li)`.
The model represents this as a separate scope predicate, recursive implied
candidate removal with `li` excluded, and the final pop-through-`li` operation.

The scope predicate follows `IsListItemScopeMarker` at
`core/html/parser/html_element_stack.cc:85-94`: besides the HTML scope-marker
set, the HTML branch treats document fragments and `ol`/`ul` as markers; the
non-HTML branch uses `IsScopeMarkerNonHTML` at lines 63-73. λΠ groups the HTML
scope-marker set, non-HTML marker set, and fragment roots into distinct
constructors. In particular, `button` is not itself a list-item-scope marker;
one finite hit case checks that distinction.

The C++ reference models the complete `HasImpliedEndTag` candidate list
(`html_construction_site.cc:113-130`) as HTML constructors and performs
eligible popping before `PopUntilPopped`. Eleven stacks are compared: five
scope hits and six misses, including `ol`, `ul`, HTML, non-HTML, and fragment
markers. For each hit, an explicit implied-pop sequence is checked against the
direct final-stack projection through the same `li`. The model does not
represent token diagnostics, the current-node parse-error condition, namespace
identity for ordinary tags, actual DOM effects, or state beyond the open
element stack. It therefore checks this finite transition projection, not the
complete Blink tree-builder.

## Layout boundary: dirty-bit propagation

`examples/blink_layout_dirty_bisimulation.cpp` models a bounded part of
`LayoutObject::SetNeedsLayout` in `core/layout/layout_object_inlines.h` and
`MarkContainerChainForLayout` in `core/layout/layout_object.cc`. For a
target-first chain of rooted normal-flow objects, a clean target becomes
self-dirty and propagates child-dirty bits toward its container ancestors.
Propagation stops when an ancestor already needs self layout or child layout;
calling `SetNeedsLayout` on an already self-dirty target is idempotent and does
not repeat propagation. The C++ reference and typed λΠ rewrite model are
compared on eight finite chains, including dirty target, dirty self ancestor,
dirty child ancestor, singleton, and empty cases.

The model holds fixed `mark_parents` at `kMarkContainerChain` and abstracts
overflow/cache dirtiness. It omits display-lock stopping behavior,
out-of-flow containing-block jumps, unrooted subtrees, simplified-layout
propagation, orthogonal writing modes, and in-layout behavior. It assumes the
container chain does not trigger the `allow_subtree_layout_root` /
`ObjectIsRelayoutBoundary` early return; it does not model the view's layout
state or subtree scheduling. Paint invalidation is also omitted. This is a
state-transition projection, not a translation of the `LayoutObject`
implementation or layout algorithms.

`examples/blink_css_priority_layer_normalization.cpp` isolates the layer-field
normalization in `CascadePriority::ForLayerComparison` from pinned Chromium
commit `99d77c2af39f60ee06a5516ad5428e4d2aa61da4`. In
`third_party/blink/renderer/core/css/resolver/cascade_priority.h`,
`EncodeLayerOrder` XORs the 16-bit layer value with `0xFFFF` for important
declarations (line 45); `kLayerOrderOffset` is 33, `kLayerOrderMask` is
`0xFFFF << 33`, and `kImportantBit` is 19 in `high_bits_` (lines 82–95).
After assembling the upper 64 bits, `ForLayerComparison` tests packed bit
`kImportantBit + 32`, XORs the shifted layer mask (`kLayerOrderMask >> 32`),
then shifts right by `kLayerOrderOffset - 32` (lines 185–204). The model
exhaustively checks the low four bits of that 16-bit field for both importance
values: 32 λΠ inputs typed and reduced to the matching independent C++ packed
bit-operation transcription. On those four bits, important layer encoding
and normalization each complement with `0xF`, so the final projected field
equals the original layer value.

This is exact for the selected finite layer-field projection. It fixes origin
and tree order to zero and omits their normalization masks, inline/try-style
flags, lower rule/declaration indices, and the remaining packed 96-bit
priority comparison behavior. It does not prove equivalence of the complete
`ForLayerComparison` method.
