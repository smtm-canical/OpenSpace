#pragma once

#include <memory>
#include <string>
#include <utility>

#include <d2d1.h>
#include "vector"

namespace OS
{
	struct Vec2
	{
		float x = 0.0f;
		float y = 0.0f;
	};

	struct Rect
	{
		float x = 0.0f;
		float y = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
	};

	struct Size
	{
		unsigned int width = 0;
		unsigned int height = 0;
	};

	namespace Texture
	{
		struct Data
		{
			Size size{};
			unsigned char* pixels = nullptr;

			void SetPixelData(std::shared_ptr<unsigned char[]> pixelData, const Size& textureSize)
			{
				m_pixelBuffer = std::move(pixelData);
				pixels = m_pixelBuffer.get();
				size = textureSize;
			}

			void Reset()
			{
				m_pixelBuffer.reset();
				pixels = nullptr;
				size = {};
			}

			bool IsValid() const
			{
				return pixels != nullptr && size.width > 0 && size.height > 0;
			}

		private:
			std::shared_ptr<unsigned char[]> m_pixelBuffer;
		};

		bool LoadFromFile(const std::string& filePath, Data& outTexture);
	}

	struct Sprite2D
	{
		Texture::Data* texture = nullptr;
		Rect sourceRect{};
		Vec2 position{};
		Vec2 size{};
		Vec2 pivot{};
		float rotation = 0.0f;
	};
}
