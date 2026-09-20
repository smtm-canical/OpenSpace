#include "framework.h"
#include "GameLoop.h"
#include "Renderer.h"

#include <chrono>

namespace
{
    constexpr UINT TargetFramesPerSecond = 32;
    constexpr auto TargetFrameDuration =
        std::chrono::nanoseconds(1'000'000'000 / TargetFramesPerSecond);
}

GameLoop::~GameLoop()
{
    Stop();
}

bool GameLoop::Start(HWND window)
{
    RECT clientRect = {};
    GetClientRect(window, &clientRect);
    mClientWidth = static_cast<UINT>(clientRect.right - clientRect.left);
    mClientHeight = static_cast<UINT>(clientRect.bottom - clientRect.top);
    mStopRequested = false;

    try
    {
        mThread = std::thread([this, window]()
        {
            Run(window);
        });
    }
    catch (const std::system_error&)
    {
        return false;
    }

    return true;
}

void GameLoop::Stop()
{
    mStopRequested = true;
    if (mThread.joinable())
    {
        mThread.join();
    }
}

void GameLoop::Resize(UINT width, UINT height)
{
    mClientWidth = width;
    mClientHeight = height;
    mResizeRequested = true;
}

void GameLoop::ToggleMode()
{
    mEditorMode = !mEditorMode.load();
    mModeChangeRequested = true;
}

void GameLoop::SetLeftPanelWidth(UINT width)
{
    mLeftPanelWidth = width;
    mEditorLayoutChangeRequested = true;
}

void GameLoop::Run(HWND window)
{
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult))
    {
        PostMessageW(window, InitializationFailedMessage,
            static_cast<WPARAM>(comResult), 0);
        return;
    }

    Renderer renderer;
    const HRESULT initializationResult = renderer.Initialize(window);
    if (FAILED(initializationResult))
    {
        PostMessageW(window, InitializationFailedMessage,
            static_cast<WPARAM>(initializationResult), 0);
        renderer.Release();
        CoUninitialize();
        return;
    }

    auto nextFrameTime = std::chrono::steady_clock::now();
    while (!mStopRequested)
    {
        if (mModeChangeRequested.exchange(false))
        {
            renderer.SetEditorMode(mEditorMode.load());
        }


        if (mEditorLayoutChangeRequested.exchange(false))
        {
            renderer.SetLeftPanelWidth(mLeftPanelWidth.load());
        }

        if (mResizeRequested.exchange(false))
        {
            renderer.Resize(mClientWidth, mClientHeight);
        }

        renderer.Render();

        nextFrameTime += TargetFrameDuration;
        const auto currentTime = std::chrono::steady_clock::now();
        if (nextFrameTime > currentTime)
        {
            std::this_thread::sleep_until(nextFrameTime);
        }
        else
        {
            nextFrameTime = currentTime;
        }
    }

    renderer.Release();
    CoUninitialize();
}
