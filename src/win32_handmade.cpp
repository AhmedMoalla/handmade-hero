#include <stdint.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable bool running;

global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable LONG bitmapWidth;
global_variable LONG bitmapHeight;
global_variable int bytesPerPixel = 4;

internal void RenderWeirdGradient(int xOffset, int yOffset) {
  int pitch = bitmapWidth * bytesPerPixel;
  uint8_t *row = (uint8_t *)bitmapMemory;
  for (int y = 0; y < bitmapHeight; ++y) {
    uint32_t *pixel = (uint32_t *)row;
    for (int x = 0; x < bitmapWidth; ++x) {
      uint8_t B = (x + xOffset);
      uint8_t G = (y + yOffset);
      uint8_t R = 150;
      *pixel++ = ((R << 16) | (G << 8) | B);
    }
    row += pitch;
  }
}

// DIB = Device Independent Bitmap
internal void Win32ResizeDIBSection(LONG width, LONG height) {

  if (bitmapMemory) {
    VirtualFree(bitmapMemory, NULL, MEM_RELEASE);
  }

  bitmapWidth = width;
  bitmapHeight = height;

  bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth = width;
  bitmapInfo.bmiHeader.biHeight = -height;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;

  int bitmapMemorySize = width * height * bytesPerPixel;
  bitmapMemory =
      VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC deviceContext, RECT *windowRect, LONG x,
                                LONG y, LONG width, LONG height) {

  LONG windowWidth = windowRect->right - windowRect->left;
  LONG windowHeight = windowRect->bottom - windowRect->top;
  StretchDIBits(deviceContext, 0, 0, bitmapWidth, bitmapHeight, 0, 0,
                windowWidth, windowHeight, bitmapMemory, &bitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
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

    RECT clientRect;
    GetClientRect(window, &clientRect);

    Win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);
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

  HWND window = CreateWindowEx(NULL, windowClass.lpszClassName, "Handmade Hero",
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               NULL, NULL, instance, NULL);
  if (!window) {
    return 1;
  }

  int xOffset = 0, yOffset = 0;
  running = true;
  while (running) {
    MSG message;
    while (PeekMessage(&message, NULL, NULL, NULL, PM_REMOVE)) {
      if (message.message == WM_QUIT) {
        running = false;
      }
      TranslateMessage(&message);
      DispatchMessage(&message);
    }

    RenderWeirdGradient(xOffset, yOffset);

    HDC deviceContext = GetDC(window);
    RECT clientRect;
    GetClientRect(window, &clientRect);
    LONG windowWidth = clientRect.right - clientRect.left;
    LONG windowHeight = clientRect.bottom - clientRect.top;
    Win32UpdateWindow(deviceContext, &clientRect, 0, 0, windowWidth,
                      windowHeight);
    ReleaseDC(window, deviceContext);
    xOffset++;
  }

  return 0;
}
