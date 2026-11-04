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
* `examples/blink_html_fastpath_scan_escaped_text.cpp` models the slow text
  scanner with character-reference results supplied as observations. It handles
  literal/reference output, CR normalization, `<` handoff, and failure results
  across 12 finite sequences; entity internals and source offsets are omitted.
* `examples/blink_html_fastpath_scan_text.cpp` models the recursive plain-text
  scan and handoff to escaped scanning, preserving the `<` cursor boundary,
  8-bit flag, and null-character failure. Eleven finite sequences compare
  with a C++ transcription; escaped scanning and SIMD are outside scope.
* `examples/blink_html_fastpath_children_step.cpp` models one iteration of
  the fast parser's `ParseChildren` loop, including text append order, EOF and
  close-tag exits, depth-limit failure, child failure, and child append before
  continuation. It compares 64 finite control combinations; payloads and DOM
  mutations are abstracted.
* `examples/blink_html_fastpath_close_tag.cpp` models container closing-tag
  result selection, including first-error preservation, short-name rejection,
  exact/ASCII-case name match, and required `>` after whitespace. Thirty-six
  finite branch states compare with a C++ transcription.
* `examples/blink_html_fastpath_child_dispatch.cpp` models parent-dependent
  tag dispatch, including flow/phrasing restrictions, list/select special child
  sets, option text-only handling, void tags, and nested `a`/`li` rejection.
  Its 360 finite parent/tag/nesting combinations are compared with a C++
  transcription; recursion and DOM effects are not represented.
* `examples/blink_html_fastpath_attr_boundary.cpp` models the scan-frontier
  outcome of `ScanAttrName`: early EOF, unchanged prefix before a delimiter or
  invalid code unit, and the mapped full-name path when uppercase/digits occur.
  Fourteen finite prefix/frontier cases compare with a C++ branch transcription.
* `examples/blink_html_fastpath_attr_name.cpp` models `ScanAttrName`
  fast-path detection and ASCII lowercase mapping as recursive λΠ operations.
  Thirty bounded name/boundary states cover lowercase, uppercase, digits, and
  hyphens; malformed characters and the caller loop are omitted.
* `examples/blink_html_fastpath_attributes.cpp` models the fast parser's
  event-handler and `is` attribute bailouts, terminator validation, and
  duplicate canonical-name check. A typed λΠ recursive scan covers canonical
  attribute lists through length three and the two accepted endings; scanning
  names/values and DOM assignment remain abstract.
* `examples/blink_html_fastpath_context.cpp` models `Run` context dispatch for
  the fast HTML fragment parser: all 17 `SUPPORTED_TAGS` entries, the `body`
  special case, foreign namespaces, and unlisted tags. It distinguishes the
  accepted non-void contexts from rejected void contexts and covers the body
  whitespace behavior flag in 76 finite states. Child parsing is outside scope.
* `examples/blink_html_fastpath_gate.cpp` models the ordered fragment fast-path
  eligibility guards in `CanUseFastPath`, including the template exception to
  form-ancestor rejection. It covers all 32 Boolean input combinations; it
  does not model context dispatch or parsing after acceptance.
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
* `examples/blink_css_cascade_priority.cpp` compares the full documented
  ordering-field tuple with an independent packed `high_bits`/`low_bits`
  comparison across 18 precedence, equality, and field-boundary pairs. Rule
  collection, property-specific semantics, and declaration application are
  not modeled.
* `examples/blink_css_cascade_map.cpp` composes a selected packed priority
  field subset with strongest-first per-property insertion and
  `FindRevertLayer` first-eligible lookup. The C++ packed-key/list transcription
  and λΠ agree on eleven insertion prefixes and 99 revert-bound cases;
  origin/tree/style-prefix fields are fixed.
* `examples/blink_css_revert_color.cpp` composes first-eligible layer search,
  missing/not-weaker candidate fallback to CSS `unset`, and `color` inherited
  defaulting. Twelve finite cases compare C++ and λΠ across candidate hits,
  misses, ties, wide keywords, parent/root states, and explicit colors.
* `examples/blink_css_match_winner.cpp` composes tag/id/class simple-selector
  matching, matched-only declaration collection, and one-property winner
  selection. Nine finite C++/λΠ cases cover selector rejection, compound
  matches, priority-field ordering, and later-input selection on exact ties.
  Its three-field priority is an abstraction, not Blink's full sort key.
