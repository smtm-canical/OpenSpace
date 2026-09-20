#include "framework.h"
#include "PropertyPanel.h"
#include "Resource.h"

#include <array>

namespace
{
    constexpr COLORREF BackgroundColor = RGB(30, 30, 30);
    constexpr COLORREF PageColor = RGB(37, 37, 38);
    constexpr COLORREF SelectedTabColor = RGB(45, 45, 48);
    constexpr COLORREF TabTextColor = RGB(241, 241, 241);
    constexpr COLORREF InactiveTabTextColor = RGB(204, 204, 204);
    constexpr COLORREF AccentColor = RGB(156, 104, 192);
    constexpr COLORREF BorderColor = RGB(63, 63, 70);

    void ResizePropertyPage(HWND pageWindow)
    {
        const HWND sheetWindow = GetParent(pageWindow);
        const HWND tabControl = PropSheet_GetTabControl(sheetWindow);
        if (!tabControl)
        {
            return;
        }

        RECT tabRect = {};
        GetClientRect(tabControl, &tabRect);
        TabCtrl_AdjustRect(tabControl, FALSE, &tabRect);
        MapWindowPoints(tabControl, sheetWindow,
            reinterpret_cast<POINT*>(&tabRect), 2);
        SetWindowPos(pageWindow, HWND_TOP, tabRect.left, tabRect.top,
            tabRect.right - tabRect.left, tabRect.bottom - tabRect.top,
            SWP_NOACTIVATE);
    }

}

INT_PTR CALLBACK PropertyPanel::PropertyPageProc(HWND window, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INITDIALOG)
    {
        const PROPSHEETPAGEW* page = reinterpret_cast<PROPSHEETPAGEW*>(lParam);
        SetWindowLongPtrW(window, GWLP_USERDATA, page->lParam);
    }

    PropertyPanel* panel = reinterpret_cast<PropertyPanel*>(
        GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message)
    {
    case WM_SHOWWINDOW:
        if (wParam != FALSE)
        {
            ResizePropertyPage(window);
        }
        break;
    case WM_ERASEBKGND:
        if (panel && panel->mPageBrush)
        {
            RECT clientRect = {};
            GetClientRect(window, &clientRect);
            FillRect(reinterpret_cast<HDC>(wParam), &clientRect, panel->mPageBrush);
            return TRUE;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        if (panel && panel->mPageBrush)
        {
            HDC deviceContext = reinterpret_cast<HDC>(wParam);
            SetTextColor(deviceContext, TabTextColor);
            SetBkColor(deviceContext, PageColor);
            return reinterpret_cast<INT_PTR>(panel->mPageBrush);
        }
        break;
    }
    return FALSE;
}

