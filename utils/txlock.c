#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <crypt.h>
#include <pthread.h>
#include <pwd.h>
#include <shadow.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../core.h"

typedef struct {
  Display* dpy;
  Window win, root;
  GC gc, wrong_passwd;
  XEvent event;
  XFontStruct* font_struct;
  XWindowAttributes attr;
  Font font;
  int screen;
} txlock;

#define INPUT_LENGTH 512

char user_input[INPUT_LENGTH] = {0};
uint16_t user_input_length = 0;

uint16_t x_rand_square_data[INPUT_LENGTH], y_rand_square_data[INPUT_LENGTH];

uint16_t number_of_squares = 0;

int verify_passwd(char* password) {
  struct passwd* pe = getpwnam("root");
  if (!pe) die(__LINE__, "User does not exist!");
  if (strcmp(pe->pw_passwd, "x") == 0) {
    struct spwd* se = getspnam("root");
    if (!se) die(__LINE__, "Failed to read /etc/shadow!");
    return strcmp(se->sp_pwdp, crypt(password, se->sp_pwdp));
  }
  return strcmp(pe->pw_passwd, crypt(password, pe->pw_passwd));
}

void purge_sq(txlock* tx) {
  user_input[user_input_length - 1] = '\0';
  user_input_length--;

  x_rand_square_data[number_of_squares] = '\0',
  y_rand_square_data[number_of_squares] = '\0';
  number_of_squares--;

  XClearWindow(tx->dpy, tx->win);
  XFlush(tx->dpy);

  for (int i = 0; i < number_of_squares; i++) {
    XFillRectangle(tx->dpy, tx->win, tx->gc, x_rand_square_data[i],
                   y_rand_square_data[i], square_width, square_height);
    XFlush(tx->dpy);
  }
}

int main(int argc, char** argv) {
  txlock tx = {0};
  strcpy(name, txname());
  conf_analyzer(txname());
  verify_conf_args();
  srand(time(NULL));

  // if (getuid())
  //   die("TXlock must be run under root!");

  if ((tx.dpy = XOpenDisplay(NULL)) == NULL)
    die(__LINE__, "Failed to open X11 display");

  XSetWindowAttributes attrs;
  attrs.override_redirect = True;
  attrs.background_pixel = bg_color;
  tx.root = DefaultRootWindow(tx.dpy);
  tx.screen = DefaultScreen(tx.dpy);
  tx.win = XCreateWindow(tx.dpy, tx.root, 0, 0, DisplayWidth(tx.dpy, tx.screen),
                         DisplayHeight(tx.dpy, tx.screen), 0,
                         DefaultDepth(tx.dpy, tx.screen), CopyFromParent,
                         DefaultVisual(tx.dpy, tx.screen),
                         CWOverrideRedirect | CWBackPixel, &attrs);
  tx.gc = XCreateGC(tx.dpy, tx.win, 0, NULL);
  tx.wrong_passwd = XCreateGC(tx.dpy, tx.win, 0, NULL);

  tx.font_struct = XLoadQueryFont(tx.dpy, "fixed");

  tx.font = tx.font_struct->fid;
  XSetFont(tx.dpy, tx.wrong_passwd, tx.font);

  XSetForeground(tx.dpy, tx.gc, square_color);
  XSetFillStyle(tx.dpy, tx.gc, FillSolid);

  XStoreName(tx.dpy, tx.win, "txlock");
  XSelectInput(tx.dpy, tx.win,
               StructureNotifyMask | KeyPressMask | FocusChangeMask);
  XMapWindow(tx.dpy, tx.win);
  XGrabKeyboard(tx.dpy, tx.win, True, GrabModeAsync, GrabModeAsync,
                CurrentTime);

  for (;;) {
    XEvent event;
    XNextEvent(tx.dpy, &event);
    printf("%s\n", user_input);
    switch (event.type) {
      case KeyPress:
        KeySym keysym = XLookupKeysym(&event.xkey, 0);
        if ((event.xkey.state & ControlMask) && keysym == XK_u) {
          number_of_squares = 1, user_input_length = 1;
          purge_sq(&tx);
          break;
        }
        if (keysym > 32 && keysym < 127 && user_input_length < INPUT_LENGTH) {
          int x_rand_coordinates = randnum(
                  (((DisplayWidth(tx.dpy, tx.screen) - 1270) >> 1) + 1270) -
                      100,
                  (DisplayWidth(tx.dpy, tx.screen) - 1270) >> 1),
              y_rand_coordinates = randnum(
                  (((DisplayHeight(tx.dpy, tx.screen) - 720) >> 1) + 720) - 100,
                  (DisplayHeight(tx.dpy, tx.screen) - 720) >> 1);
          x_rand_square_data[number_of_squares] = x_rand_coordinates,
          y_rand_square_data[number_of_squares] = y_rand_coordinates;
          XFillRectangle(tx.dpy, tx.win, tx.gc, x_rand_coordinates,
                         y_rand_coordinates, square_width, square_height);
          XFlush(tx.dpy);

          user_input[user_input_length++] = (char)keysym;
          user_input[user_input_length + 1] = '\0';
          number_of_squares++;
          XFlush(tx.dpy);
        } else if (keysym == XK_BackSpace && user_input_length > 0) {
          purge_sq(&tx);
        } else if (keysym == XK_Return) {
          if (verify_passwd(user_input) == 0) {
            XCloseDisplay(tx.dpy);
            return EXIT_SUCCESS;
          }
        }
        break;
      case FocusOut:
        XGrabKeyboard(tx.dpy, tx.win, True, GrabModeAsync, GrabModeAsync,
                      CurrentTime);
        break;
    }
  }

  XCloseDisplay(tx.dpy);

  return EXIT_SUCCESS;
}
