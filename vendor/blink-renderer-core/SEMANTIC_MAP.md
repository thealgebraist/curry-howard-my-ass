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

`examples/blink_css_match_winner.cpp` composes a deliberately small portion of
that path with declaration selection. The C++ reference filters declarations
by a conjunction of tag, id, and class tests, then chooses the greatest
three-field priority among only those matches; its recursive λΠ `collect` and
`best` definitions implement the same sequence. Exact ties select the later
input declaration. The finite fixtures include unmatched higher-priority
rules, a compound selector, an explicit match-set with no declarations, order changes in
the second and third priority fields, and an exact tie.

The source boundary is `element_rule_collector.cc:577-673`, where Blink rejects
non-matching rules and runs either `EasySelectorChecker::Match` or the general
checker; accepted rules reach `DidMatchRule` at lines 712-718. The simple tag,
id, and class tests are in `selector_checker-inl.h:218-245`. Blink's actual
matched-rule ordering stores layer, specificity, and inverse proximity in
`element_rule_collector.h:70-86`, then sorts by that key and source position in
`element_rule_collector.cc:1514-1534`. `StyleCascade::CollectFromMatchResult`
expands matched properties into the property map in `style_cascade.cc:498-524`.
`cascade_priority.h:53-73` documents the surrounding criteria: origin and
importance, shadow-including tree order, inline style, layer order, matched
rule/declaration indices, and applied state.
The model's priority tuple is an abstract projection and does not claim to
reproduce that full ordering or Blink's per-property map. It assumes selectors
are already parsed; namespaces, combinators, attributes, pseudo-classes,
layers, origins, importance, animations, CSS-wide keyword resolution, and
property-specific application are omitted. The C++ side is a transcription,
not execution of Blink's linked classes.

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

## Tree-builder boundary: shared block-element end tags

`examples/blink_tree_builder_block_end_bisimulation.cpp` models the shared
case set in `HTMLTreeBuilder::ProcessEndTagForInBody` at
`core/html/parser/html_tree_builder.cc:2230-2267`. The 28 target names each
have a distinct λΠ `BlockTarget` constructor: `address`, `article`, `aside`,
`blockquote`, `button`, `center`, `details`, `dialog`, `dir`, `div`, `dl`,
`fieldset`, `figcaption`, `figure`, `footer`, `header`, `hgroup`, `listing`,
`main`, `menu`, `nav`, `ol`, `pre`, `search`, `section`, `summary`, `select`,
and `ul`. The transition checks scope for that target, generates implied end
tags, then pops through the target; a scope miss leaves the stack unchanged.

The target-first order within Blink's `InScopeCommon` loop is at
`core/html/parser/html_element_stack.cc:420-430`: a matching HTML element
returns true before the marker predicate can stop the scan. The model groups
the standard HTML marker names listed in lines 48-56 into one constructor,
uses a separate constructor for `IsScopeMarkerNonHTML` at lines 64-74, and
uses another for document-fragment roots. It represents `select` separately
because it is both a target and an HTML scope marker. The λΠ
`generateImpliedEndTags` operator models `GenerateImpliedEndTags` at
`core/html/parser/html_construction_site.cc:113-130,1529-1532`, using Blink's
`dd`, `dt`, `li`, `option`, `optgroup`, `p`, `rb`, `rp`, `rt`, and `rtc` set.

Thirty-five finite configurations were compared between the C++ transcription
and λΠ: one scope-hit stack for each distinct target plus nested implied-tag,
unrelated-top, select-marker, scope-miss, and absent-target cases. For the 31
hits, the explicit implied-end-tag pass followed by pop-through-target was
also checked against direct pop-through-target on the original stack; this is
the final-stack abstraction boundary. The model omits parse-error reporting,
the current-node diagnostic, DOM effects, foreign namespace identity for
ordinary tag items, and general mixtures of multiple different block target
tags in one stack. Its target-matching rules cover the enumerated finite stack
cases; this check is not a proof of all possible stacks or full tree-builder
equivalence.

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

## CSS `color` defaulting boundary

`examples/blink_css_color_defaulting.cpp` models the defaulting decision for
the `color` longhand, whose metadata in pinned commit
`99d77c2af39f60ee06a5516ad5428e4d2aa61da4` marks it `inherited: true` and gives
it `StyleColor(Color::kBlack)` as its default (`core/css/css_properties.json5`
lines 1398–1419). `StyleBuilder::ApplyProperty` in
`core/css/resolver/style_builder.cc` lines 104–132 turns explicit `inherit`,
`initial`, and `unset` values into `ApplyInherit` or `ApplyInitial`; if
`inherit` or `unset` has no parent style it rewrites the value to initial.
`StyleResolverState::IsInheritedForUnset` at
`core/css/resolver/style_resolver_state.cc` lines 164–167 returns
`property.IsInherited()` (with an extra highlight-style condition omitted in
this model). Therefore explicit `unset` for ordinary `color` follows the
parent when present and the initial value at the root. Missing `color` is
modeled from the base-style initialization split described in
`core/css/resolver/style_resolver.cc`: `InitStyle` receives a source for
non-inherited properties and a parent style (lines 1623–1649), and its base
style description says these sources are merged before matched properties are
applied (lines 1905–1923). Because `color` is in the inherited field group,
the model uses the parent value for a missing declaration when one exists.

The typed λΠ function and a separate C++ branch transcription agree on 24
inputs: four keyword/missing cases, parent present or absent, and three
abstract parent color values. Initial black is represented by zero. This is a
projection of the one property's defaulting choice, not a translation of the
generated `ApplyInherit`/`ApplyInitial` implementations or `ComputedStyle`
construction. It omits highlight-specific inheritance, pseudo-element and
shadow-boundary adjustments, actual color representation, cascade selection,
and all effects of color on dependent properties.

## Restricted selector-to-color pipeline

`examples/blink_css_selector_color.cpp` composes three existing projections:
tag/id/class matching and matched-rule transfer from
`selector_checker-inl.h:218-245` and `element_rule_collector.cc:1260-1295`,
per-property winner selection from an abstract three-field priority, and
`color` value defaulting described above. Its C++ and typed λΠ paths agree on
twelve fixtures spanning match/miss, priority selection and represented ties,
explicit values, `initial`/`inherit`/`unset`, and missing declarations with a
parent or at the root.

`ElementRuleCollector::CompareRules` uses the full `MatchedRule::SortKey` plus
source position (`element_rule_collector.cc:1514-1534`), while
`StyleCascade::CollectFromMatchResult` expands matched properties and inserts
their `CascadePriority` keys (`resolver/style_cascade.cc:498-523`). The example
uses only a named three-field priority projection and abstract color IDs. Its
tie behavior is tested against its finite C++ transcription; the check does
not establish equality with Blink's complete sort key or `CascadeMap` behavior.
It also omits selector parsing, combinators, attributes, origins/layers,
importance, actual `StyleColor` values, pseudo/highlight inheritance, and
`ComputedStyle` construction or lifecycle.

## HTML tokenizer boundary: open-tag dispatch

`examples/blink_tokenizer_open_tag_bisimulation.cpp` models the dispatch
branches of `HTMLTokenizer::NextTokenImpl` in
`core/html/parser/html_tokenizer.cc:279-410` at the pinned Chromium revision.
It covers the Data state, TagOpen state, and EndTagOpen state over ten abstract
character classes. Outcomes include data-character and EOF emission, start or
end tag opening with ASCII lowercasing abstracted into the action, markup
declaration handoff, character-reference handoff, and malformed-open recovery
such as reconsuming `<` or `</` in Data or entering bogus-comment handling.

Seven states times ten character classes produce 70 finite transitions. The
native transcription and typed λΠ transition agree on every pair. The Data
state assumes there is no buffered character token before `<`; processing
instruction support is fixed disabled, matching the default branch in the
selected source. TagName, MarkupDeclarationOpen, BogusComment, and
CharacterReference are opaque handoff states whose later input preserves that
state with an `opaqueContinue` observation. Consequently this is a model of
selected front-end dispatch only. It omits segmented-stream buffering and
preprocessing, full tag-name scanning, entity decoding, comments and doctypes,
attribute states, processing-instruction mode variation, token emission
internals, and tree-builder/DOM effects. The C++ side is a transcription, not
execution of Blink's linked `HTMLTokenizer`.

## HTML construction site: normal element insertion

`examples/blink_dom_insert_html_element.cpp` models a normal-path projection
of `HTMLConstructionSite::InsertHTMLElement` at
`core/html/parser/html_construction_site.cc:986-989`. Blink calls
`CreateElement` and creates a stack item, then calls `AttachOrFosterParent`.
`CreateElement` is defined at `html_construction_site.cc:1240`; the ordinary
non-custom branch appends token attributes with `SetAttributes` at
`html_construction_site.cc:1460-1461`, which calls `ParserSetAttributes` at
`html_construction_site.cc:95-100`. `Element::ParserSetAttributes` consumes the
ordered attribute vector at `core/dom/element.cc:4216-4248`. The model projects
that token-vector-to-new-element copy as an ordered list of finite
name/value constructors on the new DOM record; it does not execute Blink's
creation code. In `AttachOrFosterParent`
(`html_construction_site.cc:368-384`), the current insertion location is
captured before the optional open-element stack push. On the modeled
non-foster-parent path, `Attach` inserts the child at that captured parent.
The λΠ state represents the DOM as node records with explicit parent IDs,
ordered attributes, an open-element stack of IDs, and a fresh-ID counter. Its
transition copies the token's attribute list onto a node with the old stack top
as parent (or a root sentinel when empty), pushes the new ID, and increments
the counter. Four finite inputs compare empty attributes, one attribute, and
two attributes in different orders; C++ and typed λΠ agree on the resulting
DOM/stack records.

This is a projection of token-attribute copying, not execution of Blink's
`CreateElement`, `SetAttributes`, or `ParserSetAttributes` routines. Attribute
names and values are finite constructors; the check does not cover arbitrary
strings, duplicate-attribute tokenization, attribute namespace handling,
scripting-attribute stripping, or callbacks triggered during attribute setup.
It omits `ShouldFosterParent` and `FosterParent`, adjusted insertion
locations, `ShouldInsertChild`, fragment/root insertion points, sanitizer and
depth handling, flushing pending text, actual Node/Element allocation,
`BeginParsingChildren`/self-closing lifecycle, script/custom-element policy,
and callbacks. The comparison models parent linkage and stack/fresh-ID effects
for normal append only; it does not model full DOM construction or execution
of Blink's `HTMLConstructionSite`.

## HTML tree-builder p close followed by block insertion

`examples/blink_tree_builder_p_start_dom_insert.cpp` composes the shared block
start-tag case from `HTMLTreeBuilder::ProcessStartTagForInBody` at
`core/html/parser/html_tree_builder.cc:855-883`: it calls
`ProcessFakePEndTagIfPInButtonScope` and then `InsertHTMLElement`. The helper
at `html_tree_builder.cc:643-648` checks `InButtonScope(kP)` and routes a
synthetic `</p>` through end-tag processing. The p branch at
`html_tree_builder.cc:2296-2308` uses the scope check, implied-end-tag
generation, and `PopUntilPopped`; the model abstracts the implied-end-tag and
parse-error work and retains only the final pop-through-p stack effect.
`HTMLElementStack::InButtonScope` is at
`core/html/parser/html_element_stack.cc:493-497`; `PopUntilPopped` is at
`html_element_stack.cc:227-230`.