LRESULT CALLBACK PropertyPanel::SheetSubclassProc(HWND window, UINT message,
    WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR referenceData)
{
    PropertyPanel* panel = reinterpret_cast<PropertyPanel*>(referenceData);
    switch (message)
    {
    case WM_DRAWITEM:
        if (panel)
        {
            const DRAWITEMSTRUCT* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (drawItem && drawItem->hwndItem == PropSheet_GetTabControl(window))
            {
                panel->DrawTab(*drawItem);
                return TRUE;
            }
        }
        break;
    case WM_ERASEBKGND:
        if (panel && panel->mBackgroundBrush)
        {
            RECT clientRect = {};
            GetClientRect(window, &clientRect);
            FillRect(reinterpret_cast<HDC>(wParam), &clientRect, panel->mBackgroundBrush);
            return TRUE;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        if (panel && panel->mBackgroundBrush)
        {
            HDC deviceContext = reinterpret_cast<HDC>(wParam);
            SetTextColor(deviceContext, TabTextColor);
            SetBkColor(deviceContext, BackgroundColor);
            return reinterpret_cast<LRESULT>(panel->mBackgroundBrush);
        }
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(window, SheetSubclassProc, subclassId);
        break;
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

LRESULT CALLBACK PropertyPanel::TabSubclassProc(HWND window, UINT message,
    WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR referenceData)
{
    PropertyPanel* panel = reinterpret_cast<PropertyPanel*>(referenceData);
    switch (message)
    {
    case WM_PAINT:
        if (panel)
        {
            PAINTSTRUCT paint = {};
            HDC deviceContext = BeginPaint(window, &paint);
            RECT clientRect = {};
            GetClientRect(window, &clientRect);
            FillRect(deviceContext, &clientRect, panel->mPageBrush);

            HPEN borderPen = CreatePen(PS_SOLID, 1, BorderColor);
            HGDIOBJ previousPen = SelectObject(deviceContext, borderPen);
            HGDIOBJ previousBrush = SelectObject(deviceContext, GetStockObject(NULL_BRUSH));
            Rectangle(deviceContext, clientRect.left, clientRect.top,
                clientRect.right, clientRect.bottom);
            SelectObject(deviceContext, previousBrush);
            SelectObject(deviceContext, previousPen);
            DeleteObject(borderPen);

            const int itemCount = TabCtrl_GetItemCount(window);
            for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex)
            {
                DRAWITEMSTRUCT drawItem = {};
                drawItem.CtlType = ODT_TAB;
                drawItem.itemID = static_cast<UINT>(itemIndex);
                drawItem.itemAction = ODA_DRAWENTIRE;
                drawItem.hwndItem = window;
                drawItem.hDC = deviceContext;
                TabCtrl_GetItemRect(window, itemIndex, &drawItem.rcItem);
                panel->DrawTab(drawItem);
            }

            EndPaint(window, &paint);
            return 0;
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_NCDESTROY:
        RemoveWindowSubclass(window, TabSubclassProc, subclassId);
        break;
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

void PropertyPanel::DrawTab(const DRAWITEMSTRUCT& drawItem) const
{
    const HWND tabControl = drawItem.hwndItem;
    wchar_t title[64] = {};
    TCITEMW item = {};
    item.mask = TCIF_TEXT;
    item.pszText = title;
    item.cchTextMax = static_cast<int>(std::size(title));
    TabCtrl_GetItem(tabControl, static_cast<int>(drawItem.itemID), &item);

    RECT itemRect = drawItem.rcItem;
    const bool selected = TabCtrl_GetCurSel(tabControl) == static_cast<int>(drawItem.itemID);
    HBRUSH itemBrush = CreateSolidBrush(selected ? SelectedTabColor : PageColor);
    FillRect(drawItem.hDC, &itemRect, itemBrush);
    DeleteObject(itemBrush);

    if (selected)
    {
        RECT accentRect = itemRect;
        accentRect.bottom = accentRect.top + 3;
        HBRUSH accentBrush = CreateSolidBrush(AccentColor);
        FillRect(drawItem.hDC, &accentRect, accentBrush);
        DeleteObject(accentBrush);
    }

    SetBkMode(drawItem.hDC, TRANSPARENT);
    SetTextColor(drawItem.hDC, selected ? TabTextColor : InactiveTabTextColor);
    DrawTextW(drawItem.hDC, title, -1, &itemRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

bool PropertyPanel::Create(HWND parentWindow, HINSTANCE instance)
{
    mBackgroundBrush = CreateSolidBrush(BackgroundColor);
    mPageBrush = CreateSolidBrush(PageColor);
    if (!mBackgroundBrush || !mPageBrush)
    {
        Destroy();
        return false;
    }

    const std::array<UINT, 3> pageResourceIds = {
        IDD_PROPERTY_GENERAL,
        IDD_PROPERTY_RENDERING,
        IDD_PROPERTY_TILE
    };
    std::array<HPROPSHEETPAGE, 3> pages = {};

    for (size_t index = 0; index < pages.size(); ++index)
    {
        PROPSHEETPAGEW page = {};
        page.dwSize = sizeof(page);
        page.hInstance = instance;
        page.pszTemplate = MAKEINTRESOURCEW(pageResourceIds[index]);
        page.pfnDlgProc = PropertyPageProc;
        page.lParam = reinterpret_cast<LPARAM>(this);
        pages[index] = CreatePropertySheetPageW(&page);
        if (!pages[index])
        {
            for (size_t createdIndex = 0; createdIndex < index; ++createdIndex)
            {
                DestroyPropertySheetPage(pages[createdIndex]);
            }
            Destroy();
            return false;
        }
    }

    PROPSHEETHEADERW sheet = {};
    sheet.dwSize = sizeof(sheet);
    sheet.dwFlags = PSH_MODELESS | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
    sheet.hwndParent = parentWindow;
    sheet.hInstance = instance;
    sheet.pszCaption = L"Properties";
    sheet.nPages = static_cast<UINT>(pages.size());
    sheet.phpage = pages.data();

    mWindow = reinterpret_cast<HWND>(PropertySheetW(&sheet));
    if (!mWindow || mWindow == reinterpret_cast<HWND>(-1))
    {
        mWindow = nullptr;
        Destroy();
        return false;
    }

    SetParent(mWindow, parentWindow);
    LONG_PTR style = GetWindowLongPtrW(mWindow, GWL_STYLE);
    style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME);
    style |= WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    SetWindowLongPtrW(mWindow, GWL_STYLE, style);
    LONG_PTR extendedStyle = GetWindowLongPtrW(mWindow, GWL_EXSTYLE);
    extendedStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE |
        WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
    SetWindowLongPtrW(mWindow, GWL_EXSTYLE, extendedStyle);
    SetWindowPos(mWindow, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    SetWindowSubclass(mWindow, SheetSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));

    const std::array<int, 3> buttonIds = { IDOK, IDCANCEL, IDHELP };
    for (const int buttonId : buttonIds)
    {
        const HWND button = GetDlgItem(mWindow, buttonId);
        if (button)
        {
            ShowWindow(button, SW_HIDE);
        }
    }

    const HWND tabControl = PropSheet_GetTabControl(mWindow);
    if (tabControl)
    {
        LONG_PTR tabStyle = GetWindowLongPtrW(tabControl, GWL_STYLE);
        tabStyle |= TCS_BOTTOM | TCS_OWNERDRAWFIXED | TCS_FIXEDWIDTH;
        SetWindowLongPtrW(tabControl, GWL_STYLE, tabStyle);
        TabCtrl_SetItemSize(tabControl, 72, 26);
        SetWindowSubclass(tabControl, TabSubclassProc, 2,
            reinterpret_cast<DWORD_PTR>(this));
        SetWindowPos(tabControl, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    return true;
}

void PropertyPanel::Destroy()
{
    if (mWindow)
    {
        DestroyWindow(mWindow);
        mWindow = nullptr;
    }
    if (mBackgroundBrush)
    {
        DeleteObject(mBackgroundBrush);
        mBackgroundBrush = nullptr;
    }
    if (mPageBrush)
    {
        DeleteObject(mPageBrush);
        mPageBrush = nullptr;
    }
}

void PropertyPanel::Show(bool visible)
{
    if (mWindow)
    {
        ShowWindow(mWindow, visible ? SW_SHOW : SW_HIDE);
    }
}

void PropertyPanel::Resize(int width, int height)
{
    if (mWindow)
    {
        SetWindowPos(mWindow, HWND_TOP, 0, 0, width, height, SWP_NOACTIVATE);
        ResizeContents();
    }
}

void PropertyPanel::ResizeContents()
{
    constexpr int margin = 4;
    const HWND tabControl = PropSheet_GetTabControl(mWindow);
    if (!tabControl)
    {
        return;
    }

    RECT clientRect = {};
    GetClientRect(mWindow, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    const int tabWidth = (std::max)(clientWidth - margin * 2, 0);
    const int tabHeight = (std::max)(clientHeight - margin * 2, 0);
    SetWindowPos(tabControl, HWND_TOP, margin, margin, tabWidth, tabHeight,
        SWP_NOACTIVATE);

    RECT pageRect = { 0, 0, tabWidth, tabHeight };
    TabCtrl_AdjustRect(tabControl, FALSE, &pageRect);
    MapWindowPoints(tabControl, mWindow,
        reinterpret_cast<POINT*>(&pageRect), 2);

    const HWND activePage = PropSheet_GetCurrentPageHwnd(mWindow);
    if (activePage)
    {
        const int pageWidth = (std::max)(pageRect.right - pageRect.left, 0L);
        const int pageHeight = (std::max)(pageRect.bottom - pageRect.top, 0L);
        SetWindowPos(activePage, HWND_TOP, pageRect.left, pageRect.top,
            pageWidth, pageHeight, SWP_NOACTIVATE);
    }

    RedrawWindow(mWindow, nullptr, nullptr,
        RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

bool PropertyPanel::HandleMessage(MSG* message) const
{
    return mWindow && PropSheet_IsDialogMessage(mWindow, message) != FALSE;
}
