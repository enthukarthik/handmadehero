#include <windows.h>
#include <stdint.h>

#define file_scope static
#define local_persist static

file_scope BITMAPINFO gBitmapInfo;

file_scope int32_t gBitmapWidth;
file_scope int32_t gBitmapHeight;
file_scope int16_t gBytesPerPixel = 4;

file_scope bool    gGameRunning   = true;
file_scope void *gBackBuffer;

file_scope void RenderColorGradient(int32_t xOffset, int32_t yOffset)
{
    // Pitch : No. of pixels to move to get from one row beginning to another beginning
    // Stride : No of pixels to move to get from one row end to another row beginning
    // Casey Muratori says that for pixel operations sometimes strides are not aligned properly at pixel boundaries
    // So do the op for each row and increase the row pixel by pitch amount is the right thing to do. TODO : Need to understand it better

    uint8_t *each_row = (uint8_t *)gBackBuffer;
    int32_t  pitch    = gBitmapWidth * gBytesPerPixel;

    for(int32_t row = 0; row < gBitmapHeight; ++row)
    {
        uint32_t *pixel = (uint32_t *)each_row;
        for(int32_t col = 0; col < gBitmapWidth; ++col)
        {
            // For each row and column write a 4 byte xRGB entry
            // Due to little endianness, 
            // Byte 0 = Blue,
            // Byte 1 = Green,
            // Byte 2 = Red,
            // Byte 3 = Padding
            // So when read as 4 bytes as mentioned in the BITMAPINFOHEADER it'll be read as <Padding><Red><Green><Blue> and the least 24 bits (RGB) will be used for the paint32_ting

            uint8_t blue  = (uint8_t)col + xOffset; // Take the lower order byte from col. So the blue color gradually increases from 0 to 256 sideward and suddenly drops to black. Adding xOffset to animate
            uint8_t red   = (uint8_t)row + yOffset; // Take the lower order byte from row. So the red color gradually increases from 0 to 256 downward and suddenly drops to black. Add yOffset to animate
            uint8_t green = blue ^ red;

            const uint8_t red_offset   = 16;
            const uint8_t green_offset = 8;
            const uint8_t blue_offset  = 0;

            *pixel++ = (uint32_t)((red << red_offset) | (green << green_offset) | (blue << blue_offset));
        }

        each_row += pitch;
    }    
}

file_scope void CreateBackBufferForNewSize(RECT *client_rect)
{
    // if back buffer got already allocated in the previous WM_SIZE call, release that memory
    if(gBackBuffer)
    {
        VirtualFree(gBackBuffer, 0, MEM_RELEASE);
    }

    gBitmapWidth  = client_rect->right - client_rect->left;
    gBitmapHeight = client_rect->bottom - client_rect->top;

    // Allocate Device Independent Bitmap (DIB) parameters
    gBitmapInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    gBitmapInfo.bmiHeader.biWidth       = gBitmapWidth;
    gBitmapInfo.bmiHeader.biHeight      = -gBitmapHeight; // if biHeight is positive it's considered as bottom-up DIB, otherwise top-down DIB
    gBitmapInfo.bmiHeader.biPlanes      = 1;
    gBitmapInfo.bmiHeader.biBitCount    = gBytesPerPixel * 8; // 8 bits for each RGB and 8 bits for padding/alignment in memory
    gBitmapInfo.bmiHeader.biCompression = BI_RGB; // uncompressed RGB format
    // all others fields in the structure are zero, which is already done by virtue of the structure being global

    size_t bitmapSizeInBytes = gBitmapWidth * gBitmapHeight * gBytesPerPixel;
    // Allocate and commit a new back buffer, from the virtual pages for read and write
    gBackBuffer = VirtualAlloc(0, bitmapSizeInBytes, MEM_COMMIT, PAGE_READWRITE);
}

file_scope void PaintWindowFromCurrentBackBuffer(HDC windowDC, RECT *client_rect)
{
    int32_t windowWidth  = client_rect->right - client_rect->left;
    int32_t windowHeight = client_rect->bottom - client_rect->top;

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
    LRESULT result = 0;
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

                PaintWindowFromCurrentBackBuffer(windowDC, &paint_struct.rcPaint);

                EndPaint(hWnd, &paint_struct);
            }
            break;

        case WM_CLOSE:
            gGameRunning = false;                   // User pressed the close button. Give some UI popup and close the game gracefully
            break;

        case WM_DESTROY:
            gGameRunning = false;                   // We shouldn't be getting this without our knowledge, if so we should close and recrete the game window gracefully. How to handle it?
            break;

        default:
            result = DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;
    }

    return result;
}

int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    WNDCLASS wndClass {};
    // wndClass.style       = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;  // CS_OWNDC : Allocate own DC for every window created through this class
    //                                                             // CS_HREDRAW : Redraw the entire client rect area when the width changes
    //                                                             // CS_VREDRAW : Redraw the entire client rect area when the height changes
    wndClass.lpfnWndProc   = Wndproc;                             // The windows proc to be called
    wndClass.hInstance     = hInstance;                           // hInstance of the module that contains the WndProc. GetModuleHandle(0) should do the same
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
            int32_t xOffset = 0;
            int32_t yOffset = 0;

            gGameRunning = true;
            
            while (gGameRunning)
            {
                MSG msg;
                // PeekMessage won't block on the thread if no msg in the queue
                if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
                {
                    if(msg.message == WM_QUIT) // If we get this message through some other means, exit the game
                    {
                        gGameRunning = false;
                    }
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);

                RenderColorGradient(xOffset, yOffset);

                RECT client_rect;
                GetClientRect(hhWindow, &client_rect);

                HDC windowDeviceContext = GetDC(hhWindow);
                PaintWindowFromCurrentBackBuffer(windowDeviceContext, &client_rect);
                ReleaseDC(hhWindow, windowDeviceContext);

                ++xOffset;
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
