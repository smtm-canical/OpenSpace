#pragma once

class SpriteToolWindow
{
public:
	bool Create(HINSTANCE instance, int commandShow);

private:
	static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	void Draw(HDC deviceContext) const;
	bool IsOverSplitter(int x, int y) const;

	HINSTANCE m_instance = nullptr;
	HWND m_window = nullptr;
	int m_splitterPosition = 280;
	bool m_isDraggingSplitter = false;
};
