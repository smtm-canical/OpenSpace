#include "framework.h"
#include "CommandPopupController.h"

#include <filesystem>

namespace
{
    std::string GetPopupBackgroundPath()
    {
        wchar_t executablePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
        const std::filesystem::path executable(executablePath);
        const std::filesystem::path projectRoot = executable.parent_path().parent_path().parent_path().parent_path().parent_path();
        return (projectRoot / "data" / "images" / "popup_main.png").string();
    }
}

bool CommandPopupController::Initialize()
{
    std::string backgroundPath = GetPopupBackgroundPath();
    m_backgroundTexture = OS::LoadTexture(backgroundPath);
    m_isVisible = m_backgroundTexture.pixels != nullptr;
    return m_isVisible;
}

void CommandPopupController::Toggle()
{
    m_isVisible = !m_isVisible;
}

void CommandPopupController::Close()
{
    m_isVisible = false;
}

void CommandPopupController::Shutdown()
{
    m_isVisible = false;
    OS::UnloadTexture(m_backgroundTexture);
}

bool CommandPopupController::IsVisible() const
{
    return m_isVisible;
}

void CommandPopupController::Paint(ID2D1RenderTarget* renderTarget, float width, float height)
{
    if (!m_isVisible || renderTarget == nullptr)
        return;

    const float popupWidth = static_cast<float>(m_backgroundTexture.size.width);
    const float popupHeight = static_cast<float>(m_backgroundTexture.size.height);
    const float x = max(0.0f, (width - popupWidth) / 2.0f);
    const float y = max(0.0f, (height - popupHeight) / 2.0f);
    OS::DrawTexture(renderTarget, m_backgroundTexture, { x, y });
}
