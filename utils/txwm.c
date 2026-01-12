#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../core.h"

typedef struct {
  Display* dpy;
  Window root;
  XWindowAttributes attr;
  XButtonEvent start;
  XEvent event;
} txwm;

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main(void) {
  txwm tx = {0};

  if (!(tx.dpy = XOpenDisplay(0x0))) die(__LINE__, "Cannot open X11 display");

  tx.root = DefaultRootWindow(tx.dpy);

  XGrabKey(tx.dpy, XKeysymToKeycode(tx.dpy, XStringToKeysym("n")), Mod4Mask,
           tx.root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(tx.dpy, XKeysymToKeycode(tx.dpy, XStringToKeysym("q")), Mod4Mask,
           tx.root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(tx.dpy, XKeysymToKeycode(tx.dpy, XStringToKeysym("e")), Mod1Mask,
           tx.root, True, GrabModeAsync, GrabModeAsync);

  XGrabKey(tx.dpy, XKeysymToKeycode(tx.dpy, XStringToKeysym("F1")), Mod1Mask,
           tx.root, True, GrabModeAsync, GrabModeAsync);
  XGrabButton(tx.dpy, 1, Mod1Mask, tx.root, True,
              ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(tx.dpy, 3, Mod1Mask, tx.root, True,
              ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);

  tx.start.subwindow = None;

  while (1) {
    XNextEvent(tx.dpy, &tx.event);

    if (tx.event.type == KeyPress) {
      if (tx.event.xkey.keycode ==
          XKeysymToKeycode(tx.dpy, XStringToKeysym("e"))) {
        printf("dmenu\n");
        system("dmenu_run &");
      } else if (tx.event.xkey.subwindow != None) {
        if (tx.event.xkey.keycode ==
            XKeysymToKeycode(tx.dpy, XStringToKeysym("n"))) {
          XCirculateSubwindowsUp(tx.dpy, tx.root);
          XSetInputFocus(tx.dpy, tx.event.xkey.window, RevertToParent,
                         CurrentTime);
        } else if (tx.event.xkey.keycode ==
                   XKeysymToKeycode(tx.dpy, XStringToKeysym("q"))) {
          XKillClient(tx.dpy, tx.event.xkey.subwindow);
        } else if (tx.event.xkey.keycode ==
                   XKeysymToKeycode(tx.dpy, XStringToKeysym("F1"))) {
          XRaiseWindow(tx.dpy, tx.event.xkey.subwindow);
        }
      }
    } else if (tx.event.type == ButtonPress &&
               tx.event.xbutton.subwindow != None) {
      XGetWindowAttributes(tx.dpy, tx.event.xbutton.subwindow, &tx.attr);
      tx.start = tx.event.xbutton;
    } else if (tx.event.type == MotionNotify && tx.start.subwindow != None) {
      int xdiff = tx.event.xbutton.x_root - tx.start.x_root;
      int ydiff = tx.event.xbutton.y_root - tx.start.y_root;

      XMoveResizeWindow(
          tx.dpy, tx.start.subwindow,
          tx.attr.x + (tx.start.button == 1 ? xdiff : 0),
          tx.attr.y + (tx.start.button == 1 ? ydiff : 0),
          MAX(1, tx.attr.width + (tx.start.button == 3 ? xdiff : 0)),
          MAX(1, tx.attr.height + (tx.start.button == 3 ? ydiff : 0)));
    } else if (tx.event.type == ButtonRelease) {
      tx.start.subwindow = None;
    }
  }
  XCloseDisplay(tx.dpy);

  return EXIT_SUCCESS;
}
