# Blink source snapshot used by the λΠ rendering model

Source repository: [chromium/src](https://chromium.googlesource.com/chromium/src)

## Current checkout

[`../chromium-src`](../chromium-src) is a shallow, blob-filtered sparse checkout
of `third_party/blink/renderer` at
[`99d77c2af39f60ee06a5516ad5428e4d2aa61da4`](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4).
The sparse path is the complete renderer subtree at that commit. Git tree
enumeration reported 18,986 tracked paths, all 18,986 present in the checkout;
13,266 paths have C/C++ source or header extensions. The renderer subtree
contains `core`, `modules`, `platform`, `bindings`, `controller`, and
`extensions`. This is not a full Chromium checkout, and it does not fetch the
dependencies referenced by GN/DEPS files.

That checkout is a local working-tree clone and is intentionally excluded from
this repository because it is a separate Chromium Git repository (about 320 MB
in this workspace). To recreate it, run:

```sh
git clone --filter=blob:none --no-checkout --depth=1 https://chromium.googlesource.com/chromium/src.git vendor/chromium-src
git -C vendor/chromium-src sparse-checkout init --cone
git -C vendor/chromium-src sparse-checkout set third_party/blink/renderer
git -C vendor/chromium-src checkout 99d77c2af39f60ee06a5516ad5428e4d2aa61da4
```

See [`RENDERER_INVENTORY.md`](RENDERER_INVENTORY.md) for the checked-out file
counts and the current semantic-model coverage.

## Additional source references for newer model slices

The two pinned checkouts above differ: the full sparse renderer is at
`99d77c2af39f60ee06a5516ad5428e4d2aa61da4`, while the curated excerpts below
remain at `b8b9e813434efddb2eed020daa9cc44686aeee7a`. The newer λΠ examples
refer to files from the full sparse checkout. Stable source links:

* [HTMLTreeBuilder::ProcessEndTagForInBody and the `</p>` branch](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_tree_builder.cc#2217)
* [HTMLConstructionSite implied end tags](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_construction_site.cc#113)
* [HTMLElementStack button-scope markers and cached paragraph state](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_element_stack.cc#145)
* [HTMLTreeBuilder list-item end-tag processing](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_tree_builder.cc#2309)
* [HTMLTokenizer::NextTokenImpl state dispatch](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_tokenizer.cc#260)
* [HTMLConstructionSite::AttachOrFosterParent and normal Attach](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_construction_site.cc#368)
* [HTMLConstructionSite::InsertHTMLElement](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_construction_site.cc#986)
* [HTMLConstructionSite::FlushPendingText](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_construction_site.cc#245)
* [HTMLConstructionSite::InsertTextNode](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_construction_site.cc#1081)
* [HTMLConstructionSite::PendingText accumulator](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_construction_site.h#264)
* [FlushPendingText appending to the previous Text sibling or inserting](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_construction_site.cc#265)
* [HTMLConstructionSite::CreateElement and token attribute attachment](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_construction_site.cc#1240)
* [Element::ParserSetAttributes](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/dom/element.cc#4216)
* [HTMLTreeBuilder shared in-body block-start branch](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_tree_builder.cc#855)
* [ProcessFakePEndTagIfPInButtonScope](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_tree_builder.cc#643)
* [HTMLElementStack::InButtonScope](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_element_stack.cc#493)
* [StyleCascade revert-layer resolution](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/style_cascade.cc#1593)
* [StyleBuilder CSS-wide keyword application](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/style_builder.cc#104)
* [Inherited `color` property metadata](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_properties.json5#1398)
* [`StyleResolverState::IsInheritedForUnset`](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/style_resolver_state.cc#164)
* [CascadeMap revert-layer candidate search](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/cascade_map.cc#78)
* [CascadeMap Add and sorted priority list insertion](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/cascade_map.cc#208)
* [CascadePriority encoding and layer comparison](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/cascade_priority.h#21)
* [ElementRuleCollector rule matching path](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/element_rule_collector.cc#577)
* [Easy selector matching for tag, id, and class](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/selector_checker-inl.h#218)
* [MatchedRule sort key and source-order sort](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/element_rule_collector.h#70)
* [StyleCascade collects matched declarations](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/style_cascade.cc#498)
* [StyleRecalcChange flag families and propagation](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/style_recalc_change.h#21)
* [StyleRecalcChange query-dependent node predicate](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/style_recalc_change.cc#30)
* [StyleRecalcChange pseudo-element update predicate](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/style_recalc_change.cc#75)
* [StyleRecalcChange independent inheritance check](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/style_recalc_change.cc#191)
* [Element independent inherited-property propagation caller](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/dom/element.cc#5894)
* [Element independent inherited-property copy eligibility](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/dom/element.cc#5746)
* [ComputedStyle difference classification and first-line aggregation](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/computed_style.cc#421)
* [DiffAffectsScrollAnimations timeline field comparisons](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/computed_style.cc#284)
* [ComputedStyle difference severity ordering](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/computed_style.h#377)
* [Element::ApplyComputedStyleDiff propagation update](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/dom/element.cc#5808)
* [ComputedStyle::NeedsReattachLayoutTree predicate](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/computed_style.cc#312)
* [ComputedStyle::ContentDataEquivalent delegation](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/computed_style.h#1633)
* [ContentData linked-list value equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/content_data.h#412)
* [CounterContentData equality fields](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/content_data.cc#149)
* [ContentData node variants and type-specific equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/content_data.h#110)
* [LayoutTextCombine supported writing modes](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/layout/layout_text_combine.h#137)
* [WritingMode values and typographic-mode predicates](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/platform/text/writing_mode.h#44)
* [ComputedStyle scroll-marker-group value equivalence](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/computed_style.h#2578)
* [ScrollMarkerGroup position/mode value equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/scroll_marker_group.h#25)
* [ComputedStyle::NeedsReinsertLayoutTree position/float predicate](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/computed_style.cc#406)
* [Element layout ApplyStyleChanges selection](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/dom/element.cc#6218)
* [CascadeOrigin enum and ordering codes](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/cascade_origin.h#15)
* [HTMLTokenizer attribute-name/value scanning and tag termination](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_tokenizer.cc#852)
* [HTMLToken attribute name/value accumulation](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_token.h#292)
* [HTMLTreeBuilder p-start branch pop and insert](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_tree_builder.cc#875)
* [HTMLTokenizer AttributeName state](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_tokenizer.cc#856)
* [HTMLTokenizer attribute-value states](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_tokenizer.cc#925)
* [LayoutObject::SetNeedsLayout](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/layout/layout_object_inlines.h#36)
* [LayoutObject::MarkContainerChainForLayout](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/layout/layout_object.cc#1634)

## Earlier selected-file snapshot

The curated files in `vendor/blink-renderer-core` and the expanded directory
archives in `vendor/blink-src` are an earlier snapshot at
[`b8b9e813434efddb2eed020daa9cc44686aeee7a`](https://chromium.googlesource.com/chromium/src/+/b8b9e813434efddb2eed020daa9cc44686aeee7a).
They were fetched before the full sparse checkout was available and remain as
the small source excerpt associated with the first λΠ model.

The selected files below were fetched from `third_party/blink/renderer` at the
earlier revision:

* [`paint/paint_chunker.cc`](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/platform/graphics/paint/paint_chunker.cc)
* [`paint/paint_chunker.h`](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/platform/graphics/paint/paint_chunker.h)

* `core/README.md`, `core/dom/README.md`, `core/css/README.md`,
  `core/layout/README.md`, `core/paint/README.md`
* `core/html/parser/html_document_parser.cc`
* `core/css/style_engine.cc`
* `core/layout/layout_object.cc`
* `platform/graphics/paint/paint_artifact.h`

The attempt to download the whole `core/` subtree and the CSS archives as
Gitiles archives returned HTTP 503. Separate HTML, DOM, layout, and paint
archives succeeded. Several individual CSS implementation files were fetched
as listed. The λΠ models encode selected control-flow and one closed rendering
example; they do not claim behavioral equivalence with Blink.

* [StyleImage equality dispatch](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_image.h#61)
* [Fetched image equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_fetched_image.cc#61)
* [Generated image equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_generated_image.cc#46)
* [Pending image equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_pending_image.h#105)
* [Image-set equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_image_set.cc#42)
* [Crossfade equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_crossfade_image.cc#29)
* [Mask-source equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_mask_source_image.cc#154)

* [Generated image equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_generated_image.cc#46)
* [ContainerSizes equality declaration](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_to_length_conversion_data.h#246)
* [ContainerSizes optional width/height equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_to_length_conversion_data.cc#258)
* [ViewportSize fields and defaulted value equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_to_length_conversion_data.h#162)

* [CSSPaintValue custom CSS text construction](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_paint_value.cc#47)
* [CSSPaintValue name projection](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_paint_value.cc#59)
* [CSSPaintValue equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_paint_value.cc#260)
* [CSSValue class-type equality dispatch](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_value.cc#192)

* [Gradient color-stop value equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_gradient_value.h#68)
* [CSSGradientValue common equality fields](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_gradient_value.cc#1699)
* [CSSLinearGradientValue subclass equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_gradient_value.cc#1470)

* [CSSGradientValue equality fields and stop vector](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_gradient_value.cc#1699)
* [CSSLinearGradientValue deprecated/modern equality branches](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_gradient_value.cc#1470)

* [CSSRadialGradientValue equality branches](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_gradient_value.cc#2040)
* [Radial shape/sizing identifier equality defaults](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_gradient_value.cc#2030)

* [CSSConicGradientValue equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_gradient_value.cc#2281)
* [CSSConstantGradientValue equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_gradient_value.cc#2379)
* [CSSColorImageValue equality delegation](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_gradient_value.cc#2457)
* [TryParsingHTMLFragment gate, parser-width dispatch, and reporting](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#1985)
* [HTMLFastPathParser tag-name scan and delimiter handling](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#942)
* [HTMLFastPathParser character-reference gate and dispatch](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#1209)
* [HTMLFastPathParser escaped-text and newline normalization](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#907)
* [HTMLFastPathParser plain and escaped text scan dispatch](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#914)
* [HTMLFastPathParser child-loop scan and append order](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#1347)
* [HTMLFastPathParser container closing-tag validation](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#1679)
* [HTMLFastPathParser supported child-tag dispatch](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#1600)
* [HTMLFastPathParser attribute-name scan frontier behavior](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#990)
* [HTMLFastPathParser attribute-name scan and lowercase mapping](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#990)
* [HTMLFastPathParser attribute scanning and rejection](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#1491)
* [HTMLFastPathParser context-tag dispatch](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#457)
* [HTML fragment fast-path eligibility gates](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_document_parser_fastpath.cc#1733)
* [StyleImageLoader image-set resolution then cache wrap](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/element_style_resources.cc#195)
* [StyleImageSet wrapper construction](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_image_set.cc#34)
* [CSSImageSetValue cached subresource failure lookup](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_value.cc#171)
* [CSSImageSetValue resolve-and-copy loop](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_value.cc#203)
* [CSSImageSetValue original-or-copy return](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_value.cc#225)
* [CSSImageSetTypeValue MIME support delegation](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_type_value.cc#28)
* [Blink supported-image MIME set and feature gates](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/common/mime_util/mime_util.cc#26)
* [Blink image MIME lowercasing and membership check](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/common/mime_util/mime_util.cc#117)
* [CSSImageSetOptionValue support conjunction](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_option_value.cc#38)
* [CSSImageSetValue cache pending predicate](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_value.cc#110)
* [CSSImageSetValue cached-image lookup](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_value.cc#123)
* [CSSImageSetValue cache update](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_value.cc#139)
* [StyleImage resolution equivalence epsilon](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_image.h#205)
* [CSSImageSetValue best-option filtering, sorting, deduplication, and selection](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_value.cc#49)
* [CSSImageSetOptionValue support predicate](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_option_value.cc#38)
* [CSSImageSetOptionValue computed resolution](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_option_value.cc#33)
* [CSSPrimitiveValue dots-per-pixel dispatch and clamping](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_primitive_value.cc#310)
* [CSSMathFunctionValue computed resolution](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_math_function_value.cc#74)
* [CSSNumericLiteralValue canonical resolution conversion](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_numeric_literal_value.cc#113)
* [CSSValueClampingUtils NaN handling and double bounds](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_value_clamping_utils.cc#10)
* [HTML numeric character-reference parsing and scalar correction](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_entity_parser.cc#135)
* [StyleImageLoader image-set resolution and load dispatch](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/element_style_resources.cc#195)
* [CSSValueList ordered list equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_value_list.cc#186)
* [CSSImageSetOptionValue field equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_image_set_option_value.cc#70)
* [StyleImageSet subclass, best-fit, and CSS value equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_image_set.cc#42)
* [CSSCrossfadeValue ordered image/percentage equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_crossfade_value.cc#86)
* [StyleCrossfadeImage subclass and original-value equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_crossfade_image.cc#29)

* [StyleGeneratedImage equality over sizes and generator value](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/style/style_generated_image.cc#46)

* [CSSColor value equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/css_color.h#31)
* [Color field-by-field equality](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/platform/graphics/color.h#380)
* [InputStreamPreprocessor CR/LF and NUL handling](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/input_stream_preprocessor.h#45)
* [HTMLTokenizer null-character policy and state enum](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/html/parser/html_tokenizer.h#57)
