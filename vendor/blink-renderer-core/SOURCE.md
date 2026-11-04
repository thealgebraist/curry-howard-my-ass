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
* [CascadeOrigin enum and ordering codes](https://chromium.googlesource.com/chromium/src/+/99d77c2af39f60ee06a5516ad5428e4d2aa61da4/third_party/blink/renderer/core/css/resolver/cascade_origin.h#15)
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
