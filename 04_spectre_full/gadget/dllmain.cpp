#include <windows.h>

extern "C" {
  void LoadGadget() {}
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) {
  return TRUE;
}
