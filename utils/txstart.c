#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <linux/limits.h>

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../core.h"

#define INPUT_LENGTH 512

Display *dpy;
char user_input[INPUT_LENGTH] = {0};
uint16_t user_input_length = 0;

void run_plugin(Display *dpy, Window win, GC gc, XFontStruct *font_struct,
                char *cmd) {
  int exist_status = 1;
  struct dirent *de;

  char cmd_copy[256];
  strcpy(cmd_copy, cmd);
  char *cmd_args[512];
  int cmd_args_count = 0;
  char *token = strtok(cmd_copy, " ");
  while (token != NULL) {
    cmd_args[cmd_args_count++] = token;
    token = strtok(NULL, " ");
  }

  cmd_args[cmd_args_count] = NULL;

  char *path_to_config = txconf(name, "");
  printf("path to config: %s\n", path_to_config);

  DIR *dr = opendir(path_to_config);
  if (dr == NULL)
    die("Could not open directory");

  while ((de = readdir(dr)) != NULL) {
    if (strncmp(cmd_args[0], de->d_name, strlen(cmd_args[0]))) {
      exist_status = 0;
      break;
    }
  }

  if (exist_status == 0) {
    FILE *rc;

    char output[4096];
    int new_lines_output = 0;

    char exec_plugin_cmd[512];
    char plugin_args[256];

    for (int i = 1; i < cmd_args_count; i++) {
      strcat(plugin_args, cmd_args[i]);
      if (i != cmd_args_count)
        strcat(plugin_args, " ");
    }

    sprintf(exec_plugin_cmd, "sh %s%s.sh %s", path_to_config, cmd_copy,
            plugin_args);
    rc = popen(exec_plugin_cmd, "r");
    if (rc == NULL)
      die("rc error");

    while (fgets(output, 4096, rc) != NULL) {
      puts(output);
      XDrawString(dpy, win, gc, 100,
                  (window.heigth - strhei(font_struct)) / 2 +
                      font_struct->ascent,
                  output, strlen(output));
      XFlush(dpy);
    }

    pclose(rc);

    output[0] = '\0';
    plugin_args[0] = '\0';
    new_lines_output = 0;
  }

  closedir(dr);

  path_to_config[0] = '\0';
}

void display_programs(Display *dpy, Window win, GC gc,
                      XFontStruct *font_struct) {
  struct dirent *de;

  DIR *dr = opendir("/usr/bin/");
  if (dr == NULL)
    die("Could not open directory");

  long int x = 10 + strwid(user_input, font_struct) + 5;
  int n = 0;
  while ((de = readdir(dr)) != NULL) {
    if (strncmp(user_input, de->d_name, user_input_length) == 0 &&
        de->d_type != DT_DIR) {
      if (n <= 100) {
        XDrawString(dpy, win, gc, x,
                    (window.heigth - strhei(font_struct)) / 2 +
                        font_struct->ascent,
                    de->d_name, strlen(de->d_name));
        x += strwid(de->d_name, font_struct) + 5;
      }
      n++;
    }
  }
}

void run_program(char *name) {
  char *cmd_args[512];
  int cmd_args_count = 0;
  char *token = strtok(name, " ");
  while (token != NULL) {
    cmd_args[cmd_args_count++] = token;
    token = strtok(NULL, " ");
  }

  cmd_args[cmd_args_count] = NULL;

  execvp(cmd_args[0], cmd_args);
  perror("execvp failed");
  _exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  strcpy(name, txname());
  conf_analyzer(txname());
  verify_conf_args();

  Window win, root;
  GC gc, rect_gc;
  XFontStruct *font_struct;
  XWindowAttributes attr;
  int screen;

  if ((dpy = XOpenDisplay(NULL)) == NULL)
    die("Cannot open X11 display");

  XSetWindowAttributes attrs;
  attrs.override_redirect = True;
  attrs.background_pixel = background.extra;
  root = DefaultRootWindow(dpy);
  screen = DefaultScreen(dpy);
  win = XCreateWindow(dpy, root, 0, 0, window.width, window.heigth, 0,
                      DefaultDepth(dpy, screen), CopyFromParent,
                      DefaultVisual(dpy, screen),
                      CWOverrideRedirect | CWBackPixel, &attrs);

  gc = XCreateGC(dpy, win, 0, NULL);
  font_struct = XQueryFont(dpy, XGContextFromGC(gc));

  XSetForeground(dpy, gc, foreground.extra);
  XSetFillStyle(dpy, gc, FillSolid);

  XStoreName(dpy, win, "txstart");
  XSelectInput(dpy, win,
               StructureNotifyMask | ExposureMask | KeyPressMask |
                   FocusChangeMask);
  XMapWindow(dpy, win);
  XGrabKeyboard(dpy, win, True, GrabModeAsync, GrabModeAsync, CurrentTime);

  if (XGetWindowAttributes(dpy, win, &attr)) {
    width = attr.width;
    height = attr.height;
  }

  char *path2conf = txconf(name, "");
  XFlush(dpy);
  while (1) {
    XEvent event;
    XNextEvent(dpy, &event);

    switch (event.type) {
    case Expose:
      display_programs(dpy, win, gc, font_struct);
      XFlush(dpy);
      break;
    case KeyPress:
      KeySym keysym = XLookupKeysym(&event.xkey, 0);

      if (keysym == XK_Escape ||
          (event.xkey.state & ControlMask) && keysym == XK_c) {
        die("");
      } else if ((event.xkey.state & ControlMask) && keysym == XK_u) {
        user_input[0] = '\0';
        user_input_length = 0;

        XClearWindow(dpy, win);
        XFlush(dpy);

        break;
      }

      if (keysym >= 32 && keysym < 127 && user_input_length < INPUT_LENGTH) {
        user_input[user_input_length] = (char)keysym;
        user_input[user_input_length + 1] = '\0';
        user_input_length++;

        XClearWindow(dpy, win);
        XDrawString(dpy, win, gc, 10,
                    (window.heigth - strhei(font_struct)) / 2 +
                        font_struct->ascent,
                    user_input, user_input_length);

        display_programs(dpy, win, gc, font_struct);
        XFlush(dpy);
      } else if (keysym == XK_BackSpace && user_input_length > 0) {
        user_input[user_input_length - 1] = '\0';
        user_input_length--;

        XClearWindow(dpy, win);
        XDrawString(dpy, win, gc, 10,
                    (window.heigth - strhei(font_struct)) / 2 +
                        font_struct->ascent,
                    user_input, user_input_length);
        display_programs(dpy, win, gc, font_struct);
        XFlush(dpy);
      } else if (keysym == XK_Return && user_input_length > 0) {
        char cmd_existance_checker[100];
        sprintf(cmd_existance_checker, "which %s > /dev/null 2>&1", user_input);

        printf("return: %s\n", user_input);
        if (system(cmd_existance_checker)) {
          printf("system: %s\n", user_input);
          run_plugin(dpy, win, gc, font_struct, user_input);
        } else {
          run_program(user_input);
        }
      }
    }
  }

  XCloseDisplay(dpy);
  return 0;
}
