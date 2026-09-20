#include "framework.h"
#include "SpriteToolWindow.h"

#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int commandShow)
{
	INITCOMMONCONTROLSEX commonControls{};
	commonControls.dwSize = sizeof(commonControls);
	commonControls.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx(&commonControls);

	SpriteToolWindow application;
	if (!application.Create(instance, commandShow))
	{
		return FALSE;
	}

	MSG message{};
	while (GetMessageW(&message, nullptr, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}

	return static_cast<int>(message.wParam);
}
