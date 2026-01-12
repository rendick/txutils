#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../core.h"

typedef struct {
  Display* dpy;
  Window win, root;
  GC gc;
  XEvent event;
  XFontStruct* font_struct;
  XWindowAttributes attr;
  int screen;
} txdm;

#define INPUT_LENGTH 512

char user_input[INPUT_LENGTH] = {0};
uint16_t user_input_length = 0;

char (*get_envs(int* count))[2][256] {
  (*count) = 0;

  char env_dir[] = "/usr/share/xsessions/";
  static char envs[10][2][256];

  struct dirent* de;

  DIR* rd = opendir(env_dir);
  if (rd == NULL) die(__LINE__, "Could not open %s", env_dir);

  while ((de = readdir(rd)) != NULL) {
    if (de->d_type != DT_DIR) {
      char line[1024];
      char file_path[256];
      snprintf(file_path, sizeof(file_path), "%s%s", env_dir, de->d_name);

      char name[256] = "";
      char exec[256] = "";

      FILE* rf = fopen(file_path, "r");
      while (fgets(line, 1024, rf)) {
        line[strlen(line) - 1] = '\0';
        if (strncmp(line, "Name=", strlen("Name=")) == 0) {
          strcpy(name, line + 5);
        } else if (strncmp(line, "Exec=", strlen("Exec=")) == 0) {
          strcpy(exec, line + 5);
        }
      }
      fclose(rf);

      (*count)++;
      if (*count <= 10) {
        for (int i = *count - 1; i < 10; i++) {
          strcpy(envs[i][0], name);
          strcpy(envs[i][1], exec);
        }
      }
    }
  }
  closedir(rd);

  return envs;
}

int main(void) {
  txdm tx = {0};

  if ((tx.dpy = XOpenDisplay(NULL)) == NULL)
    die(__LINE__, "Failed to open X11 display");

  XSetWindowAttributes attrs;
  attrs.background_pixel = 0xffffff;
  tx.root = DefaultRootWindow(tx.dpy);
  tx.screen = DefaultScreen(tx.dpy);
  tx.win = XCreateWindow(tx.dpy, tx.root, 0, 0, DisplayWidth(tx.dpy, tx.screen),
                         DisplayHeight(tx.dpy, tx.screen), 0,
                         DefaultDepth(tx.dpy, tx.screen), CopyFromParent,
                         DefaultVisual(tx.dpy, tx.screen),
                         CWOverrideRedirect | CWBackPixel, &attrs);
  tx.gc = XCreateGC(tx.dpy, tx.win, 0, NULL);
  tx.font_struct = XQueryFont(tx.dpy, XGContextFromGC(tx.gc));

  XStoreName(tx.dpy, tx.win, "txdm");
  XSelectInput(
      tx.dpy, tx.win,
      StructureNotifyMask | ExposureMask | KeyPressMask | FocusChangeMask);
  XMapWindow(tx.dpy, tx.win);
  XGrabKeyboard(tx.dpy, tx.win, True, GrabModeAsync, GrabModeAsync,
                CurrentTime);

  while (1) {
    XEvent event;
    XNextEvent(tx.dpy, &event);
    switch (event.type) {
        // case Expose:
        //   static int count;
        //   char (*envs)[2][256] = get_envs(&count);
        //   printf("%d\n", count);

        //   int x = 100;
        //   int y = 100;
        //   for (int i = 0; i < count; i++) {
        //     printf("%d: %s\n", i, envs[i][0]);
        //     XDrawString(tx.dpy, tx.win, tx.gc, x, y, envs[i][0],
        //     strlen(envs[i][0])); y += strhei(tx.font_struct) + 1;
        //   }
        //   XFlush(tx.dpy);
        //   break;

      case KeyPress:

        KeySym keysym = XLookupKeysym(&event.xkey, 0);
        if (keysym >= 32 && keysym < 127 && user_input_length < INPUT_LENGTH) {
          user_input[user_input_length] = (char)keysym;
          user_input[user_input_length + 1] = '\0';
          user_input_length++;
          printf("%s\n", user_input);
          XClearWindow(tx.dpy, tx.win);
          XDrawString(tx.dpy, tx.win, tx.gc, 50, 50, user_input,
                      user_input_length);
          XFlush(tx.dpy);
        } else if (keysym == XK_BackSpace && user_input_length > 0) {
          user_input[user_input_length - 1] = '\0';
          user_input_length--;

          XClearWindow(tx.dpy, tx.win);
          printf("backspace: %s\n", user_input);
          XFillRectangle(tx.dpy, tx.win, tx.gc, 0, 0, width, 100);
          XDrawString(tx.dpy, tx.win, tx.gc, 50, 50, user_input,
                      user_input_length);
          XFlush(tx.dpy);
        } else if (keysym == XK_Return) {
          int count;
          char (*envs)[2][256] = get_envs(&count);
          printf("%d\n", count);

          for (int i = 0; i < count; i++) {
            if (strcmp(user_input, envs[i][0]) == 0) {
              execlp(envs[i][0], envs[i][0], NULL);
            }
          }
        }
        break;
    }
    int count;
    char (*envs)[2][256] = get_envs(&count);
    printf("%d\n", count);

    int x = 100;
    int y = 100;
    for (int i = 0; i < count; i++) {
      printf("%d: %s\n", i, envs[i][0]);
      XDrawString(tx.dpy, tx.win, tx.gc, x, y, envs[i][0], strlen(envs[i][0]));
      y += strhei(tx.font_struct) + 1;
    }

    XFlush(tx.dpy);
  }

  return EXIT_SUCCESS;
}
