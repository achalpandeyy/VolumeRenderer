#include "Core/Win32.h"
#include "Core/Types.h"

#include <cstdlib>

// TODO(achal): I would like to pop up OS MessageBox when something fails (window registration/creation)
// instead of debug output.

LRESULT CALLBACK Win32WindowCallback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    // NOTE(achal): You can get your stuff here by retrieving a pointer from HWND. 

    switch (message)
    {
        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;
    }

    return DefWindowProcA(window, message, w_param, l_param);
}

HWND Win32CreateWindow(int width, int height, const char* name, HINSTANCE instance)
{
    WNDCLASSA window_class = {};
    window_class.style = CS_CLASSDC;
    window_class.lpfnWndProc = Win32WindowCallback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "Volume Renderer Window Class";

    if (!RegisterClassA(&window_class))
    {
        OutputDebugStringA("Error: Failed to register window class!\n");
        exit(1);
    }

    return CreateWindowExA(0, window_class.lpszClassName, name, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, instance, 0);
}

b32 Win32WindowShouldQuit()
{
    MSG message;
    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
            return true;

        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return false;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, INT show_code)
{
    int width = 768;
    int height = 576;

    HWND window = Win32CreateWindow(width, height, "Volume Renderer", instance);
    if (!window)
    {
        OutputDebugStringA("Error: Failed to create window!\n");
        exit(1);
    }
    
    while (!Win32WindowShouldQuit())
    {

    }

    return 0;
}