The unified finite state keeps each open entry's node ID and tag together.
Scope is evaluated over its tags; on a hit, entries through p are removed.
Insertion then uses the remaining top entry's ID (or root sentinel), appends a
node carrying token attributes, and pushes the fresh ID/tag entry. Four
fixtures compare p at the top, an ordinary entry above an in-scope p
(exercising recursive prefix removal), a button-marker miss with p below it,
and no p. They exercise the operation order through to the resulting DOM and
stack.

This is a bounded projection rather than execution of Blink's tree builder or
construction site. It models only p presence and button/HTML scope barriers,
not every scope marker or stack mutation. The fake end-tag parse-error,
`ProcessFakeStartTag` fallback, implied-end-tag side effects, custom-element
and sanitizer paths, foster parenting, insertion callbacks, and full DOM
semantics are excluded. Attribute names and values remain finite constructors.

## HTML tokenizer boundary: TagName scanning

`examples/blink_tokenizer_tag_name_bisimulation.cpp` models the TagName state
body in `HTMLTokenizer::NextTokenImpl` at
`core/html/parser/html_tokenizer.cc:407-424`. The tokenizer appends
`ToLowerCaseIfAlpha(cc)` while the character is not in `kTagNameSpecial`
(`html_tokenizer.cc:79`), then branches on slash, `>`, EOF, or tokenizer
whitespace. The λΠ `scanTagName` transition explicitly snocs each normalized
character onto a name list; upper- and lowercase inputs for the represented
letter classes both append the lowercase constructor, while a representative
nonalphabetic name character is preserved.

The delimiters yield the corresponding immediate branch observations:
whitespace consumes and enters `BeforeAttributeName`, slash consumes and
enters `SelfClosingStartTag`, `>` emits the current token and resumes Data,
and EOF reports a parse error then reconsumes in Data. EOF is modeled as
pending for the next Data-state call rather than consumed in TagName. An
exhausted finite input chunk returns a `TagName`/`awaitMore` result with the
accumulated name intact. Nine finite character sequences, including `DiV` →
`div`, were compared between a C++ transcription and typed λΠ.

The alphabet is bounded to six representative ASCII letters plus a
nonalphabetic character. The model does not encode Unicode case conversion,
NUL replacement, CR/LF preprocessing, or `SegmentedString` boundaries within
the tokenizer's scan loop. It also omits token attributes, start-versus-end
tag identity, EOF's subsequent Data-state emission, token buffering/emission
internals, and tree-builder/DOM effects. The C++ side is a transcription, not
execution of Blink's tokenizer.

## HTML tokenizer boundary: AttributeName scanning

`examples/blink_tokenizer_attribute_name_bisimulation.cpp` models a bounded
scan from an existing attribute-name accumulator, corresponding to
`HTMLTokenizer::NextTokenImpl`'s `kAttributeNameState` at
`core/html/parser/html_tokenizer.cc:856-898`. The initial attribute prefix is
assumed to have been added by the preceding `kBeforeAttributeNameState`
branch; this model covers the subsequent `AppendToAttributeName` behavior.
Ordinary represented letters pass through explicit lowercase normalization,
and a nonalphabetic `#` is preserved.

The λΠ transition models whitespace to `AfterAttributeName`, slash to
`SelfClosingStartTag`, `=` to `BeforeAttributeValue`, and `>` to token emission
in Data. EOF sets the parse-error observation and reconsumes in Data. Quote,
apostrophe, and `<` each set the parse-error bit, append that character to the
name, and continue scanning in AttributeName. The scan result carries both
the accumulated name and whether one of those parse errors occurred, so a later
delimiter does not erase the earlier observation. An exhausted finite input
returns an await-more result with the prefix and parse-error bit retained.
Fourteen finite sequences were compared between the C++ transcription and
typed λΠ.

`kAttributeNameSpecial` is declared at `html_tokenizer.cc:80-81`; the ordinary
append loop is at lines 856-861. Actual attribute range bookkeeping in
`EndAttributeName`, duplicate-attribute handling, first-character creation,
null handling, and the subsequent attribute-value states are omitted. So are
input-stream preprocessing details (including CR/LF and segmented-buffer
boundaries), Unicode case mapping beyond the representative ASCII letters,
token internals, and DOM effects. The comparison establishes the finite
AttributeName projection only, not full tokenizer equivalence.

## HTML tokenizer boundary: attribute-value states

`examples/blink_tokenizer_attribute_value_bisimulation.cpp` models the path
from `kBeforeAttributeValueState` through quoted and unquoted values, then the
after-quoted and self-closing dispatch boundaries. The corresponding branches
are `HTMLTokenizer::NextTokenImpl` in
`core/html/parser/html_tokenizer.cc:925-963` (before value), 966-1013
(double/single quoted), 1016-1047 (unquoted), 1049-1080 (character-reference
handoff), and 1082-1108 (after quoted/self-closing).

The transition carries an explicit value list, parse-error bit, state, and
action. It models quote selection; ordinary append behavior; the parse-error
append cases for `<`, `=`, and backtick before/unquoted; whitespace and `>`
termination; EOF reconsumption; and post-quote/self-closing outcomes. A slash
inside an unquoted value is appended as ordinary value text, as Blink's
unquoted branch requires; a slash after a quoted value takes the self-closing
path. The first-value and subsequent quoted/unquoted ampersands return distinct
opaque handoff states for `additional_allowed_character_` `>`, `"`, and `'`.

Twenty finite traces were compared between a C++ transcription and typed λΠ.
This covers the selected transitions, accumulation, error flag, and emission
action. It does not execute `ConsumeHTMLEntity`: entity success/failure,
decoded characters, the literal-ampersand fallback, and not-enough-input
behavior are outside the opaque reference boundary. It also omits full
`SegmentedString`/preprocessor behavior, NUL replacement, attribute-range
tracking, generated token internals, and tree-builder/DOM effects. The result
is finite evidence for these state transitions, not complete tokenizer
equivalence.

## HTML fragment fast-path wrapper

`examples/blink_html_fastpath_wrapper.cpp` models the public
`TryParsingHTMLFragment` route and its implementation report. `CanUseFastPath`
failure returns false before parser construction and leaves
`failed_because_unsupported_tag` unchanged. An allowed gate chooses the 8-bit or
16-bit implementation from the source representation. After `Run`, the wrapper
reports success/abort; when the optional output pointer exists it is set true
only for `kFailedUnsupportedTag` and false for other parse results. The
unsupported-context detail log occurs only when that parse result is present,
the runtime feature is enabled, and the context classification is not
`kSupported`. Typed λΠ and C++ models cover 320 gate/width/result/pointer/logging
states. Parser internals, real histograms, elapsed time, byte counts, and DOM
effects are abstract. Sources: `html_document_parser_fastpath.cc:1927-2004,1737-1771`.

## HTML fragment fast-path tag-name scan

`examples/blink_html_fastpath_tag_name.cpp` models `ScanTagname` over ASCII
letters and an abstract following-code-unit class. All-lowercase names use the
source fast path; uppercase letters cause a restart from the beginning and an
ASCII lowercase mapping. The name must be followed by whitespace, `>`, or `/`;
whitespace is skipped and the remaining cursor class is retained in the result.
EOF and invalid delimiters fail. Twelve finite name/ending observations compare
with a C++ transcription. Empty names are returned by the scanner and are
rejected later by `ParseElement`; this scanner model does not collapse those
stages. Source: `html_document_parser_fastpath.cc:942-977,790-794`.

## HTML fragment fast-path character-reference dispatch

`examples/blink_html_fastpath_reference_dispatch.cpp` models the gate and
branch structure in `ScanHTMLCharacterReference`. The scanner must find a
semicolon without hitting EOF, NUL, or the greater-than-20 lookahead failure; a
semicolon after 20 reference code units is represented as the accepted boundary.
The resulting reference must have at least two code units. Numeric references
consume a supplied numeric-decoder result; exact `amp`, `lt`, `gt`, and `nbsp`
names emit their direct output; other names use a supplied
`ConsumeHTMLEntity` observation. A non-empty fallback remainder selects the
source cursor-rewind branch. Fifteen finite C++/λΠ observations cover these
branches. The character scan, numeric accumulation/`AppendLegalEntityFor`,
entity table, fallback decoder, and concrete offsets are modeled elsewhere or
left as explicit inputs. Source: `html_document_parser_fastpath.cc:1209-1305`.

## HTML fragment fast-path escaped-text scan

`examples/blink_html_fastpath_scan_escaped_text.cpp` models the slow
`ScanEscapedText` loop as a recursive λΠ fold. Literal text and decoded
character-reference output append to the buffer; CR and CRLF each append LF;
`<` is left at the cursor and returns the accumulated text; NUL and decoder
failure return failure. Twelve finite C++/λΠ comparisons include one- and
two-unit decoder outputs. `ScanHTMLCharacterReference` itself is supplied as an
explicit decoded/failure observation; its parsing and cursor consumption,
source offsets, string storage, and buffer allocation are outside this layer.
Source: `html_document_parser_fastpath.cc:907-939`.

## HTML fragment fast-path text scan

`examples/blink_html_fastpath_scan_text.cpp` models the non-vectorized core
loop in `ScanText`: ordinary 8-bit/16-bit code units accumulate into a plain
span; `<` remains at the cursor and ends the span; encountering `&` or `\r`
resets scanning to the original start and delegates to `ScanEscapedText`; NUL
returns `kFailedContainsNull`. The typed λΠ scanner recurses over symbolic code
units and tracks the accumulated text, remaining input, and `is_8bit` flag.
Eleven finite sequences compare with a C++ transcription. The escaped scanner,
newline-then-spaces canonicalization fast check, SIMD implementation, and actual
string storage are omitted. Source: `html_document_parser_fastpath.cc:850-905,907-939`.

## HTML fragment fast-path child-loop step

`examples/blink_html_fastpath_children_step.cpp` models one iteration of
`ParseChildren`. A failed `ScanText` returns immediately. Otherwise plain or
escaped text append is recorded before control-flow checks; input end stops the
loop, a following `</` hands control to the container closer, and a start tag
checks the incremented nesting depth before parsing the child. A max-depth or
child failure returns after any earlier text append; a successful child is
appended and parsing continues. Typed λΠ rules and a C++ transcription cover
64 finite combinations of scan kind, end/close state, depth state, and child
result. Text contents, child recursion, node allocation, and DOM mutations are
abstracted. Source: `html_document_parser_fastpath.cc:1347-1380`.

## HTML fragment fast-path container closure

`examples/blink_html_fastpath_close_tag.cpp` models the post-child phase of
`ParseContainerElement`. A prior child-parser failure remains the reported
result because `Fail` retains the first failure. Clean EOF before a close tag
returns the container-end failure. Once `</` is reached, the remaining span
must contain more than the expected tag-name length; the fixed-length name
must match exactly or under ASCII lowercase mapping, and after skipped
whitespace the next code unit must be `>`. The model compares typed λΠ rules
with a C++ transcription across 36 child-state/name-match/length/delimiter
combinations. It abstracts source slicing/cursor changes and
`FinishParsingChildren`. Sources: `html_document_parser_fastpath.cc:1312-1319,1679-1715`; `SpanMatchesLowercase` is at `:288-305`.

