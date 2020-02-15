#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

// TBD: This is a global for now
global_variable bool Running;

internal void
Win32ResizeDIBSection(int Width, int Height)
{
    HBITMAP CreateDIBSection(
        HDC hdc,
        const BITMAPINFO *pbmi,
        UINT iUsage,
        VOID **ppvBits,
        HANDLE hSection,
        DWORD dwOffset);
}

internal void
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
    StretchDIBits(
        HDC hdc,
        int xDest,
        int yDest,
        int nDestWidth,
        int nDestHeight,
        int XSrc,
        int YSrc,
        int nSrcWidth,
        int nSrcHeight,
        const VOID *lpBits,
        const BITMAPINFO *lpBitsInfo,
        UINT iUsage,
        DWORD dwRop);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            ResizeDIBSection(Width, Height);
        } break;
        
        case WM_CLOSE:
        {
            // TBD: Handle this with a message to the user
            Running = false;
        } break;
 
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            // TBD: Handle this as an error - recreate window
            Running = false;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            Win32UpdatWindow(DeviceContext, X, Y, Width, Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
//            OutputDebugStringA("default\n");
              Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    WNDCLASS WindowClass = {};

    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
//  WindowClass.hIcon;
    WindowClass.lpszClassName = "GameWindowClass";


    if(RegisterClassA(&WindowClass))
    {
        HWND WindowHandle = 
            CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "Game",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);
        if(WindowHandle)
        {
            Running = true;
            while(Running)
            {
                MSG Message;
                BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);
                if(MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            // TBD: Logging
        }
   }
   else
   {
      // TBD: Logging
   }            


    return(0);
}
