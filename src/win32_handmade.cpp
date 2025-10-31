#include <windows.h>

LRESULT MainWindowCallback(HWND window, UINT message, WPARAM wParam,
                           LPARAM lParam) {
  LRESULT result = 0;
  switch (message) {
  case WM_SIZE: {
    OutputDebugString("WM_SIZE\n");
  } break;
  case WM_DESTROY: {
    OutputDebugString("WM_DESTROY\n");
  } break;
  case WM_CLOSE: {
    OutputDebugString("WM_CLOSE\n");
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugString("WM_ACTIVATEAPP\n");
  } break;
  case WM_PAINT: {
    PAINTSTRUCT paint;
    HDC deviceContext = BeginPaint(window, &paint);
    int x = paint.rcPaint.left;
    int y = paint.rcPaint.top;
    LONG height = paint.rcPaint.bottom - paint.rcPaint.top;
    LONG width = paint.rcPaint.right - paint.rcPaint.left;
    static auto operation = WHITENESS;
    if (operation == WHITENESS) {
        operation = BLACKNESS;
    } else {
        operation = WHITENESS;
    }
    PatBlt(deviceContext, x, y, width, height, operation);
    EndPaint(window, &paint);
  } break;
  default: {
    result = DefWindowProc(window, message, wParam, lParam);
  } break;
  }

  return result;
}

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE previousInstance,
                     LPSTR cmdLine, int showCmd) {

  WNDCLASS windowClass = {};
  windowClass.lpfnWndProc = MainWindowCallback;
  windowClass.hInstance = instance;
  windowClass.lpszClassName = "HandmadeHeroWindowClass";

  if (!RegisterClass(&windowClass)) {
    return 1;
  }

  HWND windowHandle = CreateWindowEx(
      0, windowClass.lpszClassName, "Handmade Hero",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
  if (!windowHandle) {
    return 1;
  }

  for (;;) {
    MSG message;
    BOOL result = GetMessage(&message, 0, 0, 0);
    if (result > 0) {
      TranslateMessage(&message);
      DispatchMessage(&message);
    } else {
      break;
    }
  }

  return 0;
}