## HTML fragment fast-path child dispatch

`examples/blink_html_fastpath_child_dispatch.cpp` models the parent-specific
child dispatch in `ParseChildren` and `ParseElement`. Flow containers use the
non-phrasing mode; phrasing-content containers use the phrasing-only mode. The
model includes tag-category outcomes for the pinned 17-tag set, the special
`<ol>/<ul>` to `<li>` and `<select>` to `<option>` dispatches, option element
child rejection, void elements, and the `inside_of_tag_a_` /
`inside_of_tag_li_` guards that prevent nested auto-closing elements. It compares
a typed λΠ reduction with a C++ transcription over 360 finite
parent/tag/flag states. Token scanning, attributes, recursive parsing, element
construction, and DOM effects remain outside scope. Source:
`html_document_parser_fastpath.cc:543-747,1347-1378,1600-1725`.

## HTML fragment fast-path attribute-name frontier

`examples/blink_html_fastpath_attr_boundary.cpp` models the decision at the end
of the lowercase scan in `ScanAttrName`. A lowercase/hyphen prefix that reaches
EOF triggers `kFailedEndOfInputReached`; an ASCII delimiter returns the prefix
without consuming the delimiter; another invalid name code unit also returns
the prefix, leaving the caller to reject the input; uppercase/digit input enters
the mapping loop, which lowercases the full valid name even if that loop reaches
EOF. Fourteen finite prefix/frontier fixtures compare with a C++ transcription.
The following caller behavior and arbitrary source strings remain unmodeled.
Source: `html_document_parser_fastpath.cc:990-1025`.

## HTML fragment fast-path attribute-name scanning

`examples/blink_html_fastpath_attr_name.cpp` models the name transformation in
`ScanAttrName`: lowercase ASCII letters and hyphens remain on the no-mapping
path; encountering uppercase ASCII or a digit triggers the full valid-name scan,
which lowercases uppercase letters. At end-of-input the lowercase fast path
returns its end-of-input failure, while the mapping path returns the accumulated
name and leaves later caller handling to `ParseAttributes`. Typed λΠ list
recursions for the fast-character predicate and ASCII lowercase map are compared
with a C++ transcription over 15 finite names and two ending conditions (30
states). Inputs are restricted to valid name characters; invalid-character
handling and the subsequent attribute loop are not modeled. Source:
`html_document_parser_fastpath.cc:990-1025`.

## HTML fragment fast-path attributes

`examples/blink_html_fastpath_attributes.cpp` models the decision in
`ParseAttributes` after each name has been scanned: names longer than two code
units beginning with `on` fail with the event-handler result; exact `is` fails
with the generic attribute result; then the parser accepts only `>` and `/>`,
and rejects duplicate canonical local names. The typed λΠ terms implement a
recursive special-name scan and duplicate predicate over lists, compared with a
C++ transcription for all six-name lists through length three and three
terminators (777 states). Case folding/name validity, value scanning, and
`ParserSetAttributes` are outside scope. Source:
`html_document_parser_fastpath.cc:1463-1561`.

## HTML fragment fast-path context dispatch

`examples/blink_html_fastpath_context.cpp` models the context-tag switch in
`HTMLFastPathParser::Run`. The `body` context is handled separately and records
whether the behavior flag strips initial whitespace. Among `SUPPORTED_TAGS`,
non-void tags are accepted as parser contexts and void tags are rejected.
Unlisted tags and foreign-namespace names reject. The pinned switch checks exact
HTML QName equality after `TagnameHash`, so a hash case alone does not establish
acceptance. The C++ transcription and typed λΠ result relation cover 76 finite
tag/namespace/behavior states. Hash implementation, element construction,
child parsing, and DOM effects are abstracted. Source: `html_document_parser_fastpath.cc:457-499,320-345`.

## HTML fragment fast-path eligibility

`examples/blink_html_fastpath_gate.cpp` models the ordered guard decision in
`CanUseFastPath`: included shadow roots reject first, followed by enabled
tracing, disallowed parser content policy, then form ancestry unless the context
is a template element. The final branch permits the fast path. A C++
transcription and typed λΠ reduction cover all 32 Boolean combinations. This is
only the entry eligibility decision; supported context-tag dispatch, parser
state, fragment construction, and DOM mutation are not covered. Source: `html_document_parser_fastpath.cc:1733-1770`.

## Numeric HTML character references

`examples/blink_numeric_html_entity_bisimulation.cpp` models the numeric path
of `ConsumeHTMLEntity` at `core/html/parser/html_entity_parser.cc:151-270`.
The finite state transitions cover the initial `#`, optional `x`/`X`, decimal
and hexadecimal digits, optional semicolon, unconsumed terminators, malformed
prefix rollback, and exhausted-input rollback. Finite radix lookup rules
compare representative decimal and hex traces with a C++ transcription.
Named-entity lookup is excluded.

Blink checks `result > UCHAR_MAX_VALUE` and latches `kInvalidUnicode` at
`html_entity_parser.cc:261-262`. The pinned `DEPS:2443-2445` selects ICU
revision `b09986242cfb165b9111ade98be6b042ef973b86`; ICU defines
`UCHAR_MAX_VALUE` as `0x10FFFF`. The λΠ model now compares each transition
against an independently declared finite radix table and covers exact-limit,
over-limit, and later-digit traces for both decimal and hex. The invalid
sentinel remains latched across subsequent digits in those traces.
`AppendLegalEntityFor` at `html_entity_parser.cc:138-149` replaces zero,
surrogates, and values above U+10FFFF, then applies a Windows-1252 extension
for U+0080-U+009F. The table at `html_entity_parser.cc:41-46` supplies all 32
entries, and `AdjustEntity` at lines 48-52 indexes it only in that interval.
The source-derived C++ table is in `examples/blink_html_numeric_entity_table.hpp`
and is shared by the standalone numeric transcription and the integrated raw
HTML pipeline. The typed λΠ mapping remains a separately declared table. The
model scans each
hexadecimal input U+0080-U+009F through the numeric state machine, then
compares the mapped result; this includes unchanged C1
controls such as U+0081, U+008D, U+008F, U+0090, and U+009D. It also checks
pass-through at U+007F and U+00A0, replacement for zero, a surrogate and the
invalid sentinel, plus ordinary values. Forty-nine per-character C++/λΠ
comparisons pass. Radix transitions are still limited to the finite fixtures
and do not establish arbitrary numeric accumulation.
`SegmentedString` rollback mechanics and tokenizer call-site append/error
behavior are also outside this slice.

## Named HTML character references

`examples/blink_html_named_entity.cpp` remains a compact 11-entry model for
focused longest-prefix and attribute-guard cases. The separate
`examples/blink_html_named_entity_states.cpp` generates a state machine from
all 2,231 rows of the pinned `html_entity_names.csv`: 9,854 trie states each
have distinct `walk_<id>` and `prefix_<id>` rewrite heads, and the compiled
`RuleIndex` dispatches character transitions by those heads. On the latest
`make` run, it generated 59,258 rules in 18 ms, built the index in 6 ms,
typechecked the observer by 24 ms total, and reduced twelve selected inputs in
0–4 ms each.
The C++ helper and typed λΠ model agreed on semicolon-terminated matches,
longest-prefix misses, semicolonless attribute acceptance/rejection,
two-codepoint output, EOF prefixes, and no match. The follow-up
`examples/blink_html_named_entity_remainder.cpp` adds `decoded`, `noEntity`,
`guardReject`, and `needMore` outcomes, plus consumed spelling and a remaining
text value for twelve selected inputs. This demonstrates full-table machine
generation and finite result agreement, not arbitrary-input equivalence; the
C++ helper is a separately written reference rather than a direct call into
Blink's parser.

The data flow is generated by `build/scripts/make_html_entity_table.py:122-148`
from `core/html/parser/html_entity_names.csv`; the generated table stores each
name with one or two codepoints. `html_entity_search.cc:33-62` narrows its
candidate range per character and records the most recent exact entry, which
supports longest-match behavior. `html_entity_parser.cc:78-134` handles
exhausted input, restores overconsumed characters to the latest match, and
rejects a semicolonless match in attribute context when the following
character is ASCII alphanumeric or `=`. The λΠ state machine follows names by
character and retains the most recent terminal, with per-state fallback rules.
It models lookup outcomes and the EOF-prefix decision. The remainder example
uses a generic list-prefix `length`/`after` operation to compute consumed
spelling, an offset relative to the input after `&`, and the remaining list.

`examples/blink_named_entity_segmented_composition.cpp` now makes public
`observe(Source, InputEnd, context)` traverse the 2,231-name trie directly over
typed source chunks. Each `sourceWalk_<state>` transition consumes a matching
current character, crosses exhausted chunk heads, and leaves mismatch
lookahead unconsumed. Per-state path and latest-terminal metadata drive
longest-match restoration, attribute guarding, and the explicit
`streamingExhausted`/`finalEOF` decision. The independent C++
`ConsumeNamedEntity` transcript checks prefix-only scanning, one/two-character
reverse `Push`, longer `Prepend(kUnconsume)`, exact-match no-rewind, and
longest-match reconsume. The typed tokenizer transition preserves buffered
output/context and suspends on need-more; on completion it appends one or two
decoded codepoints and sets the entity flag, or appends literal `&` on failure.
Ten direct CursorSource cases and fourteen legacy observer cross-checks pass. A typed `enterCharacterReference` transition consumes `&` and maps the originating Data/double-quoted/single-quoted/unquoted value state to a cursor context; the remaining CursorSource feeds the named-entity guard and direct trie walker. The resulting source outcome is applied to a cursor-preserving tokenizer state. Seven handoff fixtures and pending/append/resume in all four contexts compare output, entity flag, restored phase, and exact cursor source, including the consumed-ampersand offset. Double-quoted `amp;"`, single-quoted `NotEqualTilde;'`, and unquoted `notin;>` fixtures retain their delimiters. `RuleIndex` indexes 177,709 rules and builds in about 30 ms in the bounded run. Sources are
`html_entity_parser.cc:66-134`, `platform/text/segmented_string.h:92-115`,
`platform/text/segmented_string.cc:67-81`, and `html_tokenizer.cc:173-188,
298-302, 976-1026, 1049-1078`. The direct traversal consumes CursorSource at each edge;
comparison checks the full CursorSource constructor, current segment before/remaining lists, queued segment partition, and global offset after rollback. Backing String identity and intermediate cursor positions are omitted. The tokenizer state is a bounded projection
and does not include complete token attributes, preprocessor behavior, or full
tokenizer lifecycle. The old flat-list observer remains only as a separate
finite cross-check. The earlier focused examples below retain their own
selected-outcome coverage.
The earlier `examples/blink_segmented_entity_source_bisimulation.cpp` and
`examples/blink_html_named_entity_remainder.cpp` remain bounded flat-list
models. Their twelve selected outcomes and restart-based continuation do not
model `SegmentedString` segments or its actual `Push`/`Prepend` mutations.
Those limitations do not describe the direct traversal above, which does use
typed Source during trie lookup; its final-result comparison still omits the
backing String identity and intermediate cursor positions. Preprocessing
and numeric references remain separate.
`html_entity_table.h:35-53` defines each generated entry's one/two-value
layout and name accessors. `html_tokenizer.cc:173-185` and `:1049-1062` show
how the tokenizer preserves an incomplete reference for a later chunk and
returns to its character-reference state; those buffer/state mutations remain
outside this model.