* `examples/blink_css_selector_color.cpp` connects restricted selector matching
  and winner selection to `color` explicit/initial/inherit/unset/missing
  defaulting. Twelve finite cases agree between its C++ transcription and
  typed λΠ, including parent/root behavior. Full CSS priority, actual values,
  and computed-style lifecycle remain outside this projection.
* `examples/blink_css_color_defaulting.cpp` models missing, `inherit`,
  `initial`, and `unset` outcomes for Blink's inherited `color` longhand, with
  parent-present and root cases. A C++ branch transcription and typed λΠ
  reduction agree on 24 finite cases; values are abstract color IDs.
* `examples/blink_style_recalc_gate.cpp` models the suppression-first
  `ShouldRecalcStyleFor` decision and the `ForChildren` propagation projection
  for `kIndependentInherit`, `kRecalcChildren`, and `kRecalcDescendants`, plus
  the style-container-query flag. Its C++ transcription agrees with λΠ on 320
  node/change inputs and 20 child-change inputs, including an Element without
  computed style; other flags and Element
  traversal remain outside scope.
* `examples/blink_style_recalc_dependencies.cpp` models grouped query-dependent
  `ShouldRecalcStyleFor` behavior. The λΠ/C++ comparison exhausts 64 grouped
  flag masks across seven dependency categories, Element/Text kinds, and old
  style present/absent (1,792 states), then checks the whole gate across five
  propagation levels, suppression, and dirty/clean state (35,840 states).
  Size flags match size-query dependencies or container-unit-dependent
  highlight styles; style, scroll-state, anchored, and cached-hover dependencies
  are checked separately. Content-visibility contributes to recalc-flag
  presence but has no per-node dependency predicate in this source method.
  Child/descendant variants are grouped by family; `FlagsForChildren`, pseudo
  element decisions, reattachment behavior, and Element traversal remain
  unmodeled.
* `examples/blink_style_recalc_children.cpp` models the exact 13-bit
  `FlagsForChildren` transform. Its C++/λΠ comparison exhausts all 8,192 input
  masks for old style present/absent and size-query matching true/false (32,768
  states), including conditional size-flag clearing and the suppression/
  mark-reattach interaction. A separate check composes the transform with all
  five propagation levels. Element traversal and style scheduling remain
  outside scope.
* `examples/blink_style_recalc_traversal.cpp` models the `TraverseChildren`,
  `TraversePseudoElements`, `TraverseChild`, `MarkReattachLayoutTree`,
  `ShouldUpdatePseudoElement`, and `IndependentInherit` predicates. λΠ/C++
  comparisons cover 640 cases each for the first two, 17,920 pseudo-element
  states, 1,280 independent-inherit states, eight mark-reattach combinations,
  the 64-case child traversal OR boundary, and 320
  representative compositions with `ShouldRecalcStyleFor`. The pseudo-element
  dependency set excludes cached hover, content visibility, and container-unit
  highlight styles, matching its separate source expression. Recursive Element
  traversal and style-engine scheduling remain outside scope.
* `examples/blink_style_recalc_copy.cpp` models the independent-inheritance
  shortcut boundary in `Element::RecalcOwnStyle` and
  `PropagateInheritedProperties`. The typed λΠ/C++ comparison exhausts 32,768
  combinations of three caller guards and twelve style eligibility conditions.
  Shortcut decline is distinguished from resolver entry, which also depends on
  a separate parent-style/root-layout condition. It does not model generated
  inherited-property fields or later style diff processing.
* `examples/blink_computed_style_difference.cpp` models the ordered style
  difference classifier and the outer null/pointer/cached-first-line maximum
  logic from `ComputedStyle::ComputeDifference`. The C++/typed-λΠ comparison
  covers 524,288 abstract classifier states and 576 outer-wrapper states. It
  also compares `ApplyComputedStyleDiff` over all 30 propagation/difference
  combinations, `NeedsReinsertLayoutTree` over 16 position/float states, and
  layout `ApplyStyleChanges` selection over 32 finite cases. The classifier
  predicates are independently varied, so some combinations may not correspond
  to concrete `ComputedStyle` pairs; generated field equality, cache evaluation,
  layout-tree mutation, and visual invalidation side effects are not modeled.
