#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <chrono>
#include <filesystem>
#include <vector>

class Renderer final
{
public:
    HRESULT Initialize(HWND window);
    void Resize(UINT width, UINT height);
    void SetEditorMode(bool editorMode);
    void SetLeftPanelWidth(UINT width);
    void Render();
    void Release();

private:
    HRESULT CreateDeviceResources();
    HRESULT LoadImages();
    HRESULT LoadBitmap(const std::filesystem::path& imagePath, ID2D1Bitmap** bitmap);
    void GenerateRandomTiles(UINT width, UINT height);
    void UpdateFramesPerSecond();
    D2D1_RECT_F GetGameViewport() const;

    HWND mWindow = nullptr;
    Microsoft::WRL::ComPtr<ID2D1Factory> mD2DFactory;
    Microsoft::WRL::ComPtr<IDWriteFactory> mDWriteFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> mFpsTextFormat;
    Microsoft::WRL::ComPtr<IWICImagingFactory> mWICFactory;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> mRenderTarget;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> mFpsTextBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> mEditorPanelBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> mTileOutlineBrush;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> mBackgroundBitmap;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> mTerrainAtlasBitmap;
    std::vector<UINT> mTileIndices;
    UINT mTileColumnCount = 0;
    UINT mTileRowCount = 0;
    bool mEditorMode = false;
    UINT mLeftPanelWidth = 0;
    std::chrono::steady_clock::time_point mFpsMeasurementStart;
    UINT mRenderedFrameCount = 0;
    wchar_t mFpsText[32] = L"FPS: 0.00";
};