## Data/RCDATA character-reference application

`examples/blink_process_entity_bisimulation.cpp` models the observable effects
of `HTMLTokenizer::ProcessEntity` (`core/html/parser/html_tokenizer.cc:173-189`)
as invoked from the Data and RCDATA reference states at lines 298-301 and
323-326. A successful result appends one or two decoded values to the character
buffer and sets the entity flag; failure appends literal `&`; insufficient
input suspends while retaining the prior buffer. The caller's return state is
observed as Data or RCDATA. Eight finite outcomes compare between a C++
transcription and typed λΠ.

The decoder outputs are finite constructors selected from numeric and named
fixtures. The scanner and entity trie are not reduced inside this composition,
so it does not prove that arbitrary input reaches the supplied result. Exact
`SegmentedString` rewind, preprocessing, token emission lifecycle, and complete
entity coverage remain outside scope.

## Attribute-value character-reference application

`examples/blink_tokenizer_attribute_entity_bisimulation.cpp` composes a bounded
`ConsumeHTMLEntity` transcription with
`kCharacterReferenceInAttributeValueState` (`core/html/parser/html_tokenizer.cc:1049-1078`).
It records the current attribute value, entity flag, result state, and action.
Success appends decoded values and sets the flag; failure appends literal `&`;
insufficient input suspends without changing the value; completion restores the
double-quoted, single-quoted, or unquoted state from the additional allowed
character (`\"`, `'`, or `>`). Ten finite cases cover semicolonless-name
rejection before ASCII alphanumeric/`=`, punctuation acceptance, numeric
success/failure, two-codepoint output, and the return-state choices.

The C++ decoder uses selected entity names and bounded numeric inputs; the
typed decoder is a table of those finite decoder outcomes composed with the
attribute transition. Neither side models arbitrary numeric accumulation in
this file, exact consumed/unconsumed suffixes, full `SegmentedString` chunking,
attribute range tracking, or the full tokenizer lifecycle.

## Segmented source rollback and tokenizer suspension

`examples/blink_segmented_entity_source_bisimulation.cpp` gives the source
cursor an explicit sequence of bounded segments and a consumed-character
offset. Its C++ operation transcription follows `SegmentedString::Advance`,
`AdvanceExpecting`, `Push`, `Prepend(kUnconsume)`, `Append`, and the
`PushIfPossible` fast rewind. Typed λΠ rules compare source structure and
consumed offset after each operation. Fixtures cover one/two-character
rollback, segment-boundary and mismatched-character prepend fallback, and a
longer `noti` rollback followed by replay of the matched `not`, leaving `i` at
the cursor. Data and attribute traces preserve the token buffer and
character-reference phase through need-more suspension, append, resume, source
replay, and entity completion. The example passes these bounded ASCII traces;
it does not compose with the general named-entity trie or numeric decoder and
does not establish arbitrary-input parser equivalence.

## Style recalculation gate

`examples/blink_style_recalc_gate.cpp` models selected branches of
`StyleRecalcChange::ShouldRecalcStyleFor` and `ForChildren` from pinned
Chromium commit `99d77c2af39f60ee06a5516ad5428e4d2aa61da4`. In
`core/css/style_recalc_change.cc:59-70`, `ShouldRecalcStyleFor` checks
`kSuppressRecalc` first (false), then `RecalcChildren()` (true), then
`Node::NeedsStyleRecalc()`, then `RecalcFlagDependent(node)`. In
`style_recalc_change.h:198-199`, `RecalcChildren` means propagation greater
than `kUpdatePseudoElements`, so `kIndependentInherit`, `kRecalcChildren`, and `kRecalcDescendants`
force this node predicate to true. The model preserves the distinction
when propagating to children: `ForChildren` at `style_recalc_change.h:116-119`
retains `kRecalcDescendants`, while other propagation values become `kNo`.

For the flag-dependent branch, the model includes only
`kRecalcStyleContainerChildren` (`style_recalc_change.h:33-38`): for an
Element with an existing computed style, `RecalcFlagDependent` checks
`DependsOnStyleContainerQueries()` (`style_recalc_change.cc:30-57`). A Text
node is the non-Element category and returns false at the `DynamicTo<Element>`
guard. The modeled flags are `kSuppressRecalc` (documented at
`style_recalc_change.h:59-61` as suppressing the current node and lost for
children) and `kRecalcStyleContainerChildren`. The latter also models the
no-computed-style branch: for an Element without old style, this one container
query flag is enough to recalc because `RecalcContainerQueryDependent()` is
true; no dependency bit is available to inspect. `FlagsForChildren` removes
`kRecalcStyleContainerChildren` at `style_recalc_change.cc:156-163` and clears
`kSuppressRecalc` for children at lines 178-188. Thus the finite projection
models suppression, child versus descendant propagation, an already-dirty
node, and an Element-only style-query dependency.

The typed λΠ decision and an independently written C++ transcription agree on
320 combinations (five propagation levels, four modeled flag sets, Element
or Text, computed-style present/absent, dirty/clean, dependency true/false). The separate `ForChildren`
projection agrees on 20 propagation/flag combinations. This is not full style
recalculation equivalence: other container-query, hover, content-visibility,
reattach and mark-reattach flags are omitted; nodes with no old computed style
are omitted; `ShouldUpdatePseudoElement`, `TraverseChild`, child dirty flags,
layout-subtree update flags, and `StyleEngine`/`Element` traversal are not
modeled. At `ShouldRecalcStyleFor`, the child and descendant propagation
levels intentionally yield the same true result; only `ForChildren` carries
the descendant distinction onward.

## Query-dependent style recalculation

`examples/blink_style_recalc_dependencies.cpp` models the complete boolean
expression in `StyleRecalcChange::RecalcFlagDependent(const Node&)` and
composes it with the suppression, propagation, and dirty-node order of
`ShouldRecalcStyleFor`. Source anchors are
`core/css/style_recalc_change.h:21-92,198-205,257-262` and
`core/css/style_recalc_change.cc:30-70` in pinned Chromium commit
`99d77c2af39f60ee06a5516ad5428e4d2aa61da4`.

The λΠ flags group Blink's paired current-container/descendant flags into six
families: size, style, scroll-state, anchored, descendant content-visibility,
and clear-precomputed-hover. The C++ transcription and typed reduction compare
all 64 grouped masks against seven old-style dependency categories, both node
kinds, and old style present/absent (1,792 cases). They then compare the full
`ShouldRecalcStyleFor` decision across five propagation levels, suppression,
dirty/clean, node kinds, style presence, dependency categories, and all flag
masks (35,840 cases). When old style is absent, only the four container-query
families cause a node recalc. With old style present, size flags match either
size-query dependence or highlight pseudo styles using container units; style,
scroll-state, anchored, and cached-hover dependencies match their respective
flags. The content-visibility flag contributes to `RecalcFlagDependent()`'s
early presence check but has no per-node match in this method.

This covers the per-node predicate, not all style recalc behavior. Each paired
child/descendant flag is represented as one family bit; `FlagsForChildren`,
`ShouldUpdatePseudoElement`, `TraverseChild`, mark-reattach interactions,
`Element::RecalcStyle` control flow, container-query scheduling, and DOM/style
tree traversal remain outside this model. The finite flag product is exhaustive
for these grouped inputs, not a proof of the full renderer lifecycle.

`examples/blink_style_recalc_children.cpp` models the child flag transform from
`style_recalc_change.cc:156-188` using exact 13-bit inputs. The C++ reference
and typed λΠ compare all 8,192 masks across four combinations of old-style
presence and `CanMatchSizeContainerQueries`, for 32,768 cases. The λΠ rule
clears `kRecalcStyleContainerChildren`; it clears `kRecalcSizeContainer` only
when the remaining size/suppression bits equal the current-container size bit
and an old style can match size queries; it always clears `kSuppressRecalc`,
and retains `kMarkReattach` only when suppression was set. Other bits pass
through unchanged. The model also composes this result with the five
`ForChildren` propagation outcomes. This isolates the flag and propagation
transform; it does not model Element traversal or subsequent recalc scheduling.

`examples/blink_style_recalc_traversal.cpp` models the adjacent traversal and
pseudo-element predicates from `style_recalc_change.cc:14-27,75-99` and
`style_recalc_change.h:143-205`.
`TraverseChildren` combines `RecalcChildren()`, the generic recalc-flag
presence check, and `ChildNeedsStyleRecalc`. `TraversePseudoElements` combines
`UpdatePseudoElements()`, the same generic flag check, and the child-dirty bit.
`TraverseChild` combines the node-level `ShouldRecalcStyleFor` result,
`MarkReattachLayoutTree`, child dirtiness, forced reattachment, generic recalc
flags, and layout-subtree dirtiness. The mark-reattach predicate is the exact
bit condition requiring mark and reattach while suppression is absent.

`ShouldUpdatePseudoElement` is modeled separately from node recalc. It returns
true for any non-`kNo` propagation, own dirty style, child dirty style, or
layout-subtree dirtiness. Otherwise, only size/style/scroll-state/anchored
query dependencies can trigger the result. Unlike `RecalcFlagDependent(node)`,
its size branch does not include highlight pseudo styles depending on
container units; it also does not check cached hover or content-visibility.
The C++/λΠ comparison covers 17,920 combinations of grouped flags, propagation,
three dirty bits, and seven dependency categories.

