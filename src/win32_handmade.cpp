#include <stdint.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

struct Win32OffscreenBuffer {
    BITMAPINFO info;
    void* memory;
    LONG width;
    LONG height;
    int pitch;
};

struct Win32WindowDimensions {
    LONG width;
    LONG height;
};

internal Win32WindowDimensions Win32GetWindowDimensions(HWND window)
{
    Win32WindowDimensions result;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return result;
}

global_variable bool running;
global_variable Win32OffscreenBuffer globalBackBuffer;

internal void RenderWeirdGradient(Win32OffscreenBuffer buffer, int xOffset, int yOffset)
{
    uint8_t* row = (uint8_t*)buffer.memory;
    for (int y = 0; y < buffer.height; ++y) {
        uint32_t* pixel = (uint32_t*)row;
        for (int x = 0; x < buffer.width; ++x) {
            uint8_t B = (x + xOffset);
            uint8_t G = (y + yOffset);
            uint8_t R = 150;
            *pixel++ = ((R << 16) | (G << 8) | B);
        }
        row += buffer.pitch;
    }
}

// DIB = Device Independent Bitmap
internal void Win32ResizeDIBSection(Win32OffscreenBuffer* buffer, LONG width, LONG height)
{
    if (buffer->memory) {
        VirtualFree(buffer->memory, NULL, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    buffer->info.bmiHeader.biHeight = -height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bytesPerPixel = 4;
    int bitmapMemorySize = width * height * bytesPerPixel;
    buffer->memory = VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    buffer->pitch = buffer->width * bytesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC deviceContext, LONG windowWidth, LONG windowHeight, Win32OffscreenBuffer buffer)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer.width, buffer.height,
                  buffer.memory, &buffer.info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch (message) {
    case WM_SIZE: {
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
        Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
        Win32DisplayBufferInWindow(deviceContext, dimensions.width, dimensions.height, globalBackBuffer);
        EndPaint(window, &paint);
    } break;
    default: {
        result = DefWindowProc(window, message, wParam, lParam);
    } break;
    }

    return result;
}

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR cmdLine, int showCmd)
{
    Win32ResizeDIBSection(&globalBackBuffer, 1280, 720);

    WNDCLASS windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (!RegisterClass(&windowClass)) {
        return 1;
    }

    HWND window = CreateWindowEx(
        NULL, windowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, instance, NULL);
    if (!window) {
        return 1;
    }

    HDC deviceContext = GetDC(window);
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

        RenderWeirdGradient(globalBackBuffer, xOffset, yOffset);

        Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
        Win32DisplayBufferInWindow(deviceContext, dimensions.width, dimensions.height, globalBackBuffer);
        xOffset++;
        yOffset++;
    }

    return 0;
}
