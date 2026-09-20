#include "framework.h"
#include "EditorLayout.h"
#include "Renderer.h"

#include <algorithm>
#include <random>

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr wchar_t BackgroundImageRelativePath[] = L"GameData\\image\\default.jpg";
    constexpr wchar_t TerrainAtlasRelativePath[] = L"GameData\\image\\map\\terrain_atlas_32x32.png";
    constexpr D2D1_COLOR_F BackgroundColor = { 0.02f, 0.02f, 0.02f, 1.0f };
    constexpr D2D1_COLOR_F FpsTextColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    constexpr D2D1_COLOR_F EditorPanelColor = { 0.10f, 0.11f, 0.13f, 1.0f };
    constexpr D2D1_COLOR_F TileOutlineColor = { 0.0f, 1.0f, 0.0f, 1.0f };
    constexpr UINT TileSize = 32;
    constexpr float TileOutlineWidth = 1.0f;
    constexpr float FpsTextSize = 22.0f;
    constexpr float FpsTextMargin = 10.0f;
    constexpr float FpsTextWidth = 180.0f;
    constexpr float FpsTextHeight = 40.0f;

    std::filesystem::path GetExecutableDirectory()
    {
        std::wstring executablePath(MAX_PATH, L'\0');
        const DWORD length = GetModuleFileNameW(nullptr, executablePath.data(),
            static_cast<DWORD>(executablePath.size()));
        executablePath.resize(length);
        return std::filesystem::path(executablePath).parent_path();
    }
}

HRESULT Renderer::Initialize(HWND window)
{
    mWindow = window;
    HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
        IID_PPV_ARGS(mD2DFactory.ReleaseAndGetAddressOf()));

    if (SUCCEEDED(result))
    {
        result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(mDWriteFactory.ReleaseAndGetAddressOf()));
    }

    if (SUCCEEDED(result))
    {
        result = mDWriteFactory->CreateTextFormat(L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            FpsTextSize, L"ko-kr", mFpsTextFormat.ReleaseAndGetAddressOf());
    }

    if (SUCCEEDED(result))
    {
        result = mFpsTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    }

    if (SUCCEEDED(result))
    {
        result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(mWICFactory.ReleaseAndGetAddressOf()));
    }

    if (SUCCEEDED(result))
    {
        result = CreateDeviceResources();
    }

    mFpsMeasurementStart = std::chrono::steady_clock::now();

    return result;
}

void Renderer::Resize(UINT width, UINT height)
{
    if (mRenderTarget)
    {
        mRenderTarget->Resize(D2D1::SizeU(width, height));
    }

    const D2D1_RECT_F gameViewport = GetGameViewport();
    GenerateRandomTiles(
        static_cast<UINT>(gameViewport.right - gameViewport.left),
        static_cast<UINT>(gameViewport.bottom - gameViewport.top));
}

void Renderer::SetEditorMode(bool editorMode)
{
    if (mEditorMode == editorMode)
    {
        return;
    }

    mEditorMode = editorMode;
    const D2D1_RECT_F gameViewport = GetGameViewport();
    GenerateRandomTiles(
        static_cast<UINT>(gameViewport.right - gameViewport.left),
        static_cast<UINT>(gameViewport.bottom - gameViewport.top));
}

void Renderer::SetLeftPanelWidth(UINT width)
{
    if (mLeftPanelWidth == width)
    {
        return;
    }

    mLeftPanelWidth = width;
    if (mEditorMode)
    {
        const D2D1_RECT_F gameViewport = GetGameViewport();
        GenerateRandomTiles(
            static_cast<UINT>(gameViewport.right - gameViewport.left),
            static_cast<UINT>(gameViewport.bottom - gameViewport.top));
    }
}

