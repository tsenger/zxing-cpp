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

## License

The files in this directory are licensed under the **MIT License**, Copyright (c) 2026 Fraunhofer SIT.
See the [LICENSE](LICENSE) file in this directory for the full text.

Upstream re-licensed from LGPLv3 (with a static linking exception) to MIT in commit `3b56eef`
(2026-04-17). MIT is compatible with ZXing-C++'s Apache 2.0 license.