`IndependentInherit` models the predicate from
`style_recalc_change.cc:191-199`, called from `Element::RecalcOwnStyle` at
`core/dom/element.cc:5894-5903`. It is true only for `kIndependentInherit`, and
only when either the size-query family is absent or the old style does not
depend on size queries, and either the style-query family is absent or the old
style does not depend on style queries. Scroll-state and anchored query flags
do not disable this optimization in the inspected implementation. Typed λΠ
and C++ agree on 1,280 combinations of five propagation levels, 64 grouped flag
masks, and the two old-style dependency booleans. The caller's `parent_style &&
old_style` guard, inherited-property copying, and later style diff handling are
not modeled.

`examples/blink_style_recalc_copy.cpp` models the boolean boundary around the
independent-inheritance style-copy optimization. `Element::RecalcOwnStyle` at
`core/dom/element.cc:5894-5915` requires parent style, old style, and a true
`IndependentInherit` predicate before calling `PropagateInheritedProperties`.
That helper at `element.cc:5746-5795` rejects pseudo elements, dirty style,
element animations, custom style callbacks, absent computed style, style
animations/transitions, variable references/declarations, link state, applied
text decorations, and parent-derived color. Parent style inside the helper is
asserted with `DCHECK`; the model treats the caller's parent-style guard as an
input and does not turn the assertion into a runtime branch.

The typed λΠ function and C++ transcription agree over all 32,768 assignments
to three caller guards and twelve helper conditions. A true result represents
eligibility to clone and update the old style with independent inherited
properties. A false result does not alone prove resolver entry: `RecalcOwnStyle`
separately checks for a parent style or the document-root/layout-view case
before calling `StyleForLayoutObject`. The generated set of copied properties,
the builder's actual record transformation, the `DCHECK` contract, and
subsequent style-difference handling remain unmodeled.

`examples/blink_computed_style_difference.cpp` models the ordered result
selection in `ComputedStyle::ComputeDifferenceIgnoringInheritedFirstLineStyle`
(`core/style/computed_style.cc:452-527`). Its 19 abstract predicates represent
the source comparisons, including descendant-affecting scroll/display/container
conditions, non-independent inherited equality, explicit child inheritance,
variable independence and equality, independent inherited equality, and
pseudo/overscroll/display-list cases. It preserves branch order and maps results
to the source severity order in `computed_style.h:377-410`. The C++ and typed λΠ
classifiers agree on all 524,288 Boolean assignments. Those assignments vary
source observations independently; some may not be realizable by a concrete
pair of ComputedStyle values, and source DCHECK consistency conditions are not
encoded.

`examples/blink_diff_affects_scroll_animations.cpp` refines the scroll-animation
input to the difference classifier. It models the six comparisons in
`DiffAffectsScrollAnimations` (`computed_style.cc:284-300`): scroll timeline
name/axis, view timeline name/axis/inset, and timeline scope. The λΠ disjunction
and C++ transcription agree on all 64 combinations of inequality observations.
The example then composes this result into the first-priority branch of
`ComputeDifference` with other branch observations held at a baseline that
reaches the ordinary non-inherited result. Both methods agree on all 64 such
compositions. Values and `base::ValuesEquivalent` semantics remain opaque.

`examples/blink_needs_reattach_layout_tree.cpp` models the ordered predicate in
`ComputedStyle::NeedsReattachLayoutTree` (`core/style/computed_style.cc:312-404`).
The first rule treats identical style pointers as no reattach; otherwise either
missing style requires it. The remaining positive conditions abstract display,
first-letter/content/text-combine changes, scroll-marker relationships,
effective overscroll, custom display-layout names, appearance-dependent
`<progress>` and base appearance behavior, `<legend>` float/out-of-flow state,
text-combine writing-mode support, newly added `::first-line`, overlay, and
list-style-position comparisons. The typed λΠ and C++ predicates agree on all
524,288 assignments to these 19 named observations. Since the observations are
independent, this exhaustive state space includes combinations not reachable
from actual styles/elements; it validates the branch expression over its stated
abstraction, not all concrete style pairs. It does not rebuild layout objects.

The text-combine condition is refined over actual `WritingMode` constructors in
the same example. `LayoutTextCombine::IsSupportedMode` is true only for
`vertical-rl` and `vertical-lr`; horizontal and sideways modes are false. The
reattach clause requires a mode-support change only when the new style has
text-combine enabled, in addition to the separate old/new text-combine toggle
check. Typed λΠ and C++ agree on all 100 combinations of old/new text-combine
presence and five old/new writing modes after composing these conditions into
the complete reattach decision with other triggers held false.

`examples/blink_content_data_equivalence.cpp` refines the `ContentDataEquivalent`
condition in `NeedsReattachLayoutTree`. `ComputedStyle::ContentDataEquivalent`
delegates to `base::ValuesEquivalent` on the `ContentData` head pointers
(`computed_style.h:1633`); the pinned `ContentData::operator==`
(`content_data.h:412`) compares node values in order and requires both lists to
end together. The typed node algebra covers all seven Blink subclasses. Text and
alt-text values, quote variants, `none`, image equality classes, and finite
counter metadata atoms are represented explicitly. It preserves the directed
Counter/AltCounter behavior: a Counter node accepts an AltCounter node when the
five counter fields match, while AltCounter requires an AltCounter peer and
matching generated text. This follows the two `Equals` overrides at
`content_data.h:267-356` and `content_data.cc:149-160`.

The model compares C++ and typed λΠ on 11,449 directed node-payload pairs, then on 34,348
bounded list comparisons and the resulting content-change reattach trigger.
List cases include all singleton pairs, all head pairs with a shared tail, all
tail pairs with a shared head, and length mismatch cases through two nodes;
`NoneContentData` is kept terminal per `SetNext`'s source precondition. These
cases test the recursive list equation over the represented depth, not arbitrary
list length. Strings, StyleImage equality, counter-style values, and TreeScope
pointer identity are finite atoms, not evaluated Blink objects.

`examples/blink_style_image_equality.cpp` refines the StyleImage atom used for
image content. It represents the six subclasses and their pinned equality
branches: fetched/fetched, fetched/pending, generated/generated, pending/any,
image-set/image-set, crossfade/crossfade, mask-source/pending, and
mask-source/mask-source. Pending cases preserve source-directed behavior; for
example, pending/generated may compare through CSSValue equivalence while
generated/pending is false. The executable compares C++ and typed λΠ over all 6×6×2^12 = 147,456
kind/relation assignments, with these cases composed into the image-content
reattach trigger. The 12 field relations are abstract booleans: CSSValue,
resource, URL, resolution, generated-image, and image-set internals are not
evaluated. The payload relation is now included in `blink_content_data_equivalence.cpp`
and composed with an image-content reattach decision. The general linked-list fixtures still include finite image atoms. A second
relation-parameterized recursive path uses `imageNode` constructors and feeds
`nodeEqualWithImage` into `contentListEqualWithImage`. It checks singleton image
lists, image nodes in the head and tail of two-node lists, and two repeated
image-kind nodes across 589,824 kind/relation cases. List inequality routes to
reattachment. The repeated-image fixture exercises recursive delegation twice;
it does not cover arbitrary list length or every mixed image-kind sequence.

`examples/blink_generated_image_equality.cpp` expands the generated-image branch
from three relation booleans into structural values. `ContainerSizes::SizesEqual`
compares optional width and height, `ViewportSize::operator==` compares six
double fields, and `StyleGeneratedImage::IsEqual` also requires equivalent
generator values. The λΠ model represents optional dimensions with `noneSize` or
`someSize(d0|d1)`, viewport dimensions with `d0|d1`, and generator-value
equivalence as an explicit Boolean input. The executable compares 663,552 finite
size/value states, including the generated-image reattach decision. A paint-
specific route also models `CSSPaintValue::Equals`, which compares `GetName()`
and `CustomCSSText()`. It constructs six finite values from simple names
(`alpha`, `beta`) and zero or one serialized argument (`red`, `blue`), then
derives each `paint(...)` string. Thirty-six value pairs compare directly; 144
cases compose paint equality with representative equal/unequal container and
viewport sizes. The model omits arbitrary doubles, NaN or signed-zero behavior,
lazy size lookup effects, general argument serialization, multiple arguments,
and multiple paint arguments. A shared `CSSGradientValue::Equals` model covers
repeat, color interpolation space, hue method, and recursive color-stop list
equality through two stops. It compares 254,016 finite gradient-base pairs using
three representative color spaces, all four hue methods, two repeat values, and
red/blue structural CSSColor values with zero/half offsets. Offsets are finite
CSSValue atoms; null hint components are excluded.

`CSSLinearGradientValue::Equals` is modeled over 144 targeted value pairs across
the deprecated, prefixed, and standard linear types. The deprecated branch
compares four optional coordinates and stops while ignoring common repeat/color
fields, matching the pinned branch. Modern types first compare common base fields,
then handle an angle or compare optional first-position x/y values. Coordinate and
angle payloads are finite CSSValue atoms. This does not model all linear gradient
values or the complete CSSValue equality dispatch.

`CSSRadialGradientValue::Equals` is covered over 121 targeted fixture pairs. The
deprecated branch checks matching gradient type, four nullable position values,
two radii, and stops. The modern branch checks common base equality and center
coordinates, then either explicit horizontal/vertical size values or sizing and
shape identifiers with their defaults. The source only checks the gradient type
when the left value is deprecated; for modern left values, a deprecated right
value can pass the modern checks. A targeted cross-type pair exhibits this
direction-dependent result. Size/position payloads and identifiers use finite
atoms; this is not a full radial gradient CSSValue model.

`CSSConicGradientValue::Equals` is modeled over 64 targeted pairs. It requires
common gradient equality plus nullable x, y, and from-angle equality. Color-stop
values now use the structural `CSSColor` relation (color space, four missing
component flags, and four bounded components) for the red/blue fixtures. This
matches `CSSGradientColorStop::operator==`, which compares color and offset CSS
values independently. Offsets remain finite atoms. Constant-gradient and
color-image equality use the same structural relation, with color-image equality
delegating to the constant-gradient implementation. The fixture domains remain
bounded and the complete CSSValue dispatch remains outside scope.

The executable also connects these subclass relations through a tagged
`CSSImageGeneratorValue` equality and composes that with container and viewport
size equality for `StyleGeneratedImage`. Its C++/λΠ comparison loop spans 6,724
finite cases over 41 represented paint/linear/radial/conic/constant/color-image
values, paired with equal/unequal container and viewport samples. Cross-subclass
pairs reduce to false through the tag dispatch. The inequality is checked through
a generated-image change predicate. These fixtures integrate the represented
subclass models but do not widen their finite CSSValue payload domains.

`examples/blink_image_set_cache.cpp` models `IsCachePending`, `CacheImage`,
and `CachedImage` after a non-pending guard. It distinguishes the shared
single-entry cache from the fetcher-keyed map selected by the effective runtime
mode. Cache updates replace the shared entry or the selected fetcher entry;
subsequent pending observations are composed over both fetchers. The finite
float domain includes 1.0, its next representable float, and 2.0, so the model
captures the strict `abs(a-b) < float epsilon` boundary in `EqualResolutions`.
Fetcher presence/runtime-feature gating is supplied as the effective mode;
weak-key expiration, GC identity, and concurrent mutation are omitted.

`examples/blink_image_set_load_dispatch.cpp` composes the selected-option
observation with the subsequent `StyleImageLoader::ResolveImageSet` branch.
No selection and unsupported CSS value kinds return null. `CSSImageValue`
reaches its cache path with the selected resolution override; `CSSGradientValue`
reaches its generated-image path, whose shown `Load` branch does not consume that
override. The model abstracts resource/image results and does not include the outer
cache call in `Load`; its cache state effects are modeled separately in
`blink_image_set_cache.cpp`.

`examples/blink_image_set_load_cache.cpp` composes the selected option dispatch
in `StyleImageLoader::ResolveImageSet` with the outer `CSSImageSetValue::CacheImage`
call. A missing or unsupported selection yields a `StyleImageSet` whose inner
style image is null; `CSSImageValue` carries the option resolution override,
while the gradient path produces a generated image without that override. The
result is wrapped and stored in shared or fetcher-keyed cache state, replacing
the targeted entry and preserving the other fetcher entry. The C++/typed-λΠ
comparison covers 120 finite transitions. `GetBestOption`, actual resource
loading, generated-image evaluation, pointer identity, and GC are outside this
composition. Sources: `element_style_resources.cc:195-210`,
`css_image_set_value.cc:139-152`, `style_image_set.cc:34-36`.

`examples/blink_image_set_selection_load_cache.cpp` extends that composition
upstream to the first-call `GetBestOption` rules in the same typed rewrite
system. A bridge maps the selected option's finite image identity to an explicit
CSS value-kind observation, then preserves option resolution through CSS image
loading, wraps the nullable result in `StyleImageSet`, and updates the shared or
fetcher-keyed cache. Its C++ comparison harness contains 96 bounded transitions,
including a selected unsupported value and a list with no supported options.
The harness was compiled but not executed during this change. Resolution is an
integer fixture domain; lazy `options_` cache mutation, actual resource loading,
generated-image evaluation, preexisting nonempty cache values, GC/pointer
identity, and arbitrary MIME/floating-point values remain abstract. The
`imageValueKind` mapping is a declared observation for fixture images; MIME
support controls whether an option survives selection but does not determine its
CSS value subclass. Sources: `css_image_set_value.cc:49-108`,
`element_style_resources.cc:195-210`, `css_image_set_value.cc:139-152`,
`style_image_set.cc:34-36`.

`examples/blink_image_set_options_cache.cpp` adds the first-call cache state
transition from `CSSImageSetValue::GetBestOption`. An uninitialized cache is
filled with the supported, sorted, deduplicated options; if none survive, the
model stores a dedicated no-supported-options sentinel corresponding to Blink's
single null `options_` entry. Later calls select from the cached list without
re-filtering under new MIME feature observations. The included two-call C++/λΠ
comparison harness covers 128 finite inputs, but it was not executed in this
change. It assumes stable computed resolutions and omits concurrent calls,
pointer identity, and other mutable resolver behavior. Sources:
`css_image_set_value.cc:49-108`, `css_image_set_option_value.cc:38-45`.

`examples/blink_image_set_computed_resolution.cpp` models the dispatch in
`CSSPrimitiveValue::ComputeDotsPerPixel`: calculated values delegate to
`CSSMathFunctionValue::ComputeDotsPerPixel(length_resolver)`, while numeric
literals use their stored value multiplied by the canonical-unit scale; either
result then passes through `CSSValueClampingUtils::ClampDouble`. The model
isolates this boundary with literal unit metadata and a calculated-expression
observation. It does not evaluate CSS math expression trees or establish
resolver independence for every supported expression. The finite harness is
compile-checked only. Sources: `css_image_set_option_value.cc:33-36`,
`css_primitive_value.cc:310-317`, `css_math_function_value.cc:74-78`,
`css_numeric_literal_value.cc:113-116`.

`examples/blink_image_set_failed_subresources.cpp` models the cache lookup and
status decision in `HasFailedOrCanceledSubresources`. The effective cache mode
selects either the shared entry or the requested fetcher entry; null fetcher or
disabled identity checking uses the shared entry. A missing cached style image
returns false, while an image with no resource content or failed/canceled
content returns true. Loaded content returns false. The source does not check
device scale in this method, and the model leaves fetcher-key identity, resource
loading, and image/content internals as finite observations.

`examples/blink_image_set_resolve_values.cpp` models
`ResolveValuesAndCreateCopyIfNeeded` followed by `ResolveValuesIfNeeded`. For
each option, the abstract `ResolveGradients` observation either returns the
same image reference (preserving the original option) or a different reference
(creating an option with the new image and the prior resolution/type fields).
The list order is preserved. If no references change, the caller gets the
original CSSImageSetValue; otherwise it gets the rebuilt list. Seven finite
lists cover unchanged, mixed, and changed outcomes, including a replacement
with the same finite image ID to distinguish reference change from value ID.
Resolver internals and GC pointer identity are outside the model.

`examples/blink_image_set_type_support.cpp` refines the option support
predicate. `CSSImageSetOptionValue::IsSupported` accepts a missing type or a
supported `CSSImageSetTypeValue`, and also requires positive computed
resolution. The finite MIME relation lowercases representative ASCII inputs,
checks the pinned image MIME set, and models conditional AVIF and JPEG XL
support. Twenty option inputs cover all twelve static supported MIME types,
case variants, and feature-gated types under eight feature-flag combinations
(160 states). The MIME relation is also used directly in the separate
`GetBestOption` list reduction. That composed selection model covers 21 finite
lists at four scales and all eight feature configurations (672 states).

`examples/blink_image_set_selection.cpp` models the first `GetBestOption`
computation for an initially empty `options_` cache. It filters by MIME support
and positive computed resolution, stable-sorts by resolution, removes duplicate
resolutions while retaining the first option, then chooses the first option at
or above the device scale, falling back to the final (highest-resolution)
option. The reduction model compares 21 bounded lists at four scale values and all
eight AVIF/JXL build/runtime configurations. It computes support from optional
the full pinned static supported-image MIME set with ASCII case variants and
feature-gated AVIF/JXL entries; computed-resolution values remain finite. Cache mutation and later
resource loading remain outside this example.

`examples/blink_image_set_value_equality.cpp` refines the image-set arm of
`StyleImage::IsEqual`. `StyleImageSet::IsEqual` checks the right-hand subclass,
then `ValuesEquivalent` for the best-fit image, then `CSSImageSetValue::Equals`.
The latter inherits `CSSValueList::Equals`, which compares the comma separator
and ordered option list; each `CSSImageSetOptionValue::Equals` compares image,
resolution, and optional type. The λΠ model covers six finite option lists and
composes option-list equality with the subclass and best-fit conditions. Nested
CSSValue relations and best-fit image equality are finite atoms or explicit
inputs, so this is not general CSS image-set equivalence.

`examples/blink_crossfade_value_equality.cpp` models
`CSSCrossfadeValue::Equals` as length equality followed by ordered pairwise
`ValuesEquivalent` checks for each image and percentage. The finite model uses
five values with list lengths zero through two and composes the relation with
`StyleCrossfadeImage::IsEqual`, which first checks that the right operand is a
crossfade image. This abstracts nested CSSValue equality to finite atoms and
omits construction, resolution, weight calculation, and image rendering.

`examples/blink_css_color_equality.cpp` refines the CSSColor atom relation to
Blink's value fields. `CSSColor::Equals` delegates to `Color::operator==`, which
compares color space, four component-presence flags, and four stored component
values. The executable compares 262,144 ordered pairs using two color spaces,
all four-flag assignments, and exact component values in `{0,1}`. This is a
model relation: it is also used by constant-gradient, color-image, and
gradient-stop color comparisons in the generated-image model. Gradient-stop
fixtures still use only the bounded red/blue subset. Other color spaces, arbitrary float values, and parsing or color
construction remain outside scope.

The scroll-marker-group comparison is also refined. `ComputedStyle::ScrollMarkerGroupEqual`
uses value equivalence for the optional group, and `ScrollMarkerGroup::operator==`
compares position and mode (`computed_style.h:2578`, `scroll_marker_group.h:25`).
The model represents absence plus the four `{after,before} × {links,tabs}` values.
The C++ structural comparison and typed λΠ agree on all 25 old/new pairs; the
resulting inequality is then supplied to the `NeedsReattachLayoutTree` decision
with all other conditions false. Distinct heap pointers with equal immutable
position/mode fields map to the same value constructor, matching value rather
than allocation-identity comparison.

The same example models the outer `ComputeDifference` cases (`computed_style.cc:421-450`):
pointer identity yields equal, a missing style yields inherited, and otherwise
the result is the maximum of the ordinary and optional cached inherited
first-line difference. Typed λΠ and C++ agree on 576 finite combinations. The
cached pseudo-style lookup and recursive style comparison are represented by
input observations rather than evaluated. Generated field comparisons and
style-recalc side effects remain outside the model.

The example also models `Element::ApplyComputedStyleDiff`
(`core/dom/element.cc:5808-5823`). An existing descendant-recalc level, or a
difference below `kPseudoElementStyle`, leaves propagation unchanged.
Otherwise, descendant-affecting, inherited, independent-inherited, and
pseudo-element differences ensure the corresponding propagation level. The
C++ and λΠ rules agree for all 30 pairs of five propagation levels and six
difference severities. This models only the propagation field; query flags and
later style/DOM side effects remain outside this helper model.

The same example covers two adjacent `RecalcOwnStyle` layout decisions. The
`ComputedStyle::NeedsReinsertLayoutTree` predicate (`computed_style.cc:406-419`)
requests reinsertion when out-of-flow status changes, or when float status
changes while the new style is not out of flow. The C++/λΠ comparison covers all
16 old/new position/float combinations. The model also captures selection of
`LayoutObject::ApplyStyleChanges` (`element.cc:6218-6268`): difference severity
sets the initial value, HTML root style forces application, an anchor evaluator
overrides to no application, and anchor functions or a non-normal first-letter
initial-letter force application when no evaluator is active. The comparison
covers all 32 Boolean combinations. These are decision models only; the
subsequent layout-tree remove/reinsert, `SetStyle`, and layout invalidation
operations are not executed or modeled.

Typed λΠ agrees with the C++ transcription on 640 states each for
`TraverseChildren` and `TraversePseudoElements`, all eight mark-reattach
boolean combinations, all 64 inputs to the `TraverseChild` disjunction, and
17,920 pseudo-element cases, 1,280 `IndependentInherit` cases, and 320
representative compositions with the previously modeled `ShouldRecalcStyleFor`
gate. This is still a predicate model: it does not
traverse a DOM/flat tree, invoke `Element::RecalcStyle`, model display locks,
or schedule style recalc from layout or container-query evaluation.

## Paint chunk grouping

`paint/paint_chunker.cc:30-89` updates current properties and creates a new
chunk when a boundary is forced or the property state changes. Its special-item
path at lines 91-149 forces boundaries on both sides of foreign-layer and
scrollbar display items. `examples/blink_paint_chunker_bisimulation.cpp`
models those grouping decisions and assigned chunk IDs. Eight finite item
sequences agree between the C++ transcription and typed λΠ. It omits actual
display-item creation, property payloads, cache behavior, and `PaintArtifact`
lifecycle; finite comparisons do not establish general equivalence.

## CascadePriority ordering fields

`examples/blink_css_cascade_priority.cpp` models the documented semantic
priority fields from pinned Chromium commit `99d77c2af39f60ee06a5516ad5428e4d2aa61da4`
and compares the λΠ lexicographic observer against a separate C++ transcription
of the packed words. The source is
`core/css/resolver/cascade_priority.h`: origin/importance encoding is
`origin ^ 0xF` for important values (lines 21–27); tree and layer order are
XORed with `0xFFFF` when important (lines 33–50). `CascadeOrigin` enumerators
and their important ordering rationale are in `cascade_origin.h:15-35`.

The model follows the actual bit significance. `CascadePriority::operator<`
compares `high_bits_` first and `low_bits_` second (`cascade_priority.h:208-217`).
The high word stores tree order in bits 0–15 and origin/importance at offset 16
(masks and offsets at lines 82–100; packing at 145–156). In the low word, the
most-significant-to-least-significant order is try-tactics at bit 51, try-style
at 50, inline-style at 49, 16-bit layer order at offset 33, 16-bit rule index
at offset 17, 16-bit declaration index at offset 1, and `already_applied` at
bit 0. This means the low-word flag order for comparison is try-tactics, try,
inline; `examples` uses this order even though issue shorthand listed the flag
names in a different sequence.

The typed λΠ key compares the nine encoded fields lexicographically. The
`encodeOrigin` rewrite table includes all origin enum values in the pinned
`CascadeOrigin`; the 18 comparison fixtures exercise a subset of those origins
alongside a closed selected set of 16-bit field values `{0, 1, 0xFFFE, 0xFFFF}`,
the low-word style flags, and importance on/off. They compare the semantic
observer against packed `uint32_t high_bits` and `uint64_t low_bits`. The
selected field set permits explicit layer/tree/rule/declaration boundary checks
without claiming exhaustive 16-bit arithmetic or exhaustive origin-pair
coverage.

This models the ordering representation only. It does not model
`ElementRuleCollector` sorting or matching, how rule indices refer to matched
properties, `CascadeMap` construction, revert lookups, CSS value resolution,
property application, or any browser rendering behavior. Agreement on these
finite pairs is not proof of the full CSS cascade.

## CascadeMap list composition

`examples/blink_css_cascade_map.cpp` composes selected `CascadePriority`
fields with the per-property priority-list behavior in pinned Chromium commit
`99d77c2af39f60ee06a5516ad5428e4d2aa61da4`. The C++ reference packs variable
layer, rule-index, declaration-index, and already-applied fields at their
source offsets, with origin/importance, tree order, inline style, try style,
and try-tactics fixed. Its packed low word is compared with the typed λΠ
lexicographic priority projection. For this fixed prefix, `ForLayerComparison`
is represented by the natural-number layer field.

The source insertion path is `core/css/resolver/cascade_map.cc:208-228`:
`CascadeMap::Add` checks the layer-comparison precondition, inserts equal or
weaker priorities using `InsertKeepingSorted`, and pushes a strictly stronger
priority. `cascade_map.h:284-325` implements head insertion in `Push` and
sorted traversal in `InsertKeepingSorted`; because its break condition is
`priority >= curr_node.priority`, a new equal-priority node is placed before
the older tie. `cascade_map.cc:78-103` implements `FindRevertLayer` by scanning
this strongest-first list and returning the first entry whose
`ForLayerComparison()` is strictly below the reverting key.

The model checks every prefix of two insertion sequences (11 configurations)
against the final ordered list from the C++ packed-key insertion. Cases include
a weaker same-layer priority inserted behind a stronger one, a stronger head
push, duplicate full priorities inserted before prior ties, and same-layer
priorities distinguished by rule index. For each prefix, nine reverting-layer
bounds are compared between λΠ and the C++ first-eligible scan (99 outcomes).
The sequences satisfy `Add`'s nondecreasing layer-comparison DCHECK. The
existing `blink_css_cascade_priority.cpp` covers the ordering fields more
broadly, while `blink_css_revert_layer.cpp` covers first-eligible search over
preordered lists; this example composes selected field packing, list
construction, and lookup.

This is a finite projection, not a translation of `CascadeMap`. It fixes the
high-word and style-flag prefix and abstracts a property to one list. It omits
native/custom property storage, `important_set_`, inline/presentation-hint
loss flags, list-node backing storage, arbitrary 16-bit field arithmetic,
actual `CascadePriority::ForLayerComparison` packing, `StyleCascade` resolution,
CSS parsing/application, and `ElementRuleCollector` order production.

## Revert-layer to computed color

`examples/blink_css_revert_color.cpp` composes selected branches from the
pinned Chromium snapshot (`99d77c2af39f60ee06a5516ad5428e4d2aa61da4`).
`core/css/resolver/cascade_map.cc:78-103` scans the per-property list and
returns the first `ForLayerComparison() < revert_from` entry.
`core/css/resolver/style_cascade.cc:1593-1601` passes the current priority's
layer-comparison key to that search. `ResolveRevertTo` at `style_cascade.cc`
`1613-1630` returns CSS `unset` if no candidate exists, the candidate has no
origin, or its full priority is not weaker than the reverting declaration;
otherwise it resolves the selected declaration's value. The model represents
the no-candidate and not-weaker branches explicitly, and models values from a
finite set of explicit colors and CSS-wide `initial`, `inherit`, and `unset`
keywords.

The value then passes through the `color` defaulting decision. The property's
metadata at `core/css/css_properties.json5:1398-1419` marks `color` inherited
and sets its default to `StyleColor(Color::kBlack)`. `style_builder.cc:104-132`
maps `inherit`, `initial`, and `unset` to inheritance or initial application,
and rewrites root-level `inherit`/`unset` to initial when no parent style
exists. `style_resolver_state.cc:164-167` defines `IsInheritedForUnset`;
the ordinary inherited `color` case therefore takes the parent value for
`unset` when present.

The typed λΠ composition and a separately written C++ transcription agree
on twelve finite cases: eligible lower-layer explicit values; equal/higher
layer skips; no candidate with parent and at root; same-layer eligible ties
where the first list entry wins; explicit initial/inherit/unset values with
parent and root; and a selected candidate rejected by the not-weaker guard.
Initial black is encoded as color ID zero. The model assumes any selected
candidate has a valid origin; it represents no candidate and not-weaker
fallbacks, but does not encode the `!p->HasOrigin()` test separately. Layer
and full-priority keys are abstract ordered projections, so this is a semantic
composition over selected results, not the packed 96-bit `CascadePriority`
calculation.

The model omits declaration collection and insertion, parsed CSS values,
animations, origins/tree-scope ordering, recursive substitution, generated
`ApplyInitial`/`ApplyInherit`, real `ComputedStyle` color representation,
highlight and pseudo-element rules, dependent-property invalidation, and all
rendering. It does not establish full `StyleCascade` or color computation
equivalence.

## Revert-origin selection through inherited color defaulting

`examples/blink_css_revert_origin_color.cpp` composes the projection in
`examples/blink_css_revert_origin.cpp` with the CSS-wide defaulting rules for
the inherited `color` longhand. In the pinned Chromium source,
`core/css/resolver/style_cascade.cc:180-193` defines
`TargetOriginForRevert`; `:1567-1591` searches the target origin;
`cascade_map.cc:42-71` selects the first eligible candidate from the
strongest-first property list; and `style_cascade.cc:1613-1630` applies the
no-candidate/no-origin/not-weaker `unset` fallback. `longhands/longhands_custom.cc`
`:2536-2562` gives `color` initial and inherited application behavior, while
`style_builder.cc:104-132` maps CSS-wide keywords and root inheritance to
initial defaulting. The metadata at `core/css/css_properties.json5:1398-1419`
marks `color` inherited and supplies a color-scheme-dependent initial black
value; this model fixes that initial result to abstract ID zero.

The C++ transcription and typed λΠ composition agree on twelve finite cases
covering target-origin skips, explicit values, initial/inherit/unset, missing
candidate fallback, parent/root outcomes, and equal/stronger priority fallback.
The priority projection varies only encoded origin/importance and rule index;
other `CascadePriority` fields are fixed. Color values are IDs. Color-scheme
selection, `currentColor`, visited/animated color behavior, actual style
builder flags, tree scopes, declaration collection, and rendering are outside
the model; the result is not full `StyleCascade` equivalence.

## CSS `revert` origin selection

`examples/blink_css_revert_origin.cpp` models the origin scan for the CSS-wide
`revert` keyword, distinct from `revert-layer`. In the pinned source,
`style_cascade.cc:180-193` maps UA to `kNone`, user to UA, and
author-presentational-hint/author/animation to user; `kNone` and transition
are marked unreachable. `StyleCascade::ResolveRevert` at
`style_cascade.cc:1567-1591` passes that target to `CascadeMap::Find`.
`cascade_map.cc:42-71` scans the strongest-first property list and returns the
first candidate whose origin is no higher than the target origin.

The model then follows `ResolveRevertTo` (`style_cascade.cc:1613-1630`): no
candidate, a candidate with no origin, or a candidate whose priority is equal
to or stronger than the reverting declaration produces `unset`; a weaker
candidate returns its origin and a finite explicit value. Its comparator
uses `EncodeOriginImportance` from `cascade_priority.h` and rule index. Tree
order, inline/try flags, layers, declaration index, and already-applied are
fixed to equal values. The candidate list is sorted by this projected key in
the C++ reference, then the λΠ scan chooses by target origin.

The C++ transcription and typed λΠ model agree on eleven finite checks:
target-origin selection for UA, user, hint, author and animation; absent and
no-origin candidates; a weaker candidate; and equal/stronger fallback in the
`ResolveRevertTo` branch. These cases exercise the origin projection, not
complete `CascadePriority` packing or declaration collection. CSS-wide value
recursion, tree-scope rules, property application, and rendering remain
outside the model.

## Revert-origin color with full priority ordering

`examples/blink_css_revert_origin_full_priority.cpp` extends that composition
using the pinned `core/css/resolver/cascade_priority.h` (there is no
`cascade_priority.cc` in this checkout). `EncodeOriginImportance` is at
`:21-26`, `EncodeTreeOrder` at `:33-38`, and `EncodeLayerOrder` at `:44-50`.
The class comment and constructor at `:54-76` and `:134-155` define the
significance order and packed fields. `operator<`/`operator>=` at `:208-218`
compare `high_bits_` first and `low_bits_` second. The semantic λΠ key follows
that order: encoded origin/importance, encoded tree order, try-tactics, try,
inline, encoded layer order, rule index, declaration index, already-applied.
The C++ transcription packs the same values according to the constructor and
compares the resulting high/low words. Candidate lists in the C++ observer are
sorted by that packed comparator; λΠ performs the first origin-eligible scan
and a strict weaker-than comparison against the reverting priority.

The resulting CSS value then follows the inherited-color defaulting slice
above. Eighteen finite composed cases agree with C++, and twelve direct
priority pairs agree at field precedence, important tree/layer reversals,
flags, index boundaries and equality. Sixteen-bit fields are represented by a
finite set of constructors, ranked into a small Peano domain for comparison;
this avoids treating a finite boundary value such as `0xffff` as a 65,535-step
Peano numeral. The λΠ model is a semantic tuple comparison, not 96-bit packing,
and does not establish behavior for arbitrary field values. CSS color-scheme,
`currentColor`, visited/animated color behavior, declaration collection,
tree-scope semantics, and renderer lifecycle remain outside the model.

## Typed tokenizer start tag through p-start DOM insertion

`examples/blink_tokenized_p_start_dom.cpp` adds a bounded typed tokenizer bridge from raw `<p ...>` source to `StartTagToken(pTag, Attrs, HasEntity)`, then supplies that token to p-scope pop and DOM insertion. Twenty-eight inputs cover ordered attributes, full-table named attribute references with guard behavior, selected numeric-reference spellings plus semicolon-terminated decimal/hex forms for all 32 values in the U+0080-U+009F adjustment interval, duplicate attributes, body text, and downstream DOM/CSS/render projections. Generated hexadecimal forms include both `x`/`X` markers and hex digit case variants. The C++ scanner independently emits the token and final DOM/stack projection; the comparison checks the token, its entity flag, and insertion state. Source mapping: `html_tokenizer.cc:852-1044` scans attribute names/values and tag end; `html_token.h:292-310,358-359,412` accumulates token attributes and exposes the entity flag; `html_tokenizer.cc:1061` sets the flag after successful attribute reference decoding; `html_tree_builder.cc:875-881` performs the p-start fake-end-tag check then insertion; `html_construction_site.cc:986-988,1240-1248,1458-1462` inserts, creates, and attaches attributes. This is a bounded transcription mapped to Blink branches, not execution of Blink's tokenizer. Attribute names remain ASCII Text, while attribute values use UTF-16 code units. Named references use the shared search/consume relation with the attribute additional-character guard; `NotEqualTilde;` and `Afr;` attribute fixtures preserve their multi-code-point and surrogate-pair UTF-16 outputs. Eleven earlier numeric spellings and the adjustment-interval decimal/hex forms are table-derived rewrite rules, not general numeric accumulation; optional numeric semicolons are not modeled by this exact-spelling bridge. Successful rewrites set the modeled token flag. Duplicate attributes retain the first normalized name in the DOM projection. The C++ target compiles; the evaluator and finite observer comparisons were not run for this update.

The same example continues from the normalized output of `sem(caseId)` into a fixed CSS and rendering slice. It extracts the inserted `p` node's tag and ordered attributes, collects declarations matching `p`, `.beta`, `#alpha`, `.gamma`, `.omega`, `.delta`, `.epsilon`, `.zeta`, and `.eta`, compares simple-selector specificity as the tuple (ID count, class count, tag count), then uses source order for equal-specificity declarations. It resolves the winner's `initial`/`inherit`/explicit color against an abstract parent color. The λΠ selector constructors and matcher are value-parameterized for simple tag, ID, and class selectors. Tag matching compares tag constructors; ID matching compares the complete UTF-16 attribute value to the selector value; class matching tokenizes the UTF-16 class value using HTML ASCII whitespace and compares each token with the selector value. The tuple order corresponds to `CSSSelector::Specificity` at `css_selector.cc:210-234`, with weights and masks at `css_selector.h:173-180`; class tokenization follows `EasySelectorChecker::MatchesClassName` in `selector_checker-inl.h:203-215` and `SpaceSplitString::Data::CreateVector` in `core/dom/space_split_string.cc:37-70`. For equal specificity, declarations carry a nine-field normalized `CascadePriority` key and λΠ compares fields in Blink’s packed significance order: encoded origin/importance, encoded tree order, try-tactics, try-style, inline-style, encoded layer order, rule index, declaration index, and already-applied. The field comparator is generic over the represented naturals and Booleans. The composed C++/λΠ declaration fixtures vary rule indices, normal tree-order ranks, and include normal-origin ranks (author 4 versus user 2) and important-origin ranks (user 13 versus author 11). The author-normal rule wins against a later user-normal rule; the user-important rule wins against an earlier author-important rule. These fixtures use already-normalized origin/importance and tree-order ranks; they do not execute `EncodeOriginImportance` or `EncodeTreeOrder`, vary layer/flag fields, declaration indices, or already-applied state in the composed path. The separate `blink_css_cascade_priority.cpp` model compares the normalized key with Blink’s packed high/low words over selected precedence/boundary pairs. These models do not cover full 16-bit arithmetic, rule collection, priority construction for arbitrary declarations, CSS value resolution, or full `StyleCascade`. The declaration set remains the concrete `p`/`.beta`/`#alpha`/`.gamma`/`.omega`/`.delta`/`.epsilon`/`.zeta`/`.eta` example; a `class="beta gamma"` fixture makes `.beta` and `.gamma` compete at equal specificity and checks that the greater rule index wins after declaration-list reversal. A `class="gamma omega"` fixture checks that normal author origin outranks normal user origin despite a lower rule index. A `class="delta epsilon"` fixture checks user-important encoded rank 13 outranks author-important rank 11 despite a lower rule index. A `class="zeta eta"` fixture checks tree-order rank 2 outranks rank 1 before rule index. These winner checks are repeated after declaration reversal. The integrated λΠ path applies a recursive `preprocessInputText` relation before token scanning, body decoding, and end-tag recognition; it maps CR to LF and consumes an immediately following LF. The C++ reference performs the same complete-string CR/CRLF normalization. A new fixture carries CRLF through both a quoted class value and body text. The same raw fixture also checks NUL state behavior: attribute-value scanning emits U+FFFD, while Data-state body decoding skips NUL. The independent complete-stream preprocessor model supplies the generic `skipNull` rule and terminal marker; this bridge specializes NUL handling according to tokenizer state. The tokenizer whitespace predicate then operates on normalized input. A vertical-tab class-value fixture checks that VT is retained within the unquoted value. A `class="beta&nbsp;gamma"` fixture checks that the decoded U+00A0 remains inside one class token and does not match either `.beta` or `.gamma`. Stylesheet parsing, combinators, and pseudo-classes remain outside this pipeline.

