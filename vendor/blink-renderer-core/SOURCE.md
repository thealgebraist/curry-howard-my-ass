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

## Earlier selected-file snapshot

The curated files in `vendor/blink-renderer-core` and the expanded directory
archives in `vendor/blink-src` are an earlier snapshot at
[`b8b9e813434efddb2eed020daa9cc44686aeee7a`](https://chromium.googlesource.com/chromium/src/+/b8b9e813434efddb2eed020daa9cc44686aeee7a).
They were fetched before the full sparse checkout was available and remain as
the small source excerpt associated with the first λΠ model.

The selected files below were fetched from `third_party/blink/renderer` at the
earlier revision:

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
