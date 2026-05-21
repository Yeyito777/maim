#include <chrono>
#include <stdexcept>
#include <thread>
#include "keyboard.hpp"

bool slop::Keyboard::getKey( KeySym key ) {
    KeyCode keycode = XKeysymToKeycode( x11->display, key );
    if ( keycode != 0 ) {
        // Get the whole keyboard state
        char keys[32];
        XQueryKeymap( x11->display, keys );
        // Check our keycode
        return ( keys[ keycode / 8 ] & ( 1 << ( keycode % 8 ) ) ) != 0;
    } else {
        return false;
    }
}

// This returns if a key is currently pressed.
// Ignores arrow key presses specifically so users can
// adjust their selection.
bool slop::Keyboard::anyKeyDown() {
    return keyDown;
}

bool slop::Keyboard::isEnterDown() {
    return getKey( XK_Return ) || getKey( XK_KP_Enter );
}

void slop::Keyboard::maskKey( char* keys, KeySym key ) {
    KeyCode keycode = XKeysymToKeycode( x11->display, key );
    if ( keycode != 0 ) {
        keys[ keycode / 8 ] = keys[ keycode / 8 ] & ~( 1 << ( keycode % 8 ) );
    }
}

void slop::Keyboard::update() {
    char keys[32];
    XQueryKeymap( x11->display, keys );
    // We first delete the arrow key buttons from the mapping.
    // This allows the user to press the arrow keys without triggering anyKeyDown
    maskKey( keys, XK_Left );
    maskKey( keys, XK_Right );
    maskKey( keys, XK_Up );
    maskKey( keys, XK_Down );
    // Also deleting Space for move operation
    maskKey( keys, XK_space );
    // Enter is treated as a virtual left mouse button by this fork, so it
    // should not trigger the normal "any key cancels selection" behavior.
    maskKey( keys, XK_Return );
    maskKey( keys, XK_KP_Enter );
    keyDown = false;
    for ( int i=0;i<32;i++ ) {
        if ( deltaState[i] == keys[i] ) {
            continue;
        }
        // Found a key in a group of 4 that's different
        char a = deltaState[i];
        char b = keys[i];
        // Find the "different" bits
        char c = a^b;
        // A new key was pressed since the last update.
        if ( c && b&c ) {
            keyDown = true;
        }
        deltaState[i] = keys[i];
    }
}

slop::Keyboard::Keyboard( X11* x11 ) {
    this->x11 = x11;
    grabbed = false;
    int err = XGrabKeyboard( x11->display, x11->root, False, GrabModeAsync, GrabModeAsync, CurrentTime );
    int tries = 0;
    // When maim is launched from a hotkey, the window manager/keybinding
    // daemon can still have an active keyboard grab for a short moment.  The
    // upstream slop code only retried for ~5ms and then continued without a
    // grab, which let our Enter-as-click keypress leak through to whatever
    // app was underneath the selection.  Wait longer and require the grab: if
    // we cannot own the keyboard, Enter cannot be safely used as a click.
    while( err != GrabSuccess && tries < 400 ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        err = XGrabKeyboard( x11->display, x11->root, False, GrabModeAsync, GrabModeAsync, CurrentTime );
        tries++;
    }
    if ( err != GrabSuccess ) {
        throw new std::runtime_error( "Couldn't grab the keyboard; refusing to start selection because Enter-as-click would leak to the focused application." );
    }
    grabbed = true;
    XSync( x11->display, False );
    XQueryKeymap( x11->display, deltaState );
    keyDown = false;
}

slop::Keyboard::~Keyboard() {
    if ( grabbed ) {
        XUngrabKeyboard( x11->display, CurrentTime );
    }
}