Body text before the next `<` is modeled as one Data-state CharacterToken. Named references use the same full 2,231-entry table-backed λΠ search/consume and `ProcessEntity` rules as `blink_html_entity_search_bisimulation.cpp`, extracted into `examples/blink_html_entity_search_model.hpp`. Decoded UTF-16 code units flow through CharacterToken, pending text sourced from `tokenData(inputCharacters(...))`, the Text child, and the model's abstract `paintText` projection. Eleven earlier exact decimal/hex spellings and semicolon-terminated, table-derived decimal/hex forms for all 32 Windows-1252 adjustment values are included; arbitrary numeric accumulation and optional numeric semicolons remain outside the adapter. The C++ transcription derives named output from the pinned CSV with longest matching name; the λΠ branch uses the shared relation. The harness currently compares the resulting outputs for twenty-eight fixture inputs; compilation has passed, but the observer harness was not run for the latest shared-builder integration.

Source correspondence for body tokenization is `html_tokenizer.cc:278-293` (Data state), `:173-189` (`ProcessEntity`), and `:1799-1846` (`EmitData`). Numeric conversion behavior is grounded in `html_entity_parser.cc:135-141,155-225`; this end-to-end path includes eleven earlier numeric spellings and table-derived semicolon-terminated decimal/hex forms for all 32 U+0080-U+009F values, while `blink_numeric_html_entity_bisimulation.cpp` separately models finite digit scanning and selected code-point correction. Both numeric C++ transcriptions use the source-derived table in `blink_html_numeric_entity_table.hpp`; the standalone numeric model's λΠ table remains independent. Character insertion maps to `html_tree_builder.cc:520-575,2690-2872` and `html_construction_site.cc:1078-1118`; flush behavior is at `html_construction_site.cc:245-271`. The text child list uses abstract IDs and `nodeText` concatenates its data for style/layout/paint projection. The restricted p end-tag branch is `html_tree_builder.cc:2217-2235,2296-2308`. `text_fragment_painter.cc:329-355` paints visible inline text fragments. The `computed_style.cc:800-803` color-field flag and `layout_object.cc:3133-3156` conditional simple-paint invalidation motivate the boolean projection; non-empty data stands in for a text layout with inline fragments.

