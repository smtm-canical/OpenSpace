#include "framework.h"
#include "ImagePreviewControl.h"

#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

bool ImagePreviewControl::Create(HWND parentWindow, HINSTANCE instance)
{
	constexpr wchar_t className[] = L"OSSpriteTool.ImagePreview";
	static bool isRegistered = false;
	if (!isRegistered)
	{
		WNDCLASSW windowClass{};
		windowClass.hInstance = instance;
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		windowClass.lpszClassName = className;
		windowClass.lpfnWndProc = WindowProc;
		isRegistered = RegisterClassW(&windowClass) != 0;
	}

	m_window = CreateWindowExW(WS_EX_CLIENTEDGE, className, nullptr, WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0, parentWindow, nullptr, instance, this);
	return m_window != nullptr;
}

bool ImagePreviewControl::LoadFile(const std::wstring& filePath)
{
	auto image = std::make_unique<Gdiplus::Image>(filePath.c_str());
	if (image->GetLastStatus() != Gdiplus::Ok)
	{
		m_image.reset();
		m_message = L"This file cannot be previewed as an image.";
		InvalidateRect(m_window, nullptr, TRUE);
		return false;
	}

	m_image = std::move(image);
	m_message.clear();
	InvalidateRect(m_window, nullptr, TRUE);
	return true;
}

void ImagePreviewControl::Clear()
{
	m_image.reset();
	m_message = L"Select an image file to preview.";
	InvalidateRect(m_window, nullptr, TRUE);
}

void ImagePreviewControl::Resize(int x, int y, int width, int height) const
{
	MoveWindow(m_window, x, y, width, height, TRUE);
}

LRESULT CALLBACK ImagePreviewControl::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_NCCREATE)
	{
		const auto create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
		SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
	}

	const auto control = reinterpret_cast<ImagePreviewControl*>(GetWindowLongPtrW(window, GWLP_USERDATA));
	if (message == WM_PAINT && control != nullptr)
	{
		PAINTSTRUCT paint{};
		HDC deviceContext = BeginPaint(window, &paint);
		control->Paint(deviceContext);
		EndPaint(window, &paint);
		return 0;
	}

	return DefWindowProcW(window, message, wParam, lParam);
}

void ImagePreviewControl::Paint(HDC deviceContext) const
{
	RECT client{};
	GetClientRect(m_window, &client);
	FillRect(deviceContext, &client, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

	Gdiplus::Graphics graphics(deviceContext);
	graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	if (m_image == nullptr)
	{
		Gdiplus::Font font(L"Segoe UI", 12.0f);
		Gdiplus::SolidBrush brush(Gdiplus::Color(90, 90, 90));
		graphics.DrawString(m_message.c_str(), -1, &font, Gdiplus::PointF(16.0f, 16.0f), &brush);
		return;
	}

	const float imageWidth = static_cast<float>(m_image->GetWidth());
	const float imageHeight = static_cast<float>(m_image->GetHeight());
	const float availableWidth = static_cast<float>(client.right - client.left - 32);
	const float availableHeight = static_cast<float>(client.bottom - client.top - 32);
	const float scale = min(availableWidth / imageWidth, availableHeight / imageHeight);
	const float width = imageWidth * scale;
	const float height = imageHeight * scale;
	const float x = (client.right - width) / 2.0f;
	const float y = (client.bottom - height) / 2.0f;
	graphics.DrawImage(m_image.get(), x, y, width, height);
}
