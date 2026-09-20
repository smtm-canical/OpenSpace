// OSGameClient.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "EditorLayout.h"
#include "GameLoop.h"
#include "OSGameClient.h"
#include "PropertyPanel.h"

#define MAX_LOADSTRING 100

// 전역 변수:
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
GameLoop gGameLoop;
PropertyPanel gPropertyPanel;
HWND gStatusBar = nullptr;
bool gEditorMode = false;
bool gDraggingVerticalSplitter = false;
bool gLeftPanelWidthCustomized = false;
int gLeftPanelWidth = 0;

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                ToggleEditorMode(HWND hWnd);
void                ResizeEditorPanel(HWND hWnd);
void                ResizeClientLayout(HWND hWnd);
int                 GetContentHeight(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    INITCOMMONCONTROLSEX commonControls = {};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_TAB_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&commonControls);

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_OSGAMECLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_F1 &&
            (msg.lParam & (1ULL << 30)) == 0)
        {
            const HWND rootWindow = GetAncestor(msg.hwnd, GA_ROOT);
            if (rootWindow && rootWindow != msg.hwnd)
            {
                SendMessageW(rootWindow, msg.message, msg.wParam, msg.lParam);
                continue;
            }
        }
        if (gEditorMode && gPropertyPanel.HandleMessage(&msg))
        {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
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

    wcex.style          = 0;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OSGAMECLIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = nullptr;
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
   constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, windowStyle,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   RECT windowRect = {};
   GetWindowRect(hWnd, &windowRect);
   const LONG windowWidth = windowRect.right - windowRect.left;
   const LONG windowHeight = windowRect.bottom - windowRect.top;
   const LONG desktopWidth = GetSystemMetrics(SM_CXSCREEN);
   const LONG desktopHeight = GetSystemMetrics(SM_CYSCREEN);
   const LONG windowX = (desktopWidth - windowWidth) / 2;
   const LONG windowY = (desktopHeight - windowHeight) / 2;
   SetWindowPos(hWnd, nullptr, windowX, windowY, 0, 0,
      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            RECT clientRect = {};
            GetClientRect(hWnd, &clientRect);
            const int clientWidth = clientRect.right - clientRect.left;
            gLeftPanelWidth = clientWidth * EditorLayout::DefaultLeftPanelPercentage / 100;
            gLeftPanelWidth = (std::max)(gLeftPanelWidth, EditorLayout::MinimumLeftPanelWidth);
        }
        if (!gPropertyPanel.Create(hWnd, reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance))
        {
            MessageBoxW(hWnd, L"프로퍼티 시트를 생성하지 못했습니다.",
                L"OSGameClient", MB_OK | MB_ICONERROR);
            return -1;
        }
        gPropertyPanel.Show(false);
        gStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW,
            L"게임 모드 | F1: 모드 전환", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hWnd, nullptr,
            reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance, nullptr);
        if (!gStatusBar)
        {
            gPropertyPanel.Destroy();
            MessageBoxW(hWnd, L"상태바를 생성하지 못했습니다.",
                L"OSGameClient", MB_OK | MB_ICONERROR);
            return -1;
        }
        if (!gGameLoop.Start(hWnd))
        {
            DestroyWindow(gStatusBar);
            gStatusBar = nullptr;
            gPropertyPanel.Destroy();
            MessageBoxW(hWnd, L"화면에 필요한 이미지 파일을 불러오지 못했습니다.",
                L"OSGameClient", MB_OK | MB_ICONERROR);
            return -1;
        }
        gGameLoop.SetLeftPanelWidth(static_cast<UINT>(gLeftPanelWidth));
        ResizeClientLayout(hWnd);
        break;
    case WM_SIZE:
        ResizeClientLayout(hWnd);
        break;
    case WM_KEYDOWN:
        if (wParam == VK_F1 && (lParam & (1ULL << 30)) == 0)
        {
            ToggleEditorMode(hWnd);
        }
        break;
    case WM_SETCURSOR:
        if (gEditorMode && LOWORD(lParam) == HTCLIENT)
        {
            POINT cursorPosition = {};
            GetCursorPos(&cursorPosition);
            ScreenToClient(hWnd, &cursorPosition);
            if (cursorPosition.x >= gLeftPanelWidth &&
                cursorPosition.x < gLeftPanelWidth + EditorLayout::SplitterSize)
            {
                SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                return TRUE;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_LBUTTONDOWN:
        if (gEditorMode)
        {
            const int mouseX = GET_X_LPARAM(lParam);
            if (mouseX >= gLeftPanelWidth &&
                mouseX < gLeftPanelWidth + EditorLayout::SplitterSize)
            {
                gDraggingVerticalSplitter = true;
                SetCapture(hWnd);
                return 0;
            }
        }
        break;
    case WM_MOUSEMOVE:
        if (gDraggingVerticalSplitter)
        {
            RECT clientRect = {};
            GetClientRect(hWnd, &clientRect);
            const int maximumPanelWidth = (std::max)(EditorLayout::MinimumLeftPanelWidth,
                static_cast<int>(clientRect.right) - EditorLayout::SplitterSize);
            gLeftPanelWidth = (std::clamp)(GET_X_LPARAM(lParam),
                EditorLayout::MinimumLeftPanelWidth, maximumPanelWidth);
            gLeftPanelWidthCustomized = true;
            ResizeEditorPanel(hWnd);
            gGameLoop.SetLeftPanelWidth(static_cast<UINT>(gLeftPanelWidth));
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (gDraggingVerticalSplitter)
        {
            gDraggingVerticalSplitter = false;
            ReleaseCapture();
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        gDraggingVerticalSplitter = false;
        break;
    case WM_ERASEBKGND:
        return 1;
    case GameLoop::InitializationFailedMessage:
        MessageBoxW(hWnd, L"화면에 필요한 이미지 또는 그래픽 장치를 초기화하지 못했습니다.",
            L"OSGameClient", MB_OK | MB_ICONERROR);
        DestroyWindow(hWnd);
        return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        if (gStatusBar)
        {
            DestroyWindow(gStatusBar);
            gStatusBar = nullptr;
        }
        gPropertyPanel.Destroy();
        gGameLoop.Stop();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void ToggleEditorMode(HWND hWnd)
{
    gEditorMode = !gEditorMode;
    SendMessageW(gStatusBar, SB_SETTEXTW, 0,
        reinterpret_cast<LPARAM>(gEditorMode
            ? L"에디터 모드 | F1: 모드 전환"
            : L"게임 모드 | F1: 모드 전환"));
    gPropertyPanel.Show(gEditorMode);
    if (gEditorMode)
    {
        ResizeEditorPanel(hWnd);
    }
    gGameLoop.SetLeftPanelWidth(static_cast<UINT>(gLeftPanelWidth));
    gGameLoop.ToggleMode();
}

void ResizeEditorPanel(HWND hWnd)
{
    gPropertyPanel.Resize(gLeftPanelWidth, GetContentHeight(hWnd));
}

void ResizeClientLayout(HWND hWnd)
{
    RECT clientRect = {};
    GetClientRect(hWnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int maximumPanelWidth = (std::max)(clientWidth - EditorLayout::SplitterSize, 0);
    const int minimumPanelWidth = (std::min)(EditorLayout::MinimumLeftPanelWidth,
        maximumPanelWidth);
    const int desiredPanelWidth = gLeftPanelWidthCustomized
        ? gLeftPanelWidth
        : clientWidth * EditorLayout::DefaultLeftPanelPercentage / 100;
    const int resizedPanelWidth = (std::clamp)(desiredPanelWidth,
        minimumPanelWidth, maximumPanelWidth);
    if (resizedPanelWidth != gLeftPanelWidth)
    {
        gLeftPanelWidth = resizedPanelWidth;
        gGameLoop.SetLeftPanelWidth(static_cast<UINT>(gLeftPanelWidth));
    }

    if (gStatusBar)
    {
        SendMessageW(gStatusBar, WM_SIZE, 0, 0);
    }

    const int contentHeight = GetContentHeight(hWnd);
    gGameLoop.Resize(static_cast<UINT>(clientWidth),
        static_cast<UINT>((std::max)(contentHeight, 0)));
    if (gEditorMode)
    {
        ResizeEditorPanel(hWnd);
    }
}

int GetContentHeight(HWND hWnd)
{
    RECT clientRect = {};
    GetClientRect(hWnd, &clientRect);
    int statusBarHeight = 0;
    if (gStatusBar)
    {
        RECT statusBarRect = {};
        GetWindowRect(gStatusBar, &statusBarRect);
        statusBarHeight = statusBarRect.bottom - statusBarRect.top;
    }
    return static_cast<int>(clientRect.bottom - clientRect.top) - statusBarHeight;
}
