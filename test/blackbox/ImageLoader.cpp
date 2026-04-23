/*
* Copyright 2016 Nu-book Inc.
* Copyright 2019 Axel Waggersauser.
*/
// SPDX-License-Identifier: Apache-2.0

#include "ImageLoader.h"

#include "BinaryBitmap.h"
#include "ImageView.h"

#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ZXing::Test {

class STBImage : public ImageView
{
	std::unique_ptr<stbi_uc[], void (*)(void*)> _memory;

public:
	STBImage() : _memory(nullptr, stbi_image_free) {}

	void load(const fs::path& imgPath)
	{
		int width, height, channels;
		_memory.reset(stbi_load(imgPath.string().c_str(), &width, &height, &channels, 0));
		if (_memory == nullptr)
			throw std::runtime_error("Failed to read image: " + imgPath.string() + " (" + stbi_failure_reason() + ")");

		auto ImageFormatFromChannels = std::array{ImageFormat::None, ImageFormat::Lum, ImageFormat::LumA, ImageFormat::RGB, ImageFormat::RGBA};
		ImageView::operator=({_memory.get(), width, height, ImageFormatFromChannels.at(channels)});
	}

	operator bool() const { return _data; }
};

std::map<fs::path, STBImage> cache;
std::mutex cacheMutex;

void ImageLoader::clearCache()
{
	std::scoped_lock lock(cacheMutex);
	cache.clear();
}

const ImageView& ImageLoader::load(const fs::path& imgPath)
{
	std::scoped_lock lock(cacheMutex);
	auto& binImg = cache[imgPath];
	if (!binImg)
		binImg.load(imgPath);

	return binImg;
}

} // namespace ZXing::Test
