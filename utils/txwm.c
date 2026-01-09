#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdarg.h>

#include "../core.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main(void) {
  Display* dpy;
  Window root;
  XWindowAttributes attr;

  XButtonEvent start;
  XEvent event;

  if (!(dpy = XOpenDisplay(0x0))) die(__LINE__, "Cannot open X11 display");

  root = DefaultRootWindow(dpy);

  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("n")), Mod4Mask, root,
           True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("q")), Mod4Mask, root,
           True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("e")), Mod1Mask, root,
           True, GrabModeAsync, GrabModeAsync);

  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask, root,
           True, GrabModeAsync, GrabModeAsync);
  XGrabButton(dpy, 1, Mod1Mask, root, True,
              ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(dpy, 3, Mod1Mask, root, True,
              ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);

  start.subwindow = None;

  while (1) {
    XNextEvent(dpy, &event);

    if (event.type == KeyPress) {
      if (event.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("e"))) {
        printf("dmenu\n");
        system("dmenu_run &");
      } else if (event.xkey.subwindow != None) {
        if (event.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("n"))) {
          XCirculateSubwindowsUp(dpy, root);
          XSetInputFocus(dpy, event.xkey.window, RevertToParent, CurrentTime);
        } else if (event.xkey.keycode ==
                   XKeysymToKeycode(dpy, XStringToKeysym("q"))) {
          XKillClient(dpy, event.xkey.subwindow);
        } else if (event.xkey.keycode ==
                   XKeysymToKeycode(dpy, XStringToKeysym("F1"))) {
          XRaiseWindow(dpy, event.xkey.subwindow);
        }
      }
    } else if (event.type == ButtonPress && event.xbutton.subwindow != None) {
      XGetWindowAttributes(dpy, event.xbutton.subwindow, &attr);
      start = event.xbutton;
    } else if (event.type == MotionNotify && start.subwindow != None) {
      int xdiff = event.xbutton.x_root - start.x_root;
      int ydiff = event.xbutton.y_root - start.y_root;

      XMoveResizeWindow(dpy, start.subwindow,
                        attr.x + (start.button == 1 ? xdiff : 0),
                        attr.y + (start.button == 1 ? ydiff : 0),
                        MAX(1, attr.width + (start.button == 3 ? xdiff : 0)),
                        MAX(1, attr.height + (start.button == 3 ? ydiff : 0)));
    } else if (event.type == ButtonRelease) {
      start.subwindow = None;
    }
  }
}
