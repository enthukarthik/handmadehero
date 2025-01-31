#include <windows.h>

LRESULT Wndproc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch(uMsg)
    {
        case WM_CREATE:
            OutputDebugString(TEXT("Window created\n"));
            break;

        case WM_CLOSE:
            // This will add WM_DESTROY to the queue
            OutputDebugString(TEXT("Close button is pressed\n"));
            break;

        // WM_DESTROY msg gets passed when the close button is pressed
        case WM_DESTROY:
            OutputDebugString(TEXT("WM_DESTROY msg processed\n"));
            PostQuitMessage(0);                         // This will post WM_QUIT msg which will exit the event loop
            break;

        default:
            break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    WNDCLASS wndClass {};
    wndClass.style       = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;  // CS_OWNDC : Allocate own DC for every window created through this class
    wndClass.lpfnWndProc = Wndproc;                             // The windows proc to be called
    wndClass.hInstance   = hInstance;                           // hInstance of the module that contains the WndProc. GetModuleHandle(0) should do the same
    wndClass.lpszClassName = TEXT("HHWndClass");                // A name for this class. TEXT() to make it work for both ASCII and Unicode

    if(RegisterClass(&wndClass))
    {
        HWND hhWindow = CreateWindow(
                            wndClass.lpszClassName,
                            TEXT("HH First Window"),
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            0,
                            0,
                            hInstance,
                            0);
        
        if(hhWindow)
        {
            MSG msg;
            while(GetMessage(&msg, 0, 0, 0))    // hWnd has to NULL so that all msg in the process is fetched (not window specific)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            return msg.wParam;
        }
        else
        {
            return -2;
        }
    }
    else
    {
        return -1;
    }
}
