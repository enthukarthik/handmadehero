#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <math.h>

#define file_scope static
#define local_persist static

// Device Independent Bitmap Back Buffer
typedef struct DIBBackBuffer
{
    BITMAPINFO bbStructure;
    void *bbMemory;
    int32_t bbWidth;
    int32_t bbHeight;
} BackBuffer;

file_scope BackBuffer g_hhBackBuffer;

file_scope bool       g_GameRunning   = true;

file_scope void CalcWidthHeightFromRect(RECT *client_rect, int *width, int *height)
{
    *width  = client_rect->right - client_rect->left;
    *height = client_rect->bottom - client_rect->top;
}

file_scope void RenderColorGradient(int32_t xOffset, int32_t yOffset)
{
    // Pitch : No. of pixels to move to get from one row beginning to another beginning
    // Stride : No of pixels to move to get from one row end to another row beginning
    // Casey Muratori says that for pixel operations sometimes strides are not aligned properly at pixel boundaries
    // So do the op for each row and increase the row pixel by pitch amount is the right thing to do. TODO : Need to understand it better

    uint8_t *each_row = (uint8_t *)g_hhBackBuffer.bbMemory;
    int32_t  pitch    = g_hhBackBuffer.bbWidth * g_hhBackBuffer.bbStructure.bmiHeader.biBitCount / 8;

    for(int32_t row = 0; row < g_hhBackBuffer.bbHeight; ++row)
    {
        uint32_t *pixel = (uint32_t *)each_row;
        for(int32_t col = 0; col < g_hhBackBuffer.bbWidth; ++col)
        {
            // For each row and column write a 4 byte xRGB entry
            // Due to little endianness, 
            // Byte 0 = Blue,
            // Byte 1 = Green,
            // Byte 2 = Red,
            // Byte 3 = Padding
            // So when read as 4 bytes as mentioned in the BITMAPINFOHEADER it'll be read as <Padding><Red><Green><Blue> and the least 24 bits (RGB) will be used for the paint32_ting

            uint8_t blue  = (uint8_t)(col + xOffset); // Take the lower order byte from col. So the blue color gradually increases from 0 to 256 sideward and suddenly drops to black. Adding xOffset to animate
            uint8_t red   = (uint8_t)(row + yOffset); // Take the lower order byte from row. So the red color gradually increases from 0 to 256 downward and suddenly drops to black. Add yOffset to animate
            // Trying some random things to the green channel
            uint8_t green = blue ^ red;
            green = green + green;

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
    if(g_hhBackBuffer.bbMemory)
    {
        VirtualFree(g_hhBackBuffer.bbMemory, 0, MEM_RELEASE);
    }

    CalcWidthHeightFromRect(client_rect, &g_hhBackBuffer.bbWidth, &g_hhBackBuffer.bbHeight);

    // Allocate Device Independent Bitmap (DIB) parameters
    g_hhBackBuffer.bbStructure.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    g_hhBackBuffer.bbStructure.bmiHeader.biWidth       = g_hhBackBuffer.bbWidth;
    g_hhBackBuffer.bbStructure.bmiHeader.biHeight      = -g_hhBackBuffer.bbHeight; // if biHeight is positive it's considered as bottom-up DIB, otherwise top-down DIB
    g_hhBackBuffer.bbStructure.bmiHeader.biPlanes      = 1;
    g_hhBackBuffer.bbStructure.bmiHeader.biBitCount    = 32; // 8 bits for each RGB and 8 bits for padding/alignment in memory
    g_hhBackBuffer.bbStructure.bmiHeader.biCompression = BI_RGB; // uncompressed RGB format
    // all others fields in the structure are zero, which is already done by virtue of the structure being global

    size_t bitmapSizeInBytes = g_hhBackBuffer.bbWidth * g_hhBackBuffer.bbHeight * g_hhBackBuffer.bbStructure.bmiHeader.biBitCount / 8;

    // Allocate and commit a new back buffer, from the virtual pages for read and write
    g_hhBackBuffer.bbMemory = VirtualAlloc(0, bitmapSizeInBytes, MEM_COMMIT, PAGE_READWRITE);
}

file_scope void PaintWindowFromCurrentBackBuffer(HDC windowDC, RECT *client_rect)
{
    int32_t windowWidth;
    int32_t windowHeight;
    CalcWidthHeightFromRect(client_rect, &windowWidth, &windowHeight);

    StretchDIBits(windowDC,
                0, 0, windowWidth, windowHeight,        // Destination rectangle params
                0, 0, g_hhBackBuffer.bbWidth, g_hhBackBuffer.bbHeight,      // Source rectangle params
                g_hhBackBuffer.bbMemory,        // Use this back buffer
                &g_hhBackBuffer.bbStructure,       // Use the structure defined in this header
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
            g_GameRunning = false;                   // User pressed the close button. Give some UI popup and close the game gracefully
            break;

        case WM_DESTROY:
            g_GameRunning = false;                   // We shouldn't be getting this without our knowledge, if so we should close and recrete the game window gracefully. How to handle it?
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
    wndClass.style       = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;  // CS_OWNDC : Allocate own DC for every window created through this class
                                                                // CS_HREDRAW : Redraw the entire client rect area when the width changes
                                                                // CS_VREDRAW : Redraw the entire client rect area when the height changes
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
            HDC windowDeviceContext = GetDC(hhWindow);
            RECT client_rect;
            DWORD xinputResult;

            GetClientRect(hhWindow, &client_rect);
            CreateBackBufferForNewSize(&client_rect);       // Create a fixed size back buffer immediately after windows intialization for the default size

            int32_t xOffset = 0;
            int32_t yOffset = 0;

            g_GameRunning = true;

            while (g_GameRunning)
            {
                MSG msg;
                // PeekMessage won't block on the thread if no msg in the queue
                if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
                {
                    if(msg.message == WM_QUIT) // If we get this message through some other means, exit the game
                    {
                        g_GameRunning = false;
                    }
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);

                for(int32_t controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; ++controllerIndex)
                {
                    XINPUT_STATE inputState;
                    ZeroMemory(&inputState, sizeof(XINPUT_STATE));

                    xinputResult = XInputGetState(controllerIndex, &inputState);

                    if(xinputResult == ERROR_SUCCESS) // Controller is connected
                    {
                        bool dPadUp      = inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool dPadDown    = inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool dPadLeft    = inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool dPadRight   = inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool buttonStart = inputState.Gamepad.wButtons & XINPUT_GAMEPAD_START;
                        bool buttonBack  = inputState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;
                        bool buttonA     = inputState.Gamepad.wButtons & XINPUT_GAMEPAD_A;
                        bool buttonB     = inputState.Gamepad.wButtons & XINPUT_GAMEPAD_B;
                        bool buttonX     = inputState.Gamepad.wButtons & XINPUT_GAMEPAD_X;
                        bool buttonY     = inputState.Gamepad.wButtons & XINPUT_GAMEPAD_Y;

                        // Move the offsets to animate the buffer
                        if(dPadUp)
                            yOffset += 2;

                        if(dPadDown)
                            yOffset -= 2;

                        if(dPadLeft)
                            xOffset += 2;

                        if(dPadRight)
                            xOffset -= 2;

                        if (buttonB)
                            g_GameRunning = false;

                        int16_t lx = inputState.Gamepad.sThumbLX;
                        int16_t ly = inputState.Gamepad.sThumbLY;

                        // Not working. Need to debug later
                        //if(lx > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                        //    xOffset -= log2(lx);
                        //else if (lx < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                        //    xOffset += log2(lx);


                        //if(ly > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                        //    yOffset -= log2(ly);
                        //else if (ly < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                        //    yOffset += log2(ly);

                        xOffset += lx >> 12;
                        yOffset += ly >> 12;
                    }
                }

                RenderColorGradient(xOffset, yOffset);                                  // Render the back buffer and paint the window
                GetClientRect(hhWindow, &client_rect);                                  // Find the new size of the window
                PaintWindowFromCurrentBackBuffer(windowDeviceContext, &client_rect);    // Render the fixed size back buffer in the new sized window
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
