// OSMapTool.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "OSMapTool.h"
#include "CommandPopupController.h"

#include <d2d1helper.h>

#pragma comment(lib, "d2d1.lib")

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
CommandPopupController g_commandPopupController;
ID2D1Factory* g_d2dFactory = nullptr;
ID2D1HwndRenderTarget* g_renderTarget = nullptr;

namespace
{
	constexpr COLORREF kTitleBarBackground = RGB(18, 21, 35);

	bool CreateRenderTarget(HWND hWnd)
	{
		if (g_renderTarget != nullptr)
			return true;

		RECT client{};
		GetClientRect(hWnd, &client);
		return SUCCEEDED(g_d2dFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hWnd, D2D1::SizeU(client.right - client.left, client.bottom - client.top)),
			&g_renderTarget));
	}

	void ReleaseRenderTarget()
	{
		if (g_renderTarget != nullptr)
		{
			g_renderTarget->Release();
			g_renderTarget = nullptr;
		}
	}

	void RenderMainClient(HWND hWnd)
	{
		if (!CreateRenderTarget(hWnd))
		{
			return;
		}

		RECT client{};
		GetClientRect(hWnd, &client);
		const float width = static_cast<float>(client.right - client.left);
		const float height = static_cast<float>(client.bottom - client.top);
		g_renderTarget->Resize(D2D1::SizeU(static_cast<UINT32>(width), static_cast<UINT32>(height)));
		g_renderTarget->BeginDraw();
		g_renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		g_commandPopupController.Paint(g_renderTarget, width, height);

		if (g_renderTarget->EndDraw() == D2DERR_RECREATE_TARGET)
		{
			ReleaseRenderTarget();
		}
	}

	void SetWindowTitleBarColors(HWND hWnd)
	{
		// DWM API를 동적으로 로드하여 별도의 dwmapi.lib 의존성 없이 타이틀 바를 꾸밉니다.
		using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
		constexpr DWORD kDwmwaUseImmersiveDarkMode = 20;
		constexpr DWORD kDwmwaCaptionColor = 35;
		constexpr DWORD kDwmwaTextColor = 36;

		HMODULE dwmapi = LoadLibraryW(L"dwmapi.dll");
		if (dwmapi == nullptr)
			return;

		const auto setAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
			GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
		if (setAttribute != nullptr)
		{
			const BOOL darkMode = TRUE;
			const COLORREF titleText = RGB(220, 228, 255);
			setAttribute(hWnd, kDwmwaUseImmersiveDarkMode, &darkMode, sizeof(darkMode));
			setAttribute(hWnd, kDwmwaCaptionColor, &kTitleBarBackground, sizeof(kTitleBarBackground));
			setAttribute(hWnd, kDwmwaTextColor, &titleText, sizeof(titleText));
		}
		FreeLibrary(dwmapi);
	}

}

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE hPrevInstance,
					 _In_ LPWSTR    lpCmdLine,
					 _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 여기에 코드를 입력합니다.

	// 전역 문자열을 초기화합니다.
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_OSMAPTOOL, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (!g_commandPopupController.Initialize())
	{
		return FALSE;
	}

	// 애플리케이션 초기화를 수행합니다:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OSMAPTOOL));

	MSG msg;

	// 기본 메시지 루프입니다:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OSMAPTOOL));
	wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
	// WM_PAINT에서 직접 배경을 그리므로 기본 흰색 배경 브러시는 사용하지 않습니다.
	wcex.hbrBackground  = nullptr;
	// 키보드 중심 TUI 화면이므로 Win32 메뉴 바는 연결하지 않습니다.
	wcex.lpszMenuName   = nullptr;
	wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactory)))
   {
	  return FALSE;
   }

   HWND hWnd = CreateWindowExW(WS_EX_LAYERED, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
	  g_d2dFactory->Release();
	  g_d2dFactory = nullptr;
	  return FALSE;
   }

   SetWindowTitleBarColors(hWnd);
   // Do not expose the default DWM surface before the first dark frame is painted.
   SetLayeredWindowAttributes(hWnd, 0, 0, LWA_ALPHA);
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ERASEBKGND:
		// 팝업이 닫혀 메인 창이 다시 노출될 때 흰색으로 지워지는 것을 막습니다.
		return 1;
	case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			// 메뉴 선택을 구문 분석합니다:
			switch (wmId)
			{
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			RenderMainClient(hWnd);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_KEYDOWN:
		// 키를 누른 상태에서 발생하는 반복 WM_KEYDOWN은 무시합니다.
		if (wParam == VK_SPACE && (lParam & (1 << 30)) == 0)
		{
			g_commandPopupController.Toggle();
			InvalidateRect(hWnd, nullptr, FALSE);
			return 0;
		}
		break;
	case WM_DESTROY:
		g_commandPopupController.Shutdown();
		ReleaseRenderTarget();
		if (g_d2dFactory != nullptr)
		{
			g_d2dFactory->Release();
			g_d2dFactory = nullptr;
		}
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