* `examples/blink_needs_reattach_layout_tree.cpp` models the ordered
  `ComputedStyle::NeedsReattachLayoutTree` decision. Its C++/typed-λΠ comparison
  covers 524,288 assignments to 19 source-condition observations, including
  null style, display/content, marker-group, appearance, legend, text-combine,
  first-line, overlay, and list-style triggers. Conditions derived from
  ComputedStyle fields and element subtype are abstract inputs, and layout-tree
  rebuilding is not modeled. The text-combine clause is further composed from
  all five concrete writing modes and old/new text-combine presence (100 states).
  `ScrollMarkerGroupEqual` is refined to `none` plus four position/mode values;
  all 25 old/new value pairs are checked and composed into the reattach result.
* `examples/blink_content_data_equivalence.cpp` models tagged `ContentData`
  equality and linked-list comparison, then routes inequality to the content
  reattach trigger. Typed λΠ and C++ comparisons cover 11,449 directed node
  pairs and 34,348 bounded list pairs, including Counter/AltCounter asymmetry.
  String, counter-style, and tree-scope payload values are finite atoms; ordinary list fixtures cover through length two. A relation-parameterized image-list path checks 589,824 singleton/head/tail/repeated-image comparisons over 147,456 subclass/relation states and composes inequality with reattachment.
* `examples/blink_crossfade_value_equality.cpp` models the ordered image/
  percentage pair comparison in `CSSCrossfadeValue::Equals` and composes it
  with `StyleCrossfadeImage::IsEqual`'s crossfade-kind guard. Five finite values
  cover empty, singleton, and length-two lists (25 ordered value pairs), with
  both right-hand image kinds (50 composed states). Nested CSSValue relations
  are represented by finite atoms; arbitrary crossfade values and rendering
  behavior remain outside scope.
* `examples/blink_image_set_cache.cpp` models `IsCachePending`, `CacheImage`,
  and permitted `CachedImage` reads for shared and fetcher-keyed storage. Seven
  shared states and 49 two-fetcher states cover 336 pending queries, 672 cache
  updates, and 4,032 composed update/query cases. Scale values include an
  adjacent-float strict-epsilon boundary. GC/weak-key lifetime and concurrent
  access are not modeled.
* `examples/blink_image_set_load_dispatch.cpp` models
  `StyleImageLoader::ResolveImageSet`: null selection and unsupported CSS value
  kinds return null; `CSSImageValue` and `CSSGradientValue` go through distinct
  load observations. The CSS image route carries the option's resolution
  override; the gradient route does not. Fifteen finite states are represented.
  Resource fetching and outer image-set caching remain outside scope.
* `examples/blink_image_set_failed_subresources.cpp` models the cached-image
  selection and resource-status result in `HasFailedOrCanceledSubresources`.
  Its 512 finite states include shared/fetcher-keyed cache choice, fetcher and
  feature gating, and four cached image/content states. Device scale is not
  consulted by this source method; resource internals are abstracted.
* `examples/blink_image_set_resolve_values.cpp` models the per-option
  `ResolveGradients` decision and `ResolveValuesIfNeeded` copy-or-original
  result. It preserves option order, resolution, and optional type fields across
  seven finite cases. Resolver behavior is an explicit same-reference or
  replacement observation; allocation and GC pointer details are abstracted.
* `examples/blink_image_set_type_support.cpp` models the finite
  `CSSImageSetTypeValue::IsSupported` and option positive-resolution predicate.
  It covers ASCII case variants, representative MIME support, AVIF build gating,
  and JXL build/runtime gating. Twenty finite option inputs, including all twelve static MIME types and case variants, cover 160 states. The predicate is also composed into the list-selection model.
* `examples/blink_image_set_selection.cpp` models the initial `GetBestOption`
  computation: support/positive-resolution filtering, stable sort, stable
  resolution deduplication, and threshold choice with highest-resolution
  fallback. Twenty-one finite lists are compared at four device scales and eight
  AVIF/JXL feature configurations (672 states).
  Resolution computation, lazy `options_` cache behavior, and subsequent load
  dispatch are not modeled.
* `examples/blink_image_set_value_equality.cpp` refines the image-set branch:
  `StyleImageSet::IsEqual` requires matching subclass, equivalent best-fit
  images, and equal `CSSImageSetValue` lists. Each ordered option compares its
  image, resolution, and optional type. Six finite option lists cover 36 list
  pairs and 144 StyleImageSet compositions; nested CSSValue and best-fit image
  relations remain bounded or abstract.