void Renderer::Render()
{
    if (FAILED(CreateDeviceResources()) || !mBackgroundBitmap || !mTerrainAtlasBitmap)
    {
        return;
    }

    const D2D1_SIZE_F targetSize = mRenderTarget->GetSize();
    const D2D1_RECT_F gameViewport = GetGameViewport();
    const float gameWidth = gameViewport.right - gameViewport.left;
    const float gameHeight = gameViewport.bottom - gameViewport.top;
    const D2D1_SIZE_F imageSize = mBackgroundBitmap->GetSize();
    const float left = gameViewport.left + (gameWidth - imageSize.width) * 0.5f;
    const float top = gameViewport.top + (gameHeight - imageSize.height) * 0.5f;

    mRenderTarget->BeginDraw();
    mRenderTarget->Clear(BackgroundColor);
    if (mEditorMode)
    {
        const float verticalSplitterLeft = std::min(
            static_cast<float>(mLeftPanelWidth), targetSize.width);
        const float horizontalSplitterTop = targetSize.height * EditorLayout::GamePanelHeightRatio;
        mRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f,
            verticalSplitterLeft, targetSize.height), mEditorPanelBrush.Get());
        mRenderTarget->FillRectangle(D2D1::RectF(gameViewport.left,
            horizontalSplitterTop, targetSize.width, targetSize.height),
            mEditorPanelBrush.Get());
    }

    mRenderTarget->PushAxisAlignedClip(gameViewport, D2D1_ANTIALIAS_MODE_ALIASED);
    mRenderTarget->DrawBitmap(mBackgroundBitmap.Get(),
        D2D1::RectF(left, top, left + imageSize.width, top + imageSize.height),
        1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

    const D2D1_SIZE_F atlasSize = mTerrainAtlasBitmap->GetSize();
    const UINT atlasColumnCount = static_cast<UINT>(atlasSize.width) / TileSize;
    for (UINT row = 0; row < mTileRowCount; ++row)
    {
        for (UINT column = 0; column < mTileColumnCount; ++column)
        {
            const UINT tileIndex = mTileIndices[row * mTileColumnCount + column];
            const UINT sourceColumn = tileIndex % atlasColumnCount;
            const UINT sourceRow = tileIndex / atlasColumnCount;
            const D2D1_RECT_F destination = D2D1::RectF(
                gameViewport.left + static_cast<float>(column * TileSize),
                gameViewport.top + static_cast<float>(row * TileSize),
                gameViewport.left + static_cast<float>((column + 1) * TileSize),
                gameViewport.top + static_cast<float>((row + 1) * TileSize));
            const D2D1_RECT_F source = D2D1::RectF(
                static_cast<float>(sourceColumn * TileSize),
                static_cast<float>(sourceRow * TileSize),
                static_cast<float>((sourceColumn + 1) * TileSize),
                static_cast<float>((sourceRow + 1) * TileSize));

            mRenderTarget->DrawBitmap(mTerrainAtlasBitmap.Get(), destination, 1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, source);
        }
    }

    const float gridRight = gameViewport.left + static_cast<float>(mTileColumnCount * TileSize);
    const float gridBottom = gameViewport.top + static_cast<float>(mTileRowCount * TileSize);
    for (UINT column = 0; column <= mTileColumnCount; ++column)
    {
        const float x = gameViewport.left + static_cast<float>(column * TileSize) + 0.5f;
        mRenderTarget->DrawLine(D2D1::Point2F(x, gameViewport.top),
            D2D1::Point2F(x, gridBottom), mTileOutlineBrush.Get(), TileOutlineWidth);
    }

    for (UINT row = 0; row <= mTileRowCount; ++row)
    {
        const float y = gameViewport.top + static_cast<float>(row * TileSize) + 0.5f;
        mRenderTarget->DrawLine(D2D1::Point2F(gameViewport.left, y),
            D2D1::Point2F(gridRight, y), mTileOutlineBrush.Get(), TileOutlineWidth);
    }
    mRenderTarget->PopAxisAlignedClip();

    const float fpsTextLeft = targetSize.width - FpsTextMargin - FpsTextWidth;
    const D2D1_RECT_F fpsTextArea = D2D1::RectF(fpsTextLeft, FpsTextMargin,
        fpsTextLeft + FpsTextWidth, FpsTextMargin + FpsTextHeight);
    mRenderTarget->DrawTextW(mFpsText, static_cast<UINT32>(wcslen(mFpsText)),
        mFpsTextFormat.Get(), fpsTextArea, mFpsTextBrush.Get());

    const HRESULT drawResult = mRenderTarget->EndDraw();
    if (drawResult == D2DERR_RECREATE_TARGET)
    {
        mBackgroundBitmap.Reset();
        mTerrainAtlasBitmap.Reset();
        mFpsTextBrush.Reset();
        mEditorPanelBrush.Reset();
        mTileOutlineBrush.Reset();
        mRenderTarget.Reset();
        InvalidateRect(mWindow, nullptr, FALSE);
    }
    else if (SUCCEEDED(drawResult))
    {
        UpdateFramesPerSecond();
    }
}

void Renderer::Release()
{
    mBackgroundBitmap.Reset();
    mTerrainAtlasBitmap.Reset();
    mFpsTextBrush.Reset();
    mEditorPanelBrush.Reset();
    mTileOutlineBrush.Reset();
    mRenderTarget.Reset();
    mWICFactory.Reset();
    mFpsTextFormat.Reset();
    mDWriteFactory.Reset();
    mD2DFactory.Reset();
    mWindow = nullptr;
}

HRESULT Renderer::CreateDeviceResources()
{
    if (mRenderTarget)
    {
        return S_OK;
    }

    RECT clientRect = {};
    GetClientRect(mWindow, &clientRect);
    const D2D1_SIZE_U size = D2D1::SizeU(
        static_cast<UINT32>(clientRect.right - clientRect.left),
        static_cast<UINT32>(clientRect.bottom - clientRect.top));

    HRESULT result = mD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(mWindow, size, D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        mRenderTarget.ReleaseAndGetAddressOf());

    if (SUCCEEDED(result))
    {
        result = mRenderTarget->CreateSolidColorBrush(FpsTextColor,
            mFpsTextBrush.ReleaseAndGetAddressOf());
    }

    if (SUCCEEDED(result))
    {
        result = mRenderTarget->CreateSolidColorBrush(EditorPanelColor,
            mEditorPanelBrush.ReleaseAndGetAddressOf());
    }

    if (SUCCEEDED(result))
    {
        result = mRenderTarget->CreateSolidColorBrush(TileOutlineColor,
            mTileOutlineBrush.ReleaseAndGetAddressOf());
    }

    if (SUCCEEDED(result))
    {
        result = LoadImages();
    }

    return result;
}

