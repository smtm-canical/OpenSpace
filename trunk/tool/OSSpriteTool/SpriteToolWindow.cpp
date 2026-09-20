#include "framework.h"
#include "SpriteToolWindow.h"

#include <dwmapi.h>
#include <windowsx.h>

#pragma comment(lib, "dwmapi.lib")

namespace
{
	constexpr wchar_t kWindowClass[] = L"OSSpriteTool.MainWindow";
	constexpr int kTitleBarHeight = 36;
	constexpr int kStatusBarHeight = 28;
	constexpr int kSplitterWidth = 1;
	constexpr int kSplitterHitWidth = 7;
	constexpr int kMinimumPanelWidth = 180;
	constexpr int kCaptionButtonWidth = 46;
	constexpr int kResizeBorderWidth = 6;

	constexpr COLORREF kBackgroundHard = RGB(29, 32, 33);
	constexpr COLORREF kBackground = RGB(40, 40, 40);
	constexpr COLORREF kBackgroundLight = RGB(60, 56, 54);
	constexpr COLORREF kBackgroundHighlight = RGB(80, 73, 69);
	constexpr COLORREF kForeground = RGB(235, 219, 178);
	constexpr COLORREF kForegroundMuted = RGB(168, 153, 132);
	constexpr COLORREF kAccent = RGB(250, 189, 47);

	void FillRectangle(HDC deviceContext, const RECT& rectangle, COLORREF color)
	{
		const HBRUSH brush = CreateSolidBrush(color);
		FillRect(deviceContext, &rectangle, brush);
		DeleteObject(brush);
	}

	void DrawTextAt(HDC deviceContext, const wchar_t* text, RECT rectangle, COLORREF color, UINT format)
	{
		SetTextColor(deviceContext, color);
		SetBkMode(deviceContext, TRANSPARENT);
		DrawTextW(deviceContext, text, -1, &rectangle, format);
	}
}

