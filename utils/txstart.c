#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <dirent.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
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
} txstart;

#define INPUT_LENGTH 512

char user_input[INPUT_LENGTH] = {0};
int user_input_length = 0;

void run_plugin(txstart* tx, char* cmd) {
  int exist_status = 1;
  struct dirent* de;

  char cmd_copy[256];
  strcpy(cmd_copy, cmd);
  char* cmd_args[512];
  int cmd_args_count = 0;

  char* token = strtok(cmd_copy, " ");
  while (token != NULL) {
    cmd_args[cmd_args_count++] = token;
    token = strtok(NULL, " ");
  }

  cmd_args[cmd_args_count] = NULL;

  char* path_to_config = txconf(name, "");
  printf("path to config: %s\n", path_to_config);

  DIR* dr = opendir(path_to_config);
  if (dr == NULL) die(__LINE__, "Failed to open directory");

  while ((de = readdir(dr)) != NULL) {
    if (strncmp(cmd_args[0], de->d_name, strlen(cmd_args[0]))) {
      exist_status = 0;
      break;
    }
  }

  if (exist_status == 0) {
    FILE* rc;

    char output[4096];
    int new_lines_output = 0;

    char exec_plugin_cmd[512];
    char plugin_args[256];

    for (int i = 1; i < cmd_args_count; i++) {
      strcat(plugin_args, cmd_args[i]);
      if (i != cmd_args_count) strcat(plugin_args, " ");
    }

    sprintf(exec_plugin_cmd, "sh %s%s.sh %s", path_to_config, cmd_copy,
            plugin_args);
    rc = popen(exec_plugin_cmd, "r");
    if (rc == NULL) die(__LINE__, "Failed to execute %s", exec_plugin_cmd);

    while (fgets(output, 4096, rc) != NULL) {
      puts(output);
      XDrawString(tx->dpy, tx->win, tx->gc, 100,
                  (window.heigth - strhei(tx->font_struct)) / 2 +
                      tx->font_struct->ascent,
                  output, strlen(output));
      XFlush(tx->dpy);
    }

    pclose(rc);

    output[0] = '\0';
    plugin_args[0] = '\0';
    new_lines_output = 0;
  }

  closedir(dr);

  path_to_config[0] = '\0';
}

void display_programs(txstart* tx, char* first_program) {
  struct dirent* de;

  DIR* dr = opendir("/usr/bin/");
  if (dr == NULL) die(__LINE__, "Failed to open /usr/bin/");

  long int x = 10 + strwid(user_input, tx->font_struct) + 5;
  int n = 0;
  while ((de = readdir(dr)) != NULL) {
    if (strncmp(user_input, de->d_name, user_input_length) == 0 &&
        de->d_type != DT_DIR) {
      if (n <= 100) {
        XDrawString(tx->dpy, tx->win, tx->gc, x,
                    (window.heigth - strhei(tx->font_struct)) / 2 +
                        tx->font_struct->ascent,
                    de->d_name, strlen(de->d_name));
        x += strwid(de->d_name, tx->font_struct) + 5;
      }

      if (n == 0) {
        first_program[0] = '\0';
        strcpy(first_program, de->d_name);
      }
      n++;
    }
  }
}

