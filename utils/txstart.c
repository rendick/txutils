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

    sprintf(exec_plugin_cmd, "sh %s/%s.sh %s", path_to_config, cmd_copy,
            plugin_args);
    rc = popen(exec_plugin_cmd, "r");
    if (rc == NULL)
      die("rc error");

    while (fgets(output, 4096, rc) != NULL) {
      printf("%s\n", output);
      XDrawString(dpy, win, gc, 150,
                  150 + new_lines_output++ * strhei(font_struct), output,
                  strlen(output));
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
  Display *dpy;
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
  win = XCreateWindow(dpy, root, (DisplayWidth(dpy, screen) - 600) / 2,
                      (DisplayHeight(dpy, screen) - 300) / 2, 600, 300, 0,
                      DefaultDepth(dpy, screen), CopyFromParent,
                      DefaultVisual(dpy, screen),
                      CWOverrideRedirect | CWBackPixel, &attrs);
  gc = XCreateGC(dpy, win, 0, NULL);
  font_struct = XQueryFont(dpy, XGContextFromGC(gc));

  XSetForeground(dpy, gc, foreground.extra);
  XSetFillStyle(dpy, gc, FillSolid);

  rect_gc = XCreateGC(dpy, win, 0, NULL);
  XColor rect_color;
  XSetForeground(dpy, rect_gc, 0xff0000);
  XSetFillStyle(dpy, rect_gc, FillSolid);

  XStoreName(dpy, win, "txstart");
  XSelectInput(dpy, win, StructureNotifyMask | KeyPressMask | FocusChangeMask);
  XMapWindow(dpy, win);
  XGrabKeyboard(dpy, win, True, GrabModeAsync, GrabModeAsync, CurrentTime);

  if (XGetWindowAttributes(dpy, win, &attr)) {
    width = attr.width;
    height = attr.height;
  }

  char *path2conf = txconf(name, "");
  printf("out: %s\n", path2conf);
  XFillRectangle(dpy, win, rect_gc, 0, 0, width, 100);
  XFlush(dpy);
  while (1) {
    XEvent event;
    XNextEvent(dpy, &event);

    switch (event.type) {
    case Expose:
      XFillRectangle(dpy, win, rect_gc, 0, 0, width, 100);
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
        XFillRectangle(dpy, win, rect_gc, 0, 0, width, 100);
        XFlush(dpy);

        break;
      }

      if (keysym >= 32 && keysym < 127 && user_input_length < INPUT_LENGTH) {
        user_input[user_input_length] = (char)keysym;
        user_input[user_input_length + 1] = '\0';
        user_input_length++;
        printf("%s\n", user_input);

        XClearWindow(dpy, win);
        XFillRectangle(dpy, win, rect_gc, 0, 0, width, 100);
        XDrawString(dpy, win, gc, input.x, input.y, user_input,
                    user_input_length);
        XFlush(dpy);
      } else if (keysym == XK_BackSpace && user_input_length > 0) {
        user_input[user_input_length - 1] = '\0';
        user_input_length--;

        XClearWindow(dpy, win);
        printf("backspace: %s\n", user_input);
        XFillRectangle(dpy, win, rect_gc, 0, 0, width, 100);
        XDrawString(dpy, win, gc, input.x, input.y, user_input,
                    user_input_length);
        XFlush(dpy);
      } else if (keysym == XK_Return && user_input_length > 0) {
        char cmd_existance_checker[100];
        sprintf(cmd_existance_checker, "which %s > /dev/null 2>&1", user_input);

        printf("return: %s\n", user_input);
        if (system(cmd_existance_checker)) {
          printf("system: %s\n", user_input);
          run_plugin(dpy, win, gc, font_struct, user_input);
          printf("DOES NOT EXIST\n");
          XDrawString(dpy, win, gc, 100, 100, "Does not exist",
                      strlen("Does not exist"));
          XFillRectangle(dpy, win, rect_gc, 0, 0, width, 100);
          XFlush(dpy);
        } else {
          run_program(user_input);
        }
      }
    }
  }

  XCloseDisplay(dpy);
  return 0;
}
