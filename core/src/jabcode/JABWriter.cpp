/*
 * Copyright 2026 ZXing authors
 */
// SPDX-License-Identifier: Apache-2.0

#include "JABWriter.h"

#include "BarcodeData.h"
#include "BarcodeFormat.h"
#include "Content.h"
#include "CreateBarcode.h"

extern "C" {
#include "jabcode.h"
}

#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

namespace ZXing {

struct JabEncodeDeleter
{
	void operator()(jab_encode* p) const noexcept { destroyEncode(p); }
};
using JabEncodePtr = std::unique_ptr<jab_encode, JabEncodeDeleter>;

Barcode CreateJABCode(const void* data, int size, const CreatorOptions& opts)
{
	if (!data || size < 1)
		throw std::invalid_argument("Cannot create a JABCode from NULL or empty data");

	int colorNumber = DEFAULT_COLOR_NUMBER;
	int symbolNumber = DEFAULT_SYMBOL_NUMBER;

	JabEncodePtr enc(createEncode(colorNumber, symbolNumber));
	if (!enc)
		throw std::runtime_error("Failed to create JABCode encoder");

	// Apply options
	if (auto val = opts.ecLevel(); val) {
		int level = std::stoi(*val);
		if (level >= 0 && level <= 10)
			enc->symbol_ecc_levels[0] = static_cast<jab_byte>(level);
	}
	if (auto val = opts.version(); val) {
		if (*val >= 1 && *val <= 32) {
			enc->symbol_versions[0].x = *val;
			enc->symbol_versions[0].y = *val;
		}
	}

	// Prepare input data
	auto* jabData = static_cast<jab_data*>(std::malloc(sizeof(jab_data) + size));
	if (!jabData)
		throw std::runtime_error("Failed to allocate JABCode data");
	jabData->length = size;
	std::memcpy(jabData->data, data, size);

	jab_int32 result = generateJABCode(enc.get(), jabData);
	std::free(jabData);

	if (result != 0)
		throw std::runtime_error("JABCode generation failed");

	jab_bitmap* bmp = enc->bitmap;
	if (!bmp || bmp->width <= 0 || bmp->height <= 0)
		throw std::runtime_error("JABCode generation produced no bitmap");

	// Create color RGBA image for full-color output
	Image colorBitmap(bmp->width, bmp->height, ImageFormat::RGBA);
	std::memcpy(const_cast<uint8_t*>(colorBitmap.data()), bmp->pixel,
	            static_cast<size_t>(bmp->width) * bmp->height * 4);

	// Create monochrome BitMatrix for symbol() / SVG / UTF8 output
	BitMatrix bits(bmp->width, bmp->height);
	for (int y = 0; y < bmp->height; ++y) {
		for (int x = 0; x < bmp->width; ++x) {
			const uint8_t* px = bmp->pixel + (y * bmp->width + x) * 4;
			// white (or near-white) background -> not set, everything else -> set
			bool isWhite = (px[0] > 200 && px[1] > 200 && px[2] > 200);
			if (!isWhite)
				bits.set(x, y);
		}
	}

	// Build Content
	Content content;
	content.append({static_cast<const uint8_t*>(data), static_cast<size_t>(size)});
	content.symbology = SymbologyIdentifier{'J', '0'};

	DecoderResult decRes(std::move(content));

	// Flip for standard luminance convention (BitMatrix stores foreground as SET)
	bits.flipAll();

	int left, top, width, height;
	bits.findBoundingBox(left, top, width, height);

	auto bd = MatrixBarcode(std::move(decRes), {std::move(bits), Rectangle<PointI>(left, top, width, height)}, BarcodeFormat::JABCode);
	bd.colorBitmap = std::move(colorBitmap);

	return Barcode(std::move(bd));
}

} // namespace ZXing
