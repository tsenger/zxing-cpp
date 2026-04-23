/*
 * Copyright 2026 ZXing authors
 */
// SPDX-License-Identifier: Apache-2.0

#include "JABReader.h"

#include "BarcodeData.h"
#include "BarcodeFormat.h"
#include "Content.h"
#include "ReaderOptions.h"

extern "C" {
#include "jabcode.h"
}

#include <cstdlib>
#include <cstring>
#include <memory>

namespace ZXing {

struct JabBitmapDeleter
{
	void operator()(jab_bitmap* p) const noexcept { std::free(p); }
};
using JabBitmapPtr = std::unique_ptr<jab_bitmap, JabBitmapDeleter>;

static JabBitmapPtr ToJabBitmap(const ImageView& iv)
{
	int srcW = iv.width();
	int srcH = iv.height();

	// Downsample large images by successive halving until the longer side fits within 400px.
	// JABCode finder patterns remain detectable at reduced resolution, and this avoids running
	// the expensive binarizer + pattern scan on megapixel false-positive images.
	int scale = 1;
	while (std::max(srcW, srcH) / scale > 512)
		scale *= 2;

	int width  = srcW / scale;
	int height = srcH / scale;
	int scale2 = scale * scale;

	std::size_t pixelBytes = static_cast<std::size_t>(width) * height * 4;
	auto* bmp = static_cast<jab_bitmap*>(std::calloc(1, sizeof(jab_bitmap) + pixelBytes));
	if (!bmp)
		return {nullptr, {}};

	bmp->width = width;
	bmp->height = height;
	bmp->bits_per_pixel = BITMAP_BITS_PER_PIXEL;
	bmp->bits_per_channel = BITMAP_BITS_PER_CHANNEL;
	bmp->channel_count = BITMAP_CHANNEL_COUNT;

	if (iv.format() == ImageFormat::Lum) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int sum = 0;
				for (int dy = 0; dy < scale; ++dy)
					for (int dx = 0; dx < scale; ++dx)
						sum += iv.data(x * scale + dx, y * scale + dy)[0];
				uint8_t v = static_cast<uint8_t>(sum / scale2);
				uint8_t* dst = bmp->pixel + (y * width + x) * 4;
				dst[0] = dst[1] = dst[2] = v;
				dst[3] = 255;
			}
		}
	} else {
		int ri = RedIndex(iv.format());
		int gi = GreenIndex(iv.format());
		int bi = BlueIndex(iv.format());
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int r = 0, g = 0, b = 0;
				for (int dy = 0; dy < scale; ++dy) {
					for (int dx = 0; dx < scale; ++dx) {
						const uint8_t* src = iv.data(x * scale + dx, y * scale + dy);
						r += src[ri];
						g += src[gi];
						b += src[bi];
					}
				}
				uint8_t* dst = bmp->pixel + (y * width + x) * 4;
				dst[0] = static_cast<uint8_t>(r / scale2);
				dst[1] = static_cast<uint8_t>(g / scale2);
				dst[2] = static_cast<uint8_t>(b / scale2);
				dst[3] = 255;
			}
		}
	}

	return JabBitmapPtr(bmp);
}

static bool hasColorContent(const ImageView& iv)
{
	// JABCode finder patterns are strongly saturated (pure R/G/B/Y/C/M).
	// Sample a sparse grid; if no pixel has max(R,G,B)-min(R,G,B) above the
	// threshold, there can be no finder patterns and decoding can be skipped.
	constexpr int kThreshold = 32;
	constexpr int kSteps = 8;
	int stepX = std::max(1, iv.width()  / kSteps);
	int stepY = std::max(1, iv.height() / kSteps);
	int ri = RedIndex(iv.format());
	int gi = GreenIndex(iv.format());
	int bi = BlueIndex(iv.format());
	for (int y = 0; y < iv.height(); y += stepY)
		for (int x = 0; x < iv.width(); x += stepX) {
			const uint8_t* p = iv.data(x, y);
			int lo = std::min({(int)p[ri], (int)p[gi], (int)p[bi]});
			int hi = std::max({(int)p[ri], (int)p[gi], (int)p[bi]});
			if (hi - lo > kThreshold)
				return true;
		}
	return false;
}

Barcodes ReadJABCodes(const ImageView& image, int maxSymbols, [[maybe_unused]] const ReaderOptions& opts)
{
	Barcodes results;

	if (maxSymbols <= 0)
		return results;

	if (!hasColorContent(image))
		return results;

	auto bmp = ToJabBitmap(image);
	if (!bmp)
		return results;

	jab_int32 status = 0;
	jab_decoded_symbol symbols[MAX_SYMBOL_NUMBER];
	std::memset(symbols, 0, sizeof(symbols));

	jab_data* data = decodeJABCodeEx(bmp.get(), NORMAL_DECODE, &status, symbols, MAX_SYMBOL_NUMBER);

	if (!data || status < 2) {
		std::free(data);
		return results;
	}

	// Build Content from decoded bytes
	ByteArray bytes;
	bytes.assign(reinterpret_cast<uint8_t*>(data->data),
	             reinterpret_cast<uint8_t*>(data->data) + data->length);
	std::free(data);

	// JABCode does not have an ISO/IEC 15424 symbology identifier assigned.
	// Use 'J' / '0' as a provisional identifier.
	Content content(std::move(bytes), SymbologyIdentifier{'J', '0'}, CharacterSet::Unknown);

	// Build position from master symbol finder pattern positions
	// FP0 = topLeft, FP1 = topRight, FP2 = bottomRight, FP3 = bottomLeft
	auto& pp = symbols[0].pattern_positions;
	Position position(
		PointI(static_cast<int>(pp[0].x + 0.5f), static_cast<int>(pp[0].y + 0.5f)),
		PointI(static_cast<int>(pp[1].x + 0.5f), static_cast<int>(pp[1].y + 0.5f)),
		PointI(static_cast<int>(pp[2].x + 0.5f), static_cast<int>(pp[2].y + 0.5f)),
		PointI(static_cast<int>(pp[3].x + 0.5f), static_cast<int>(pp[3].y + 0.5f))
	);

	BarcodeData bd;
	bd.content = std::move(content);
	bd.format = BarcodeFormat::JABCode;
	bd.position = position;

	if (status == 2)
		bd.error = Error(Error::Checksum);

	results.emplace_back(std::move(bd));

	return results;
}

} // namespace ZXing
