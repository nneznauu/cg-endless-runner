#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// GLUT reports typed characters. Read the game letter keys by their Windows
// virtual-key codes instead, so English and Russian layouts use the same keys.
template <typename OnPress>
bool pollWindowsLetterKeys(bool (&held)[256], OnPress onPress) {
    const HWND active = GetActiveWindow();
    const bool focused = active != nullptr && active == GetForegroundWindow();
    constexpr unsigned char controls[]{ 'a', 'd', 'p', 'r', 't', 'i', 'c', 'v', 'h', 'j', 'l' };
    for (const unsigned char key : controls) {
        const int virtualKey = 'A' + (key - 'a');
        const bool down = focused && (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
        const bool pressed = down && !held[key];
        held[key] = down;
        // Toggles fire once per press; movement stays active while held.
        if (pressed) onPress(key);
    }
    return focused;
}
#endif
