#pragma once

template <class DERIVED_TYPE>
class BaseWindow {
public:
  static LRESULT CALLBACK WindowProc(HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) {
    DERIVED_TYPE *p = nullptr;
    if (uMsg == WM_NCCREATE) {
      if (auto cs = reinterpret_cast<LPCREATESTRUCT>(lParam)) {
        p = reinterpret_cast<DERIVED_TYPE*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd,
                         GWLP_USERDATA,
                         reinterpret_cast<LPARAM>(p));
        p->hwnd_ = hwnd;
      }
    }
    else {
      p = reinterpret_cast<DERIVED_TYPE*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    return p ? p->HandleMessage(uMsg, wParam, lParam)
             : DefWindowProc(hwnd, uMsg, wParam, lParam);
  }

  BaseWindow() : hwnd_(nullptr) {}

  bool Create(PCWSTR caption,
              DWORD style,
              DWORD style_ex = 0,
              int x = CW_USEDEFAULT,
              int y = CW_USEDEFAULT,
              int width = CW_USEDEFAULT,
              int height = CW_USEDEFAULT,
              HWND parent = nullptr,
              LPCWSTR menu = nullptr) {
    if (!windowclass) {
      WNDCLASSEXW wcex{};
      wcex.cbSize = sizeof(wcex);
      wcex.style = CS_HREDRAW | CS_VREDRAW;
      wcex.lpfnWndProc = DERIVED_TYPE::WindowProc;
      wcex.hInstance = GetModuleHandle(nullptr);
      wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
      wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
      wcex.lpszMenuName = menu;
      wcex.lpszClassName = ClassName();
      windowclass = RegisterClassExW(&wcex);
    }
    if (!hwnd_) {
      hwnd_ = CreateWindowEx(style_ex,
                             ClassName(),
                             caption,
                             style,
                             x, y, width, height,
                             parent,
                             /*hMenu*/nullptr,
                             GetModuleHandle(nullptr),
                             this);
    }
    return !!hwnd_;
  }

  HWND hwnd() {
    return hwnd_;
  }

protected:
  virtual LPCWSTR ClassName() const = 0;
  virtual LRESULT HandleMessage(UINT, WPARAM, LPARAM) = 0;

private:
  static ATOM windowclass;
  HWND hwnd_;
};

template <class DERIVED_TYPE>
ATOM BaseWindow<DERIVED_TYPE>::windowclass = 0;
