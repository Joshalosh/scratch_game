#include <windows.h>

int CALLBACK
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
  MessageBoxA(0, "This is a game.", "GAME FROM SCRATCH",
             MB_OK|MB_ICONINFORMATION);
  return(0);
}
