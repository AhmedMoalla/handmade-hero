#include <dsound.h>
#include <stdint.h>
#include <windows.h>
#include <xinput.h>

#define internal static
#define local_persist static
#define global_variable static

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void)
{
    HMODULE xInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!xInputLibrary) {
        xInputLibrary = LoadLibrary("xinput1_3.dll");
    }

    if (xInputLibrary) {
        XInputGetState = (x_input_get_state*)GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(xInputLibrary, "XInputSetState");
    }
}

typedef HRESULT WINAPI direct_sound_create(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);

internal void Win32InitDSound(HWND window, DWORD samplesPerSecond, DWORD bufferSize)
{
    HMODULE dSoundLibrary = LoadLibrary("dsound.dll");
    if (dSoundLibrary) {
        direct_sound_create* DirectSoundCreate = (direct_sound_create*)
            GetProcAddress(dSoundLibrary, "DirectSoundCreate");
        IDirectSound* directSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(NULL, &directSound, NULL))) {
            WAVEFORMATEX waveFromat = {};
            waveFromat.wFormatTag = WAVE_FORMAT_PCM;
            waveFromat.nChannels = 2;
            waveFromat.nSamplesPerSec = samplesPerSecond;
            waveFromat.wBitsPerSample = 16;
            waveFromat.nBlockAlign = (waveFromat.nChannels * waveFromat.wBitsPerSample) / 8;
            waveFromat.nAvgBytesPerSec = waveFromat.nSamplesPerSec * waveFromat.nBlockAlign;
            waveFromat.cbSize = 0;

            if (FAILED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
                return;
            }

            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

            IDirectSoundBuffer* primaryBuffer;
            if (FAILED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, NULL))) {
                return;
            }
            OutputDebugString("Primary buffer was created.\n");

            if (FAILED(primaryBuffer->SetFormat(&waveFromat))) {
                return;
            }
            OutputDebugString("Primary buffer format was set.\n");

            bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFromat;

            IDirectSoundBuffer* secondaryBuffer;
            if (FAILED(directSound->CreateSoundBuffer(&bufferDescription, &secondaryBuffer, NULL))) {
                return;
            }
            OutputDebugString("Secondary buffer was created.\n");
        }
    }
}

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

internal void RenderWeirdGradient(Win32OffscreenBuffer* buffer, int xOffset, int yOffset)
{
    uint8_t* row = (uint8_t*)buffer->memory;
    for (int y = 0; y < buffer->height; ++y) {
        uint32_t* pixel = (uint32_t*)row;
        for (int x = 0; x < buffer->width; ++x) {
            uint8_t B = (x + xOffset);
            uint8_t G = (y + yOffset);
            uint8_t R = 150;
            *pixel++ = ((R << 16) | (G << 8) | B);
        }
        row += buffer->pitch;
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
    buffer->memory = VirtualAlloc(NULL, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    buffer->pitch = buffer->width * bytesPerPixel;
}

internal void Win32DisplayBufferInWindow(Win32OffscreenBuffer* buffer, HDC deviceContext, LONG windowWidth, LONG windowHeight)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory, &buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
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
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
        WPARAM vkCode = wParam;
        bool wasDown = (lParam & (1 << 30)) != 0;
        bool isDown = (lParam & (1 << 31)) == 0;
        if (wasDown == isDown) {
            break;
        }

        switch (vkCode) {
        case 'Z':
            OutputDebugString("Z");
            break;
        case 'Q':
            OutputDebugString("Q");
            break;
        case 'S':
            OutputDebugString("S");
            break;
        case 'D':
            OutputDebugString("D");
            break;
        case 'A':
            OutputDebugString("A");
            break;
        case 'E':
            OutputDebugString("E");
            break;
        case VK_UP:
            OutputDebugString("UP");
            break;
        case VK_LEFT:
            OutputDebugString("LEFT");
            break;
        case VK_DOWN:
            OutputDebugString("DOWN");
            break;
        case VK_RIGHT:
            OutputDebugString("RIGHT");
            break;
        case VK_ESCAPE:
            OutputDebugString("ESC: ");
            if (isDown) {
                OutputDebugString("isDown ");
            }
            if (wasDown) {
                OutputDebugString("wasDown");
            }
            OutputDebugString("\n");
            break;
        case VK_SPACE:
            OutputDebugString("SPACE");
            break;
        default:
            result = DefWindowProc(window, message, wParam, lParam);
            break;
        };
    } break;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
        Win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimensions.width, dimensions.height);
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
    Win32LoadXInput();
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

    Win32InitDSound(window, 48000, 48000 * sizeof(int16_t) * 2);

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

        DWORD result;
        for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++) {
            XINPUT_STATE controllerState;

            auto result = XInputGetState(controllerIndex, &controllerState);
            if (result == ERROR_SUCCESS) {
                XINPUT_GAMEPAD* pad = &controllerState.Gamepad;
                bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
                bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
                bool leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                bool rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
                bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
                bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
                bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

                int stickX = pad->sThumbLX;
                int stickY = pad->sThumbLY;

                xOffset -= stickX >> 12;
                yOffset += stickY >> 12;

                if (aButton) {
                    XINPUT_VIBRATION vibration;
                    vibration.wLeftMotorSpeed = 60000;
                    XInputSetState(0, &vibration);
                } else {
                    XINPUT_VIBRATION vibration;
                    vibration.wLeftMotorSpeed = 0;
                    XInputSetState(0, &vibration);
                }
            } else {
                // Controller is not connected
            }
        }

        RenderWeirdGradient(&globalBackBuffer, xOffset, yOffset);

        Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
        Win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimensions.width, dimensions.height);
        xOffset += 1;
    }

    return 0;
}