Limits: the integrated scanner consumes complete bounded fixture strings and stops at the next `<`; it does not continue across streaming chunks or model the closed-stream terminal-NUL marker and segmented preprocessing state; NUL behavior is limited to the represented attribute-value and Data states. It does not parse general numeric references, cover other tokenizer states, parse general body markup, model insertion-location changes, or use actual Blink Node pointers. The standalone reusable relation represents need-more outcomes, but this DOM adapter does not model continuation. The UTF-16 sequence is a semantic abstraction; Blink's WTF `String` may use 8-bit or 16-bit backing. Attribute values preserve UTF-16 units. The typed matcher handles only simple tag, ID, and class selectors, and the composed declaration set is fixed. The model does not cover foreign-content namespaces or duplicate-attribute diagnostics/use counters. Broader selector syntax, full `CascadePriority`/`StyleCascade`, color scheme, `currentColor`, style-tree lifecycle, glyph shaping/geometry, visibility, display-item identity, and Blink paint artifact internals remain outside the model.

## Pending text accumulation and flush

`examples/blink_pending_text_bisimulation.cpp` extracts the accumulator and
flush transition used by `HTMLConstructionSite::InsertTextNode`. The typed
state tracks parent and next-child identity, accumulated text, and whitespace
mode. An append at the same location concatenates the chunk and keeps the
lower `WhitespaceMode` by the source's `std::min` operation. A different
location flushes the old buffer first, then starts a new buffer.
`FlushPendingText` discards empty text or an invalid insertion location;
otherwise it appends to the previous Text sibling if that sibling is Text, or
inserts a new Text node. Seven finite C++/λΠ cases check these transitions.

