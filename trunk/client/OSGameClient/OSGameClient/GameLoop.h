#pragma once

#include <atomic>
#include <thread>

class GameLoop final
{
public:
    static constexpr UINT InitializationFailedMessage = WM_APP + 1;

    ~GameLoop();

    bool Start(HWND window);
    void Stop();
    void Resize(UINT width, UINT height);
    void ToggleMode();
    void SetLeftPanelWidth(UINT width);

private:
    void Run(HWND window);

    std::thread mThread;
    std::atomic_bool mStopRequested = false;
    std::atomic_bool mResizeRequested = false;
    std::atomic_bool mEditorMode = false;
    std::atomic_bool mModeChangeRequested = false;
    std::atomic_bool mEditorLayoutChangeRequested = false;
    std::atomic_uint mLeftPanelWidth = 0;
    std::atomic_uint mClientWidth = 0;
    std::atomic_uint mClientHeight = 0;
};
