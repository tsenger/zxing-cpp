/*
 * Copyright 2026 ZXing authors
 */
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Barcode.h"

namespace ZXing {

class CreatorOptions;

Barcode CreateJABCode(const void* data, int size, const CreatorOptions& opts);

} // namespace ZXing
