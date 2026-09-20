#pragma once

#include <windows.h>
#include <objidl.h>
#include <propidl.h>

#include <gdiplus.h>

#include <memory>
#include <string>

class ImagePreviewControl
{
public:
	bool Create(HWND parentWindow, HINSTANCE instance);
	bool LoadFile(const std::wstring& filePath);
	void Clear();
	void Resize(int x, int y, int width, int height) const;

private:
	static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	void Paint(HDC deviceContext) const;

	HWND m_window = nullptr;
	std::unique_ptr<Gdiplus::Image> m_image;
	std::wstring m_message = L"Select an image file to preview.";
};
