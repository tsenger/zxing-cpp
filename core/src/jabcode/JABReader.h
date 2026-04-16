/*
 * Copyright 2026 ZXing authors
 */
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Barcode.h"
#include "ImageView.h"

namespace ZXing {

class ReaderOptions;

Barcodes ReadJABCodes(const ImageView& image, int maxSymbols, const ReaderOptions& opts);

} // namespace ZXing