void run_program(char* name) {
  char* cmd_args[512];
  int cmd_args_count = 0;
  char* token = strtok(name, " ");
  while (token != NULL) {
    cmd_args[cmd_args_count++] = token;
    token = strtok(NULL, " ");
  }

  cmd_args[cmd_args_count] = NULL;

  execvp(cmd_args[0], cmd_args);
  perror("execvp failed");
  _exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
  txstart tx = {0};
  strcpy(name, txname());
  conf_analyzer(txname());
  verify_conf_args();

  if ((tx.dpy = XOpenDisplay(NULL)) == NULL)
    die(__LINE__, "Failed to open X11 display");

  XSetWindowAttributes attrs;
  attrs.override_redirect = True;
  attrs.background_pixel = background.extra;
  tx.root = DefaultRootWindow(tx.dpy);
  tx.screen = DefaultScreen(tx.dpy);
  tx.win = XCreateWindow(tx.dpy, tx.root, 0, 0, window.width, window.heigth, 0,
                         DefaultDepth(tx.dpy, tx.screen), CopyFromParent,
                         DefaultVisual(tx.dpy, tx.screen),
                         CWOverrideRedirect | CWBackPixel, &attrs);

  tx.gc = XCreateGC(tx.dpy, tx.win, 0, NULL);
  if (!tx.gc) die(__LINE__, "Failed to craete GC");

  tx.font_struct = XQueryFont(tx.dpy, XGContextFromGC(tx.gc));
  if (!tx.font_struct) die(__LINE__, "Failed to create font structure");

  XSetForeground(tx.dpy, tx.gc, foreground.extra);
  XSetFillStyle(tx.dpy, tx.gc, FillSolid);

  XStoreName(tx.dpy, tx.win, "txstart");
  XSelectInput(
      tx.dpy, tx.win,
      StructureNotifyMask | ExposureMask | KeyPressMask | FocusChangeMask);
  XMapWindow(tx.dpy, tx.win);
  XGrabKeyboard(tx.dpy, tx.win, True, GrabModeAsync, GrabModeAsync,
                CurrentTime);

  if (XGetWindowAttributes(tx.dpy, tx.win, &tx.attr)) {
    width = tx.attr.width;
    height = tx.attr.height;
  }

  char* path2conf = txconf(name, "");
  XFlush(tx.dpy);

  while (!XNextEvent(tx.dpy, &tx.event)) {
    KeySym keysym = XLookupKeysym(&tx.event.xkey, 0);
    char first_program[256];
    switch (tx.event.type) {
      case Expose:
        display_programs(&tx, first_program);
        XFlush(tx.dpy);
        break;
      case KeyPress:

        if (keysym == XK_Escape ||
            (tx.event.xkey.state & ControlMask) && keysym == XK_c) {
          die(__LINE__, "Exiting...");
        } else if ((tx.event.xkey.state & ControlMask) && keysym == XK_u) {
          first_program[0] = '\0';
          user_input[0] = '\0';
          user_input_length = 0;

          XClearWindow(tx.dpy, tx.win);
          display_programs(&tx, first_program);
          XFlush(tx.dpy);

          break;
        }

        if (keysym >= 32 && keysym < 127 && user_input_length < INPUT_LENGTH) {
          user_input[user_input_length] = (char)keysym;
          user_input[user_input_length + 1] = '\0';
          user_input_length++;

          XClearWindow(tx.dpy, tx.win);
          XDrawString(tx.dpy, tx.win, tx.gc, 10,
                      (window.heigth - strhei(tx.font_struct)) / 2 +
                          tx.font_struct->ascent,
                      user_input, user_input_length);

          display_programs(&tx, first_program);
          XFlush(tx.dpy);
        } else if (keysym == XK_BackSpace && user_input_length > 0) {
          user_input[user_input_length - 1] = '\0';
          user_input_length--;

          XClearWindow(tx.dpy, tx.win);
          XDrawString(tx.dpy, tx.win, tx.gc, 10,
                      (window.heigth - strhei(tx.font_struct)) / 2 +
                          tx.font_struct->ascent,
                      user_input, user_input_length);
          display_programs(&tx, first_program);
          XFlush(tx.dpy);
        } else if (keysym == XK_Tab) {
          user_input[0] = '\0';
          strcpy(user_input, first_program);
          user_input_length = strlen(user_input);
          printf("%s, %s\n", first_program, user_input);
          XClearWindow(tx.dpy, tx.win);
          XDrawString(tx.dpy, tx.win, tx.gc, 10,
                      (window.heigth - strhei(tx.font_struct)) / 2 +
                          tx.font_struct->ascent,
                      user_input, user_input_length);

          XFlush(tx.dpy);
        } else if (keysym == XK_Return && user_input_length > 0) {
          char cmd_existance_checker[100];
          sprintf(cmd_existance_checker, "which %s > /dev/null 2>&1",
                  user_input);

          printf("return: %s\n", user_input);
          if (system(cmd_existance_checker)) {
            printf("system: %s\n", user_input);
            run_plugin(&tx, user_input);
          } else {
            run_program(user_input);
          }
        }
    }
  }

  XFreeGC(tx.dpy, tx.gc);
  XCloseDisplay(tx.dpy);

  return EXIT_SUCCESS;
}