* `examples/blink_style_image_equality.cpp` models all six `StyleImage` subclass
  equality branches and the image-content reattach trigger across 147,456 kind/
  relation assignments. Its comparison loop checks the C++ transcription against λΠ.
  Underlying field equalities remain abstract Boolean inputs.
* `examples/blink_css_color_equality.cpp` models `CSSColor::Equals` and the
  underlying `Color::operator==` fields over 262,144 ordered pairs: two color
  spaces, four Boolean missing-component flags, and four components in `{0,1}`.
  The relation is reused for constant-gradient, color-image, and gradient-stop color fixtures in the generated-image model; those fixtures use a bounded red/blue subset.
* `examples/blink_generated_image_equality.cpp` refines generated-image equality
  into optional container dimensions, six viewport dimensions, and generator-value
  equivalence. Its C++/λΠ loop spans 663,552 finite states; numeric domains are
  restricted to absent/0/1 for container sizes and 0/1 for viewport components.
  A paint-specific extension models `CSSPaintValue::Equals` for six finite values
  built from two simple names and zero/one simple argument. It derives `paint(...)`
  text and checks 36 value pairs plus 144 image-size compositions.
  It also models the common `CSSGradientValue::Equals` fields and stop-list
  equality through two stops over 254,016 pairs; stop colors use structural CSSColor equality over bounded red/blue fixtures; offsets remain finite CSSValue atoms, with no null hints. It also models 144 targeted `CSSLinearGradientValue`
  pairs across deprecated, prefixed, and standard branches. A further 121 targeted
  `CSSRadialGradientValue` pairs cover legacy geometry, modern explicit sizes, and
  defaulted identifiers, retaining the source-directional gradient-type check.
  A 64-pair `CSSConicGradientValue` fixture path checks common fields and nullable
  x/y/from-angle values. Constant-gradient and color-image equality each cover
  two finite colors, with color-image equality delegating to the constant case.
  CSS color fields are structural over bounded fixtures; unrestricted CSSValue payload semantics remain outside scope.
  A tagged generator-value relation composes the six represented subclasses with
  container/viewport equality over 6,724 finite StyleGeneratedImage states and
  checks the resulting inequality decision.
* `examples/blink_diff_affects_scroll_animations.cpp` models the six-field
  `DiffAffectsScrollAnimations` predicate and its first-priority route into the
  computed-style difference severity. The C++/typed-λΠ comparisons cover all
  64 field-change combinations and the same 64 composed baseline states.
  Timeline values and `ValuesEquivalent` behavior are abstracted as inequality
  inputs.
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
* `examples/blink_tokenizer_attribute_name_bisimulation.cpp` models the
  AttributeName accumulator from an existing name prefix, with case
  normalization, delimiters, and quote/`<` parse-error append-and-continue
  behavior. Fourteen finite sequences compare C++ and λΠ; duplicate handling,
  source ranges, null preprocessing, and attribute values are omitted.
* `examples/blink_tokenizer_attribute_value_bisimulation.cpp` extends the
  tokenizer path through before-value, quoted/unquoted value, after-quoted, and
  self-closing states. Twenty finite traces compare value accumulation,
  parse-error flags, character-reference handoff mode, and emission/transition
  outcomes. Character-reference resolution remains opaque.
* `examples/blink_numeric_html_entity_bisimulation.cpp` models numeric
  character-reference prefix, decimal/hex accumulation, malformed-prefix
  rollback, terminators, and exhausted-input rollback. Forty-nine per-character
  finite traces agree between C++ and typed λΠ, including the U+10FFFF limit,
  overflow latch, and ignored later digits. Tested values connect through
  termination to replacement, an independent typed transcription of all 32
  U+0080–U+009F mappings (including
  unchanged C1 controls), U+007F/U+00A0 pass-through, and ordinary output.
  Arbitrary radix traces remain outside the model.
* `examples/blink_html_named_entity.cpp` parses and validates all 2,231 rows of
  the pinned names CSV and embeds 11 selected records in a focused λΠ trie.
  Twelve finite C++/λΠ cases cover longest-prefix selection, the semicolonless
  attribute guard, two-codepoint output, no match, and incomplete-prefix EOF
  rollback. Exact source cursor rewind is not modeled.