bool SpriteToolWindow::Create(HINSTANCE instance, int commandShow)
{
	m_instance = instance;
	WNDCLASSW windowClass{};
	windowClass.hInstance = instance;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = nullptr;
	windowClass.lpszClassName = kWindowClass;
	windowClass.lpfnWndProc = WindowProc;
	if (RegisterClassW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
	{
		return false;
	}

	const DWORD windowStyle = WS_POPUP;
	m_window = CreateWindowExW(WS_EX_APPWINDOW, kWindowClass, L"OSSpriteTool", windowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, 1200, 760, nullptr, nullptr, instance, this);
	if (m_window == nullptr)
	{
		return false;
	}

	const BOOL useDarkMode = TRUE;
	DwmSetWindowAttribute(m_window, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
	const COLORREF noBorderColor = DWMWA_COLOR_NONE;
	DwmSetWindowAttribute(m_window, DWMWA_BORDER_COLOR, &noBorderColor, sizeof(noBorderColor));
	ShowWindow(m_window, commandShow);
	UpdateWindow(m_window);
	return true;
}

LRESULT CALLBACK SpriteToolWindow::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_NCCREATE)
	{
		const auto create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
		const auto application = reinterpret_cast<SpriteToolWindow*>(create->lpCreateParams);
		application->m_window = window;
		SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
	}

	const auto application = reinterpret_cast<SpriteToolWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
	if (application == nullptr)
	{
		return DefWindowProcW(window, message, wParam, lParam);
	}

	switch (message)
	{
	case WM_NCCALCSIZE:
		return 0;
	case WM_NCHITTEST:
	{
		POINT cursor{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(window, &cursor);
		RECT client{};
		GetClientRect(window, &client);
		if (!IsZoomed(window))
		{
			const bool onLeft = cursor.x < kResizeBorderWidth;
			const bool onRight = cursor.x >= client.right - kResizeBorderWidth;
			const bool onTop = cursor.y < kResizeBorderWidth;
			const bool onBottom = cursor.y >= client.bottom - kResizeBorderWidth;
			if (onTop && onLeft)
			{
				return HTTOPLEFT;
			}
			if (onTop && onRight)
			{
				return HTTOPRIGHT;
			}
			if (onBottom && onLeft)
			{
				return HTBOTTOMLEFT;
			}
			if (onBottom && onRight)
			{
				return HTBOTTOMRIGHT;
			}
			if (onLeft)
			{
				return HTLEFT;
			}
			if (onRight)
			{
				return HTRIGHT;
			}
			if (onTop)
			{
				return HTTOP;
			}
			if (onBottom)
			{
				return HTBOTTOM;
			}
		}
		if (cursor.y < kTitleBarHeight && cursor.x < client.right - kCaptionButtonWidth * 3)
		{
			return HTCAPTION;
		}
		return HTCLIENT;
	}
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT:
	{
		PAINTSTRUCT paint{};
		HDC deviceContext = BeginPaint(window, &paint);
		RECT client{};
		GetClientRect(window, &client);
		const int clientWidth = client.right - client.left;
		const int clientHeight = client.bottom - client.top;
		if (clientWidth > 0 && clientHeight > 0)
		{
			HDC bufferContext = CreateCompatibleDC(deviceContext);
			HBITMAP bufferBitmap = CreateCompatibleBitmap(deviceContext, clientWidth, clientHeight);
			HGDIOBJ previousBitmap = SelectObject(bufferContext, bufferBitmap);
			application->Draw(bufferContext);
			BitBlt(deviceContext, 0, 0, clientWidth, clientHeight, bufferContext, 0, 0, SRCCOPY);
			SelectObject(bufferContext, previousBitmap);
			DeleteObject(bufferBitmap);
			DeleteDC(bufferContext);
		}
		EndPaint(window, &paint);
		return 0;
	}
	case WM_SIZE:
		InvalidateRect(window, nullptr, FALSE);
		return 0;
	case WM_LBUTTONDOWN:
		if (application->IsOverSplitter(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
		{
			application->m_isDraggingSplitter = true;
			SetCapture(window);
		}
		return 0;
	case WM_MOUSEMOVE:
		if (application->m_isDraggingSplitter)
		{
			RECT client{};
			GetClientRect(window, &client);
			application->m_splitterPosition = max(kMinimumPanelWidth,
				min(GET_X_LPARAM(lParam), client.right - kMinimumPanelWidth));
			InvalidateRect(window, nullptr, FALSE);
		}
		else if (application->IsOverSplitter(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
		{
			SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
		}
		return 0;
	case WM_LBUTTONUP:
	{
		const int x = GET_X_LPARAM(lParam);
		const int y = GET_Y_LPARAM(lParam);
		RECT client{};
		GetClientRect(window, &client);
		if (y < kTitleBarHeight)
		{
			if (x >= client.right - kCaptionButtonWidth)
			{
				SendMessageW(window, WM_CLOSE, 0, 0);
			}
			else if (x >= client.right - kCaptionButtonWidth * 2)
			{
				ShowWindow(window, IsZoomed(window) ? SW_RESTORE : SW_MAXIMIZE);
			}
			else if (x >= client.right - kCaptionButtonWidth * 3)
			{
				ShowWindow(window, SW_MINIMIZE);
			}
			return 0;
		}
		if (application->m_isDraggingSplitter)
		{
			application->m_isDraggingSplitter = false;
			ReleaseCapture();
		}
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProcW(window, message, wParam, lParam);
	}
}

void SpriteToolWindow::Draw(HDC deviceContext) const
{
	RECT client{};
	GetClientRect(m_window, &client);
	const int statusTop = max(kTitleBarHeight, client.bottom - kStatusBarHeight);
	const int splitterLeft = min(m_splitterPosition, client.right - kMinimumPanelWidth);

	FillRectangle(deviceContext, client, kBackgroundHard);

	const RECT titleBar{ 0, 0, client.right, kTitleBarHeight };
	FillRectangle(deviceContext, titleBar, kBackgroundLight);
	const RECT titleBorder{ 0, kTitleBarHeight - 1, client.right, kTitleBarHeight };
	FillRectangle(deviceContext, titleBorder, kBackgroundHighlight);

	HPEN menuPen = CreatePen(PS_SOLID, 1, kForeground);
	HGDIOBJ previousPen = SelectObject(deviceContext, menuPen);
	for (int y = 14; y <= 22; y += 4)
	{
		MoveToEx(deviceContext, 19, y, nullptr);
		LineTo(deviceContext, 29, y);
	}
	SelectObject(deviceContext, previousPen);
	DeleteObject(menuPen);

	const HFONT titleFont = CreateFontW(-14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
	HGDIOBJ previousFont = SelectObject(deviceContext, titleFont);
	RECT titleText{ 46, 0, client.right, kTitleBarHeight };
	DrawTextAt(deviceContext, L"OSSpriteTool", titleText, kForeground, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
	SelectObject(deviceContext, previousFont);
	DeleteObject(titleFont);

	HPEN captionPen = CreatePen(PS_SOLID, 1, kForegroundMuted);
	previousPen = SelectObject(deviceContext, captionPen);
	HGDIOBJ previousBrush = SelectObject(deviceContext, GetStockObject(NULL_BRUSH));
	const int captionButtonLeft = client.right - kCaptionButtonWidth * 3;
	MoveToEx(deviceContext, captionButtonLeft + 17, 23, nullptr);
	LineTo(deviceContext, captionButtonLeft + 29, 23);
	Rectangle(deviceContext, captionButtonLeft + kCaptionButtonWidth + 17, 15,
		captionButtonLeft + kCaptionButtonWidth + 29, 27);
	MoveToEx(deviceContext, client.right - 29, 15, nullptr);
	LineTo(deviceContext, client.right - 17, 27);
	MoveToEx(deviceContext, client.right - 17, 15, nullptr);
	LineTo(deviceContext, client.right - 29, 27);
	SelectObject(deviceContext, previousPen);
	SelectObject(deviceContext, previousBrush);
	DeleteObject(captionPen);

	const RECT sidePanel{ 0, kTitleBarHeight, splitterLeft, statusTop };
	FillRectangle(deviceContext, sidePanel, kBackground);
	const RECT splitter{ splitterLeft, kTitleBarHeight, splitterLeft + kSplitterWidth, statusTop };
	FillRectangle(deviceContext, splitter, kBackgroundHighlight);

	const HFONT panelFont = CreateFontW(-12, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
	previousFont = SelectObject(deviceContext, panelFont);
	RECT explorerLabel{ 16, kTitleBarHeight + 16, splitterLeft - 16, kTitleBarHeight + 36 };
	DrawTextAt(deviceContext, L"EXPLORER", explorerLabel, kForegroundMuted, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
	RECT projectLabel{ 16, kTitleBarHeight + 52, splitterLeft - 16, kTitleBarHeight + 76 };
	DrawTextAt(deviceContext, L"OSSpriteTool", projectLabel, kAccent, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
	SelectObject(deviceContext, previousFont);
	DeleteObject(panelFont);

	const RECT statusBar{ 0, statusTop, client.right, client.bottom };
	FillRectangle(deviceContext, statusBar, kBackgroundLight);
	const RECT statusBorder{ 0, statusTop, client.right, statusTop + 1 };
	FillRectangle(deviceContext, statusBorder, kBackgroundHighlight);

	const HFONT statusFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Cascadia Mono");
	previousFont = SelectObject(deviceContext, statusFont);
	RECT readyText{ 16, statusTop, client.right / 2, client.bottom };
	DrawTextAt(deviceContext, L"Ready", readyText, kForegroundMuted, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
	RECT themeText{ client.right / 2, statusTop, client.right - 16, client.bottom };
	DrawTextAt(deviceContext, L"Gruvbox Dark Hard", themeText, kForegroundMuted, DT_SINGLELINE | DT_VCENTER | DT_RIGHT);
	SelectObject(deviceContext, previousFont);
	DeleteObject(statusFont);
}

bool SpriteToolWindow::IsOverSplitter(int x, int y) const
{
	RECT client{};
	GetClientRect(m_window, &client);
	const int splitterHitLeft = m_splitterPosition - kSplitterHitWidth / 2;
	return x >= splitterHitLeft && x < splitterHitLeft + kSplitterHitWidth &&
		y >= kTitleBarHeight && y < client.bottom - kStatusBarHeight;
}
