#include "pch.h"
#include "Sprite2D.h"

#include <wincodec.h>
#include <d2d1helper.h>

#include <limits>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "d2d1.lib")

namespace OS::Texture
{
	namespace
	{
		std::wstring ToWidePath(const std::string& filePath)
		{
			if (filePath.empty())
			{
				return {};
			}

			const int utf8Length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filePath.data(), static_cast<int>(filePath.size()), nullptr, 0);
			if (utf8Length > 0)
			{
				std::wstring widePath(utf8Length, L'\0');
				MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filePath.data(), static_cast<int>(filePath.size()), widePath.data(), utf8Length);
				return widePath;
			}

			const int ansiLength = MultiByteToWideChar(CP_ACP, 0, filePath.data(), static_cast<int>(filePath.size()), nullptr, 0);
			if (ansiLength == 0)
			{
				return {};
			}

			std::wstring widePath(ansiLength, L'\0');
			MultiByteToWideChar(CP_ACP, 0, filePath.data(), static_cast<int>(filePath.size()), widePath.data(), ansiLength);
			return widePath;
		}

		template <typename T>
		void ReleaseCom(T*& object)
		{
			if (object != nullptr)
			{
				object->Release();
				object = nullptr;
			}
		}
	}

	bool LoadFromFile(const std::string& filePath, Data& outTexture)
	{
		outTexture.Reset();

		const std::wstring widePath = ToWidePath(filePath);
		if (widePath.empty())
		{
			return false;
		}

		const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool shouldUninitializeCom = SUCCEEDED(comResult);
		if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE)
		{
			return false;
		}

		IWICImagingFactory* factory = nullptr;
		IWICBitmapDecoder* decoder = nullptr;
		IWICBitmapFrameDecode* frame = nullptr;
		IWICFormatConverter* converter = nullptr;

		HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
		if (SUCCEEDED(result))
		{
			result = factory->CreateDecoderFromFilename(widePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
		}
		if (SUCCEEDED(result))
		{
			result = decoder->GetFrame(0, &frame);
		}
		if (SUCCEEDED(result))
		{
			result = factory->CreateFormatConverter(&converter);
		}
		if (SUCCEEDED(result))
		{
			result = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
		}

		UINT width = 0;
		UINT height = 0;
		if (SUCCEEDED(result))
		{
			result = converter->GetSize(&width, &height);
		}

		constexpr UINT kBytesPerPixel = 4;
		if (SUCCEEDED(result) && (width == 0 || height == 0 || width > (std::numeric_limits<UINT>::max)() / kBytesPerPixel))
		{
			result = E_FAIL;
		}

		const UINT stride = SUCCEEDED(result) ? width * kBytesPerPixel : 0;
		const size_t byteCount = static_cast<size_t>(stride) * height;
		if (SUCCEEDED(result) && byteCount > (std::numeric_limits<UINT>::max)())
		{
			result = E_FAIL;
		}

		if (SUCCEEDED(result))
		{
			std::shared_ptr<unsigned char[]> pixelData(new unsigned char[byteCount]);
			result = converter->CopyPixels(nullptr, stride, static_cast<UINT>(byteCount), pixelData.get());
			if (SUCCEEDED(result))
			{
				outTexture.SetPixelData(std::move(pixelData), { width, height });
			}
		}

		ReleaseCom(converter);
		ReleaseCom(frame);
		ReleaseCom(decoder);
		ReleaseCom(factory);

		if (shouldUninitializeCom)
		{
			CoUninitialize();
		}

		return SUCCEEDED(result);
	}
}
