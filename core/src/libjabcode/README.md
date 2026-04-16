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

The files in this directory are licensed under the **GNU Lesser General Public License v3.0** (LGPLv3)
with a static linking exception. See the [LICENSE](LICENSE) file in this directory for the full text.

This is a separate license from the rest of ZXing-C++, which is licensed under Apache 2.0. The libjabcode
code is contained entirely within this directory and is only compiled into the library when the CMake option
`ZXING_ENABLE_JABCODE=ON` is set (default: OFF). No libjabcode code has been copied into Apache 2.0 licensed
files; the integration layer (`core/src/jabcode/JABReader.cpp`) is original ZXing code under Apache 2.0 that
calls the public libjabcode API.

Users who build with `ZXING_ENABLE_JABCODE=OFF` (the default) are not affected by LGPLv3 in any way.