HRESULT Renderer::LoadImages()
{
    const std::filesystem::path executableDirectory = GetExecutableDirectory();
    HRESULT result = LoadBitmap(executableDirectory / BackgroundImageRelativePath,
        mBackgroundBitmap.ReleaseAndGetAddressOf());

    if (SUCCEEDED(result))
    {
        result = LoadBitmap(executableDirectory / TerrainAtlasRelativePath,
            mTerrainAtlasBitmap.ReleaseAndGetAddressOf());
    }

    if (SUCCEEDED(result))
    {
        const D2D1_RECT_F gameViewport = GetGameViewport();
        GenerateRandomTiles(
            static_cast<UINT>(gameViewport.right - gameViewport.left),
            static_cast<UINT>(gameViewport.bottom - gameViewport.top));
    }

    return result;
}

HRESULT Renderer::LoadBitmap(const std::filesystem::path& imagePath, ID2D1Bitmap** bitmap)
{
    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT result = mWICFactory->CreateDecoderFromFilename(imagePath.c_str(), nullptr,
        GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf());

    ComPtr<IWICBitmapFrameDecode> frame;
    if (SUCCEEDED(result))
    {
        result = decoder->GetFrame(0, frame.GetAddressOf());
    }

    ComPtr<IWICFormatConverter> converter;
    if (SUCCEEDED(result))
    {
        result = mWICFactory->CreateFormatConverter(converter.GetAddressOf());
    }

    if (SUCCEEDED(result))
    {
        result = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
    }

    if (SUCCEEDED(result))
    {
        result = mRenderTarget->CreateBitmapFromWicBitmap(converter.Get(), nullptr,
            bitmap);
    }

    return result;
}

void Renderer::GenerateRandomTiles(UINT width, UINT height)
{
    if (!mTerrainAtlasBitmap)
    {
        return;
    }

    const UINT requiredColumnCount = (width + TileSize - 1) / TileSize;
    const UINT requiredRowCount = (height + TileSize - 1) / TileSize;
    if (requiredColumnCount <= mTileColumnCount && requiredRowCount <= mTileRowCount)
    {
        return;
    }

    const UINT tileColumnCount = (std::max)(requiredColumnCount, mTileColumnCount);
    const UINT tileRowCount = (std::max)(requiredRowCount, mTileRowCount);

    const D2D1_SIZE_F atlasSize = mTerrainAtlasBitmap->GetSize();
    const UINT atlasColumnCount = static_cast<UINT>(atlasSize.width) / TileSize;
    const UINT atlasRowCount = static_cast<UINT>(atlasSize.height) / TileSize;
    const UINT atlasTileCount = atlasColumnCount * atlasRowCount;

    std::mt19937 randomEngine(std::random_device{}());
    std::uniform_int_distribution<UINT> tileDistribution(0, atlasTileCount - 1);

    std::vector<UINT> expandedTileIndices(tileColumnCount * tileRowCount);
    for (UINT row = 0; row < tileRowCount; ++row)
    {
        for (UINT column = 0; column < tileColumnCount; ++column)
        {
            UINT& tileIndex = expandedTileIndices[row * tileColumnCount + column];
            if (row < mTileRowCount && column < mTileColumnCount)
            {
                tileIndex = mTileIndices[row * mTileColumnCount + column];
            }
            else
            {
                tileIndex = tileDistribution(randomEngine);
            }
        }
    }

    mTileIndices = std::move(expandedTileIndices);
    mTileColumnCount = tileColumnCount;
    mTileRowCount = tileRowCount;
}

void Renderer::UpdateFramesPerSecond()
{
    ++mRenderedFrameCount;
    const auto currentTime = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsedTime = currentTime - mFpsMeasurementStart;
    if (elapsedTime.count() < 1.0)
    {
        return;
    }

    const double framesPerSecond = static_cast<double>(mRenderedFrameCount) / elapsedTime.count();
    swprintf_s(mFpsText, L"FPS: %.2f", framesPerSecond);
    mRenderedFrameCount = 0;
    mFpsMeasurementStart = currentTime;
}

D2D1_RECT_F Renderer::GetGameViewport() const
{
    const D2D1_SIZE_F targetSize = mRenderTarget->GetSize();
    if (!mEditorMode)
    {
        return D2D1::RectF(0.0f, 0.0f, targetSize.width, targetSize.height);
    }

    const float verticalSplitterLeft = std::min(
        static_cast<float>(mLeftPanelWidth), targetSize.width);
    const float horizontalSplitterTop = targetSize.height * EditorLayout::GamePanelHeightRatio;
    const float gameViewportLeft = verticalSplitterLeft;
    return D2D1::RectF(gameViewportLeft, 0.0f,
        targetSize.width, horizontalSplitterTop);
}
