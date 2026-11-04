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

## Style recalculation gate

`examples/blink_style_recalc_gate.cpp` models selected branches of
`StyleRecalcChange::ShouldRecalcStyleFor` and `ForChildren` from pinned
Chromium commit `99d77c2af39f60ee06a5516ad5428e4d2aa61da4`. In
`core/css/style_recalc_change.cc:59-70`, `ShouldRecalcStyleFor` checks
`kSuppressRecalc` first (false), then `RecalcChildren()` (true), then
`Node::NeedsStyleRecalc()`, then `RecalcFlagDependent(node)`. In
`style_recalc_change.h:198-199`, `RecalcChildren` means propagation greater
than `kUpdatePseudoElements`, so `kRecalcChildren` and `kRecalcDescendants`
both force this node predicate to true. The model preserves the distinction
when propagating to children: `ForChildren` at `style_recalc_change.h:116-119`
retains `kRecalcDescendants`, while other propagation values become `kNo`.

For the flag-dependent branch, the model includes only
`kRecalcStyleContainerChildren` (`style_recalc_change.h:33-38`): for an
Element with an existing computed style, `RecalcFlagDependent` checks
`DependsOnStyleContainerQueries()` (`style_recalc_change.cc:30-57`). A Text
node is the non-Element category and returns false at the `DynamicTo<Element>`
guard. The two modeled flags are `kSuppressRecalc` (documented at
`style_recalc_change.h:59-61` as suppressing the current node and lost for
children) and the style-container-children flag. `FlagsForChildren` removes
`kRecalcStyleContainerChildren` at `style_recalc_change.cc:156-163` and clears
`kSuppressRecalc` for children at lines 178-188. Thus the finite projection
models suppression, child versus descendant propagation, an already-dirty
node, and an Element-only style-query dependency.

The typed λΠ decision and an independently written C++ transcription agree on
160 combinations (five propagation levels, four modeled flag sets, Element
or Text, dirty/clean, dependency true/false). The separate `ForChildren`
projection agrees on 20 propagation/flag combinations. This is not full style
recalculation equivalence: other container-query, hover, content-visibility,
reattach and mark-reattach flags are omitted; nodes with no old computed style
are omitted; `ShouldUpdatePseudoElement`, `TraverseChild`, child dirty flags,
layout-subtree update flags, and `StyleEngine`/`Element` traversal are not
modeled. At `ShouldRecalcStyleFor`, the child and descendant propagation
levels intentionally yield the same true result; only `ForChildren` carries
the descendant distinction onward.

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