* `examples/blink_html_named_entity_states.cpp` generates a state machine from
  all 2,231 pinned names. The generated λΠ model has 9,854 trie states and
  59,258 rewrite rules, indexed by `RuleIndex`; twelve selected C++/λΠ result
  comparisons agree. It abstracts exact source rewind, segmented-input
  suspension, and tokenizer mutation, so full-table data coverage does not
  establish arbitrary-input parser equivalence.
* `examples/blink_html_named_entity_remainder.cpp` extends the generated state
  machine with decoder status, consumed spelling, generic list-prefix
  consumption, cursor offset, and remaining text. Twelve selected comparisons
  plus a two-chunk continuation pass. Offsets refer to text after `&`; actual
  `SegmentedString` mutation/segments and tokenizer-state changes remain
  unmodeled.
* `examples/blink_segmented_entity_source_bisimulation.cpp` models bounded
  segmented input operations and tokenizer suspension/resume around entity
  processing. Per-operation λΠ/C++ checks compare source shape and consumed
  offset for one/two-character `Push`, fallback prepend, longer unconsume and
  longest-prefix replay, plus Data/attribute append/resume traces. It uses
  bounded ASCII fixtures and does not include general trie decoding.
* `examples/blink_named_entity_segmented_composition.cpp` composes the
  2,231-name generated trie with direct typed-Source traversal, explicit
  `streamingExhausted`/`finalEOF` status, and tokenizer state/token-buffer
  transitions. Ten direct finite CursorSource cases cover chunk-split edges,
  mismatch lookahead, longest-match correction, exact and overread attribute
  guards, one/two-character Push, longer Prepend, and both end modes; fourteen
  legacy observer comparisons remain for cross-checking. A typed `enterCharacterReference` transition consumes `&` and maps the
  originating Data/double-quoted/single-quoted/unquoted value state to a cursor
  context; the remaining CursorSource feeds the guard and direct trie walker.
  Results update a cursor-preserving tokenizer state. Seven handoff
  fixtures and pending/append/resume in all four contexts compare output,
  entity flag, restored phase, and exact cursor source, including consumed ampersand offset.
  Double-quoted `amp;"`, single-quoted `NotEqualTilde;'`, and unquoted
  `notin;>` fixtures leave the value delimiter unconsumed. The
  trie consumes CursorSource directly; result checks compare the full cursor
  constructor, segment before/remaining lists, queued segment partition, and
  global offset. Backing String identity and intermediate cursors are omitted.
* `examples/blink_css_revert_origin.cpp` models CSS `revert` target-origin
  selection, `CascadeMap::Find(property, origin)` filtering, and the
  `ResolveRevertTo` priority fallback. Eleven finite C++/λΠ cases cover
  UA/user/hint/author/animation target origins, no candidate, no-origin,
  weaker, equal, and stronger candidates. Tree order, layers, inline/try flags,
  declaration collection, recursive value resolution, and computed-style
  application remain outside this projection.
* `examples/blink_css_revert_origin_color.cpp` composes that finite origin and
  priority projection with inherited `color` CSS-wide defaulting. Twelve
  C++/typed-λΠ cases cover selected explicit/initial/inherit/unset values,
  missing-candidate fallback, parent/root results, and weaker-priority
  rejection. Full packed `CascadePriority`, color-scheme and computed-style
  behavior remain unmodeled.
* `examples/blink_css_revert_origin_full_priority.cpp` composes revert-origin
  lookup and color defaulting with a λΠ tuple for all `CascadePriority` fields;
  a C++ packed-word transcription agrees on 18 composition cases and 12 direct
  field precedence/boundary pairs. The λΠ 16-bit field domain is finite, and it
  does not model packed 96-bit arithmetic or the full style lifecycle.
* `examples/blink_process_entity_bisimulation.cpp` models Data/RCDATA
  `ProcessEntity` effects over finite decoder outcomes. Eight cases compare
  success append (one/two codepoints plus entity flag), literal ampersand
  fallback, and pending-input suspension with typed λΠ. Decoder algorithms and
  exact stream mutations remain separate.
* `examples/blink_tokenizer_attribute_entity_bisimulation.cpp` composes bounded
  decoder outcomes with attribute-value append/fallback/suspension and quoted,
  single-quoted, or unquoted return states. Ten cases agree between the C++
  transcription and typed λΠ; full tables, arbitrary numeric decoding, and
  exact cursor rewind remain outside this slice.
