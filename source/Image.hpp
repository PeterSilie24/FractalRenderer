#pragma once

#include "Includes.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace img
{
	struct Image
	{
		Image(const std::size_t width = 0, const std::size_t height = 0) :
			width(width), height(height), pixels(width * height)
		{

		}

		Image(const std::size_t width, const std::size_t height, const std::vector<int>& pixels) :
			width(width), height(height), pixels(pixels)
		{

		}

		std::vector<int> pixels;

		std::size_t width;
		std::size_t height;
	};

	typedef std::shared_ptr<Image> ImagePtr;

	template <typename... Arguments>
	inline ImagePtr make(Arguments&&... arguments)
	{
		return std::make_shared<Image>(arguments...);
	}

	ImagePtr load(const std::string& path, const bool flip = false)
	{
		stbi_set_flip_vertically_on_load(flip);

		int x, y, comp;

		auto pixels = stbi_load(path.c_str(), &x, &y, &comp, STBI_rgb_alpha);

		if (pixels)
		{
			ImagePtr image = make(x, y);

			std::memcpy(image->pixels.data(), pixels, image->pixels.size() * 4);

			stbi_image_free(pixels);

			return image;
		}

		return ImagePtr();
	}

	bool save(const std::string& path, const ImagePtr& image, const bool flip = false)
	{
		if (!image)
		{
			return false;
		}

		stbi_flip_vertically_on_write(flip);

		std::string ext = "";

		auto pos = path.rfind(".");

		if (pos != std::string::npos)
		{
			ext = std::string(path.begin() + pos + 1, path.end());
		}

		std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

		if (ext == "png")
		{
			return stbi_write_png(path.c_str(), image->width, image->height, 4, image->pixels.data(), image->width * 4) != 0;
		}
		else if (ext == "jpg" || ext == "jpeg")
		{
			return stbi_write_jpg(path.c_str(), image->width, image->height, 4, image->pixels.data(), 100) != 0;
		}
		else if (ext == "tga")
		{
			return stbi_write_tga(path.c_str(), image->width, image->height, 4, image->pixels.data()) != 0;
		}
		else
		{
			return stbi_write_bmp(path.c_str(), image->width, image->height, 4, image->pixels.data()) != 0;
		}
	}
}
