#pragma once

#include <d2d1.h>

#include "Sprite2D.h"

// Draws the command panel as an overlay in the main client render target.
class CommandPopupController
{
public:
    bool Initialize();
    void Toggle();
    void Close();
    void Shutdown();
    bool IsVisible() const;
    void Paint(ID2D1RenderTarget* renderTarget, float width, float height);

private:
    bool m_isVisible = false;
    OS::Texture::Data m_backgroundTexture{};
};
