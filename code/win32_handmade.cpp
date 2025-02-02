#include <windows.h>
#include <stdint.h>

#define file_scope static
#define local_persist static

file_scope BITMAPINFO gBitmapInfo;
file_scope LONG gBitmapWidth;
file_scope LONG gBitmapHeight;
file_scope WORD gBytesPerPixel = 4;
file_scope void *gBackBuffer;

file_scope void CreateBackBufferForNewSize(RECT *client_rect)
{
    // if back buffer got already allocated in the previous WM_SIZE call, release that memory
    if(gBackBuffer)
    {
        VirtualFree(gBackBuffer, 0, MEM_RELEASE);
    }

    gBitmapWidth = client_rect->right - client_rect->left;
    gBitmapHeight = client_rect->bottom - client_rect->top;

    // Allocate Device Independent Bitmap (DIB) parameters
    gBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    gBitmapInfo.bmiHeader.biWidth = gBitmapWidth;
    gBitmapInfo.bmiHeader.biHeight = -gBitmapHeight; // if biHeight is positive it's considered as bottom-up DIB, otherwise top-down DIB
    gBitmapInfo.bmiHeader.biPlanes = 1;
    gBitmapInfo.bmiHeader.biBitCount = gBytesPerPixel * 8; // 8 bits for each RGB and 8 bits for padding/alignment in memory
    gBitmapInfo.bmiHeader.biCompression = BI_RGB; // uncompressed RGB format
    // all others fields in the structure are zero, which is already done by virtue of the structure being global

    size_t bitmapSizeInBytes = gBitmapWidth * gBitmapHeight * gBytesPerPixel;
    // Allocate and commit a new back buffer, from the virtual pages for read and write
    gBackBuffer = VirtualAlloc(0, bitmapSizeInBytes, MEM_COMMIT, PAGE_READWRITE);

    uint8_t *pixel = (uint8_t *)gBackBuffer;
    for(LONG row = 0; row < gBitmapHeight; ++row)
    {
        for(LONG col = 0; col < gBitmapWidth; ++col)
        {
            // For each row and column write a 4 byte xRGB entry
            // Due to little endianness, 
            // Byte 0 = Blue,
            // Byte 1 = Green,
            // Byte 2 = Red,
            // Byte 3 = Padding
            // So when read as 4 bytes as mentioned in the BITMAPINFOHEADER it'll be read as <Padding><Red><Green><Blue> and the least 24 bits (RGB) will be used for the painting
            *pixel = 255;
            pixel++;

            *pixel = 0;
            pixel++;

            *pixel = 0;
            pixel++;

            *pixel = 0;
            pixel++;
        }
    }
}

file_scope void PaintWindowBasedOnCurrentBackBuffer(HDC windowDC, RECT *client_rect)
{
    int windowWidth = client_rect->right - client_rect->left;
    int windowHeight = client_rect->bottom - client_rect->top;
    StretchDIBits(windowDC,
                0, 0, windowWidth, windowHeight,        // Destination rectangle params
                0, 0, gBitmapWidth, gBitmapHeight,      // Source rectangle params
                gBackBuffer,        // Use this back buffer
                &gBitmapInfo,       // Use the structure defined in this structure
                DIB_RGB_COLORS,     // Plain RGB
                SRCCOPY);           // Copy from source to destination
}

LRESULT Wndproc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch(uMsg)
    {
        // When the size of the window is changed
        case WM_SIZE:
            {
                RECT client_rect;
                GetClientRect(hWnd, &client_rect);
                CreateBackBufferForNewSize(&client_rect);
            }
            break;

        // When painting request needs to be handled for the window
        case WM_PAINT:
            {
                PAINTSTRUCT paint_struct;
                HDC windowDC = BeginPaint(hWnd, &paint_struct);

                PaintWindowBasedOnCurrentBackBuffer(windowDC, &paint_struct.rcPaint);

                EndPaint(hWnd, &paint_struct);
            }
            break;

        case WM_DESTROY:
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
                                                                // CS_HREDRAW : Redraw the entire client rect area when the width changes
                                                                // CS_VREDRAW : Redraw the entire client rect area when the height changes
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
            while(GetMessage(&msg, 0, 0, 0) > 0)    // hWnd has to NULL so that all msg in the process is fetched (not window specific)
                                                    // > 0 since GetMessage can return -1
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            return 0;
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