* `examples/blink_dom_insert_html_element.cpp` models normal HTML element
  attachment using explicit DOM node parent IDs, open-element IDs, a fresh ID
  counter, and ordered finite token attribute name/value pairs copied to the
  new node. Four finite DOM/stack/attribute states agree between the C++
  reference and λΠ. Custom-element creation, sanitization, foster parenting,
  insertion adjustment, and callbacks are not represented.
* `examples/blink_tree_builder_p_start_dom_insert.cpp` composes an abstract
  `InButtonScope(kP)`/pop-through-p transition with attribute-aware normal DOM
  insertion. Four finite states cover top-p hit, hit with an ordinary entry
  above p, button-marker miss, and no p;
  the C++ transcription and typed λΠ agree. Synthetic end-tag parse-error and
  reprocessing control flow, custom/sanitizer behavior, and foster parenting
  are outside the model.
* `examples/blink_tokenized_p_start_dom.cpp` composes bounded raw `<p ...>`
  source scanning into typed `StartTagToken(pTag, Attrs)`, then passes the
  scanned attributes through p-scope pop and DOM insertion. Three inputs cover
  empty attributes, multiple ordered attributes including a typed `&amp;`
  decode, and alternate source order. The C++ comparison checks the complete
  token term and final DOM/stack state. This is a mapped transcription, not
  execution of Blink's tokenizer; Unicode, duplicate attrs, parse-error
  reconsume, custom/sanitizer effects, and full DOM lifecycle are excluded.
  The same raw source yields a start-tag token, text token through the next `<`,
  and restricted `</p>` token; the p-end transition closes the open-element
  stack. A quoted-attribute `>` case checks the tag/body boundary. The DOM
  then feeds fixed `p`/`.beta`/`#alpha` selector matching, numeric-priority
  color selection, and abstract text layout/paint. Six cases
  agree with the C++ reference; the
  both-selectors-hit case also checks declaration-order-independent winner
  selection. The `.beta` selector checks whitespace-separated class tokens
  and rejects the substring `betamax`. The C++ and typed λΠ projections derive one body CharacterToken from raw data
  before the next `<`; the bounded Data-state decoder handles `&amp;`. Seven cases
  compare the resulting DOM and parser state, including child ID and next ID,
  and one verifies `a&amp;b` becomes `a&b` in the Text child and paint projection.
  Typed `nodeText` concatenates child data for style/layout/paint;
  non-empty data reaches `textRun` and abstract `paintText`,
  while an empty token emits no text item. A
  changed-color simple-paint flag is projected only when text is present.
  Only a text run followed by `</p>` is accepted; other markup, body entity
  references except `&amp;`, streaming boundaries/suspensions, location-change flushes, and
  actual Blink Node pointers remain unmodeled. Text child IDs are abstract;
  glyph shaping,
  geometry, full layout, and Blink paint artifact internals remain outside scope. Full selector syntax,
  cascade priorities, and color scheme are also unmodeled.
* `examples/blink_pending_text_bisimulation.cpp` models Blink's pending-text
  accumulation at a parent/next-child insertion location and flush effects.
  Seven finite typed-λΠ/C++ cases cover same-location appends, location
  changes, whitespace-mode minimization, invalid/empty discard, insertion of a
  new Text node, and appending to an existing Text sibling. The source
  accumulator and flush are in `html_construction_site.h:264-303` and
  `html_construction_site.cc:245-271,1081-1118`. Same-location accumulation,
  flushing into a new Text child, and a second-flush Text-sibling coalescing
  path are composed into the raw token example. The full seven-case transition
  set remains separate. Actual DOM identity, canonicalization,
  template/foster adjustment, and task ordering remain outside this
  projection.
* `examples/blink_dom_text_flush_bisimulation.cpp` models the resulting child
  list at `FlushPendingText` for a null `next_child`. A valid non-empty buffer
  inserts a fresh Text child after a non-Text tail, or appends data to the last
  Text child while preserving its ID; empty/invalid buffers leave the child
  list and next ID unchanged. Seven finite λΠ/C++ cases cover those branches.
  This models abstract IDs rather than Blink pointers and omits adjusted
  template/foster locations, String canonicalization, and task ordering.
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
