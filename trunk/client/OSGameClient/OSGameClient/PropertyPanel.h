#pragma once

#include <commctrl.h>

class PropertyPanel final
{
public:
    bool Create(HWND parentWindow, HINSTANCE instance);
    void Destroy();
    void Show(bool visible);
    void Resize(int width, int height);
    bool HandleMessage(MSG* message) const;

private:
    static INT_PTR CALLBACK PropertyPageProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK SheetSubclassProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR referenceData);
    static LRESULT CALLBACK TabSubclassProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR referenceData);
    void DrawTab(const DRAWITEMSTRUCT& drawItem) const;
    void ResizeContents();

    HWND mWindow = nullptr;
    HBRUSH mBackgroundBrush = nullptr;
    HBRUSH mPageBrush = nullptr;
};