The source boundary is `core/html/parser/html_construction_site.h:264-303`
(`PendingText::Append`/`Discard`) and
`core/html/parser/html_construction_site.cc:245-271,1081-1118`
(`FlushPendingText`/`InsertTextNode`). Chunks are opaque finite atoms and
location/node identities are small natural numbers. String canonicalization,
template/foster insertion adjustment, actual DOM mutation and task ordering
remain outside the model. A restricted raw-input path is now connected through
two pending-text flushes and models a paragraph Text-child list with abstract
IDs. The subsequent style/layout/paint projection reads the concatenated child
text through `nodeText`.

`examples/blink_dom_text_flush_bisimulation.cpp` models a stronger mutation
observation at the same source boundary. Its `Children` list distinguishes
element children from Text children with abstract IDs. At a null `next_child`,
valid non-empty text appends to the final Text child while keeping that child's
ID, or inserts a fresh Text child after a non-Text tail and advances the next
ID. Empty and invalid flushes preserve the list and next ID. Seven finite
C++/typed-λΠ cases cover these outcomes, including coalescing after an element
child. This is a child-list/identity projection for `FlushPendingText`, not
Blink pointer identity or full DOM mutation. It fixes the adjusted insertion
location to one parent at append-to-end and omits templates, foster parenting,
String canonicalization, and task scheduling.

## HTML input-stream preprocessing

`examples/blink_input_stream_preprocessor.cpp` defines a typed λΠ transducer
for complete finite code-unit streams. Its ordinary-code-unit rewrite is
parametric in an opaque input-unit term. It maps CR to LF, consumes an
immediately following LF, and represents a closed stream's final NUL as an EOF
marker. NUL handling is parameterized by `HTMLTokenizer::State` and the
force-replacement flag. The policy relation encodes all 76 states from the
pinned enum with both flag values (152 valuations): only Data, RCDATA, and
RAWTEXT skip NUL when force-replacement is false, matching
`ShouldSkipNullCharacters`. Seven finite stream fixtures select policies by
state and cover CR, CRLF, NUL replacement/skipping, and the terminal marker.
The target compiles, but observer comparisons were not run. Source:
`html_tokenizer.h:57-134,192` and `input_stream_preprocessor.h:45-111,115-167`.

This abstraction does not model `SegmentedString` cursor movement, Peek versus
Advance, line-number updates, open-stream suspension, or the precise boolean
return values at exhaustion. It describes the normalized finite sequence only. The integrated raw-tokenizer bridge composes CR/CRLF normalization and specializes NUL handling for the represented attribute-value and Data states; it does not reuse this complete-stream relation.

## Input-stream preprocessor cursor operations

`examples/blink_input_stream_preprocessor_cursor.cpp` models `Peek` and
`Advance` as λΠ transitions over a finite remaining input list, the
`skip_next_new_line_` bit, tokenizer NUL policy, closed-stream flag, and line
counter. The result records whether a character is available, the exposed
character, remaining cursor, updated suppression bit, and line count. CR is
exposed as LF without consuming it; after that peek, `Advance` consumes CR and
suppresses a following LF while updating the line number. The relation also
represents NUL replacement/skipping and the closed final NUL observation. Nine
finite C++/λΠ observations are encoded, but the executable comparisons were
not run. The target compiles. Source correspondence: pinned
`input_stream_preprocessor.h:58-66,115-167` and
`platform/text/segmented_string.h:241-312`.

The model flattens `SegmentedString` segments into one list, so it does not
represent segment identities, per-segment line-number exclusion, append/prepend,
`AdvancePastNonNewline`, `AdvancePastCarriageReturn`, or cursor metadata such as
global consumed offsets. Those operations remain separate work.
