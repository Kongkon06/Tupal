#ifndef tupal_graphics_h
#define tupal_graphics_h

#include <stdbool.h>
#include <stdint.h>

bool initWindow(int width, int height, const char* title);
bool updateWindow(const uint8_t* rgbaBuffer, int size);
void closeWindow();
bool isKeyPressed(int virtualKeyCode);
void getMouseState(int* x, int* y, bool* left, bool* right);

#endif
