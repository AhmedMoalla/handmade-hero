#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable bool running;
global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable HBITMAP bitmapHandle;
global_variable HDC bitmapDeviceContext;

// DIB = Device Independent Bitmap
internal void Win32ResizeDIBSection(LONG width, LONG height) {

  if (bitmapHandle) {
    DeleteObject(bitmapHandle);
  }

  if (!bitmapDeviceContext) {
    bitmapDeviceContext = CreateCompatibleDC(0);
  }

  bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth = width;
  bitmapInfo.bmiHeader.biHeight = height;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;

  bitmapHandle = CreateDIBSection(bitmapDeviceContext, &bitmapInfo,
                                  DIB_RGB_COLORS, &bitmapMemory, 0, 0);
}

internal void Win32UpdateWindow(HDC deviceContext, LONG x, LONG y, LONG width,
                                LONG height) {

  StretchDIBits(deviceContext, x, y, width, height, x, y, width, height,
                bitmapMemory, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam,
                                LPARAM lParam) {
  LRESULT result = 0;
  switch (message) {
  case WM_SIZE: {
    RECT clientRect;
    GetClientRect(window, &clientRect);
    LONG width = clientRect.right - clientRect.left;
    LONG height = clientRect.bottom - clientRect.top;
    Win32ResizeDIBSection(width, height);
  } break;
  case WM_DESTROY: {
    running = false;
  } break;
  case WM_CLOSE: {
    running = false;
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugString("WM_ACTIVATEAPP\n");
  } break;
  case WM_PAINT: {
    PAINTSTRUCT paint;
    HDC deviceContext = BeginPaint(window, &paint);
    LONG x = paint.rcPaint.left;
    LONG y = paint.rcPaint.top;
    LONG height = paint.rcPaint.bottom - paint.rcPaint.top;
    LONG width = paint.rcPaint.right - paint.rcPaint.left;
    Win32UpdateWindow(deviceContext, x, y, width, height);
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
  windowClass.lpfnWndProc = Win32MainWindowCallback;
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

  running = true;
  while (running) {
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
