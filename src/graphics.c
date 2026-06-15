#include "graphics.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>

static HWND g_hWnd = NULL;
static HDC g_hDC = NULL;
static int g_winWidth = 0;
static int g_winHeight = 0;
static bool g_windowClosed = false;
static uint32_t* g_pixelBuffer = NULL;

static bool g_keys[256] = {0};
static int g_mouseX = 0;
static int g_mouseY = 0;
static bool g_mouseLeft = false;
static bool g_mouseRight = false;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_DESTROY:
      g_windowClosed = true;
      PostQuitMessage(0);
      break;
    case WM_CLOSE:
      g_windowClosed = true;
      DestroyWindow(hWnd);
      break;
    case WM_KEYDOWN:
      if (wParam < 256) g_keys[wParam] = true;
      break;
    case WM_KEYUP:
      if (wParam < 256) g_keys[wParam] = false;
      break;
    case WM_MOUSEMOVE:
      g_mouseX = (int)(short)LOWORD(lParam);
      g_mouseY = (int)(short)HIWORD(lParam);
      break;
    case WM_LBUTTONDOWN:
      g_mouseLeft = true;
      break;
    case WM_LBUTTONUP:
      g_mouseLeft = false;
      break;
    case WM_RBUTTONDOWN:
      g_mouseRight = true;
      break;
    case WM_RBUTTONUP:
      g_mouseRight = false;
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

bool initWindow(int width, int height, const char* title) {
  if (g_hWnd != NULL) return true;

  g_winWidth = width;
  g_winHeight = height;
  g_windowClosed = false;
  memset(g_keys, 0, sizeof(g_keys));

  HINSTANCE hInstance = GetModuleHandle(NULL);
  WNDCLASS wc = {0};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
  wc.lpszClassName = "TupalGraphicsWindow";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);

  if (!RegisterClass(&wc)) {
    if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
      return false;
    }
  }

  RECT rect = {0, 0, width, height};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
  int w = rect.right - rect.left;
  int h = rect.bottom - rect.top;

  g_hWnd = CreateWindow(
    "TupalGraphicsWindow",
    title,
    WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
    CW_USEDEFAULT, CW_USEDEFAULT,
    w, h,
    NULL, NULL, hInstance, NULL
  );

  if (!g_hWnd) return false;

  g_hDC = GetDC(g_hWnd);
  ShowWindow(g_hWnd, SW_SHOW);
  UpdateWindow(g_hWnd);

  g_pixelBuffer = (uint32_t*)realloc(g_pixelBuffer, width * height * sizeof(uint32_t));
  memset(g_pixelBuffer, 0, width * height * sizeof(uint32_t));

  return true;
}

bool updateWindow(const uint8_t* rgbaBuffer, int size) {
  if (g_hWnd == NULL || g_windowClosed) return false;

  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  if (g_windowClosed) return false;

  int numPixels = g_winWidth * g_winHeight;
  if (size >= numPixels * 4) {
    for (int i = 0; i < numPixels; i++) {
      uint8_t r = rgbaBuffer[i * 4 + 0];
      uint8_t g = rgbaBuffer[i * 4 + 1];
      uint8_t b = rgbaBuffer[i * 4 + 2];
      g_pixelBuffer[i] = (r << 16) | (g << 8) | b;
    }
  } else if (size >= numPixels * 3) {
    for (int i = 0; i < numPixels; i++) {
      uint8_t r = rgbaBuffer[i * 3 + 0];
      uint8_t g = rgbaBuffer[i * 3 + 1];
      uint8_t b = rgbaBuffer[i * 3 + 2];
      g_pixelBuffer[i] = (r << 16) | (g << 8) | b;
    }
  } else {
    memset(g_pixelBuffer, 0, numPixels * sizeof(uint32_t));
  }

  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = g_winWidth;
  bmi.bmiHeader.biHeight = -g_winHeight; // top-down
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  StretchDIBits(
    g_hDC,
    0, 0, g_winWidth, g_winHeight,
    0, 0, g_winWidth, g_winHeight,
    g_pixelBuffer,
    &bmi,
    DIB_RGB_COLORS,
    SRCCOPY
  );

  return true;
}

void closeWindow() {
  if (g_hWnd != NULL) {
    ReleaseDC(g_hWnd, g_hDC);
    DestroyWindow(g_hWnd);
    g_hWnd = NULL;
    g_hDC = NULL;
  }
  if (g_pixelBuffer != NULL) {
    free(g_pixelBuffer);
    g_pixelBuffer = NULL;
  }
}

bool isKeyPressed(int virtualKeyCode) {
  if (virtualKeyCode < 0 || virtualKeyCode >= 256) return false;
  return g_keys[virtualKeyCode];
}

void getMouseState(int* x, int* y, bool* left, bool* right) {
  *x = g_mouseX;
  *y = g_mouseY;
  *left = g_mouseLeft;
  *right = g_mouseRight;
}
