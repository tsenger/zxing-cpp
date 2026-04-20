# libjabcode

libjabcode is the reference implementation of JABCode (Just Another Bar Code), a color two-dimensional matrix
barcode symbology standardized as ISO/IEC 23634:2022. It was developed by Fraunhofer SIT.

The libjabcode project is hosted at https://github.com/jabcode/jabcode.

## Vendoring

The relevant source files have been copied directly into this repository to avoid an external dependency.
Only the decoding-related sources are included; `image.c` was deliberately excluded to avoid dependencies
on libpng and libtiff (image I/O is handled by ZXing's own `ImageView`).

Minor portability fixes were applied to `jabcode.h`:
- `MAX`/`MIN` macros replaced with standard C-compatible versions (the originals used GCC statement expressions)
- `JAB_REPORT_ERROR`/`JAB_REPORT_INFO` macros set to no-ops (the originals used `printf`)

Memory-leak fixes were applied to `detector.c` and `sample.c`:
- `seekMissingFinderPattern` leaked the `rgb[]` binary bitmaps and `fps_miss` buffer on every
  invocation; allocation-failure paths also leaked previously allocated siblings. Rewritten with a
  single cleanup tail.
- `sampleSymbolByAlignmentPattern` leaked `aps` when the output matrix allocation failed.
- `sampleSymbol` / `sampleCrossArea` leaked `matrix` when a mapped coordinate fell outside the
  bitmap bounds.

A type fix was applied to `checkPatternCross` in `detector.c`: `fabs()` on an integer difference
was replaced with `abs()` (the original relied on implicit int-to-double promotion).

These bugs were originally reported by Michael Schuster against upstream libjabcode
(https://github.com/misch7/jabcode/tree/fix-memory-leaks); the fixes here are independent
implementations of the same corrections.

## License

The files in this directory are licensed under the **MIT License**, Copyright (c) 2026 Fraunhofer SIT.
See the [LICENSE](LICENSE) file in this directory for the full text.

Upstream re-licensed from LGPLv3 (with a static linking exception) to MIT in commit `3b56eef`
(2026-04-17). MIT is compatible with ZXing-C++'s Apache 2.0 license.
