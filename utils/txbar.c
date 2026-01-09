#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../core.h"

#define MAX_MODULES 50

int number_of_modules = 0;
char* output_of_modules[MAX_MODULES];
int width_of_modules[MAX_MODULES] = {0};

char* separator = " | ";

typedef struct {
  char* command;
  char* output;
  int interval;
  long last_time;
} Module;

Module modules[MAX_MODULES];

long current_time_in_secs() {
  time_t rawtime;
  return time(&rawtime);
}

// get_workspace_information() gives all required data: the number of workspaces
// and the current workspace. Now all that remains is to implement its graphical
// implementation.
void get_workspace_information(Display* dpy, Window root) {
  Atom net_number_of_workspaces =
      XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
  Atom net_current_workspace = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
  Atom actual_type_return;
  int actual_format_return;
  unsigned long number_of_items, bytes_after_return;
  unsigned char* property_data = NULL;

  int number_of_workspaces = 0;
  if (XGetWindowProperty(dpy, root, net_number_of_workspaces, 0, 1, False,
                         XA_CARDINAL, &actual_type_return,
                         &actual_format_return, &number_of_items,
                         &bytes_after_return, &property_data) == 0 &&
      property_data) {
    number_of_workspaces = *(int*)property_data;
    XFree(property_data);
    printf("Number of workspaces: %d\n", number_of_workspaces);
  }

  int current_workspace_position = 0;
  if (XGetWindowProperty(dpy, root, net_current_workspace, 0, 1, False,
                         XA_CARDINAL, &actual_type_return,
                         &actual_format_return, &number_of_items,
                         &bytes_after_return, &property_data) == 0 &&
      property_data) {
    current_workspace_position = *(int*)property_data + 1;
    XFree(property_data);
    printf("Current workspace: %d\n", current_workspace_position);
  }
}

void add_module(char* command, int interval) {
  modules[number_of_modules].command = strdup(command);
  modules[number_of_modules].output = NULL;
  modules[number_of_modules].interval = interval;
  modules[number_of_modules].last_time = 0;
  number_of_modules++;
}

void update_modules() {
  int now = current_time_in_secs();
  for (int i = 0; i < number_of_modules; i++) {
    if (now - modules[i].last_time >= modules[i].interval) {
      FILE* run_command = popen(modules[i].command, "r");
      if (!run_command) perror("popen()");
      char buffer[1024];
      while (fgets(buffer, sizeof(buffer), run_command)) {
        if (strlen(buffer) > 0) buffer[strlen(buffer) - 1] = '\0';
        modules[i].output = strdup(buffer);
      }
      pclose(run_command);
      modules[i].last_time = now;
    }
  }
}

void draw_modules(Display* dpy, Window win, GC gc, XFontStruct* font_struct) {
  int free_screen_width = DisplayWidth(dpy, DefaultScreen(dpy));
  for (int i = 0; i < number_of_modules; i++) {
    XDrawString(dpy, win, gc,
                free_screen_width - strwid(modules[i].output, font_struct), 16,
                modules[i].output, strlen(modules[i].output));
    free_screen_width -= strwid(modules[i].output, font_struct);

    if (i != number_of_modules - 1) {
      XDrawString(dpy, win, gc,
                  free_screen_width - strwid(separator, font_struct), 16,
                  separator, strlen(separator));
      free_screen_width -= strwid(separator, font_struct);
    }
    XFlush(dpy);
  }
}

void parse_modules() {
  char* path_to_dir = txconf(name, "");
  struct dirent* de;
  DIR* read = opendir(path_to_dir);
  if (read == NULL) {
    die("Could not open current directory.");
  }
  while ((de = readdir(read)) != NULL) {
    if (strcmp(de->d_name, "modules") == 0) {
      char buffer[1024];

      char* path_to_modules = txconf(name, "modules");
      FILE* omf = fopen(path_to_modules, "r");
      while (fgets(buffer, sizeof(buffer), omf)) {
        char module_cmd[512];
        int delay;
        int n = 0;
        char* token = strtok(buffer, "|");
        while (token != NULL) {
          switch (n) {
            case 0:
              strcpy(module_cmd, token);
            case 1:
              delay = atoi(token);
          }
          n++;
          token = strtok(NULL, "|");
        }
        add_module(module_cmd, delay);
        module_cmd[0] = '\0';
        delay = 0;
        n = 0;
      }
      break;
    }
    printf("%s\n", de->d_name);
  }

  closedir(read);
}

int main(int argc, char** argv) {
  strcpy(name, txname());
  conf_analyzer(txname());

  Display* dpy;
  Window win, root;
  GC gc;
  Font font;
  XFontStruct* font_struct;

  if ((dpy = XOpenDisplay(NULL)) == NULL) die("Cannon open X11 display");

  root = DefaultRootWindow(dpy);
  win = XCreateSimpleWindow(dpy, root, 0, 0, 1, 22, 0, 0, background.extra);

  Atom wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
  Atom wm_window_type_dock =
      XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
  XChangeProperty(dpy, win, wm_window_type, XA_ATOM, 32, PropModeReplace,
                  (unsigned char*)&wm_window_type_dock, 1);

  gc = XCreateGC(dpy, win, 0, NULL);

  font_struct = XLoadQueryFont(dpy, "fixed");

  font = font_struct->fid;
  XSetFont(dpy, gc, font);

  XSetForeground(dpy, gc, foreground.extra);

  XSelectInput(dpy, win, StructureNotifyMask | ExposureMask);
  XMapWindow(dpy, win);

  parse_modules();

  long last_use = 0;
  for (;;) {
    XFlush(dpy);

    int now = current_time_in_secs();
    if (now != last_use) {
      XClearWindow(dpy, win);
      last_use = now;
    }

    update_modules();
    get_workspace_information(dpy, root);
    draw_modules(dpy, win, gc, font_struct);
    printf("%ld\n", current_time_in_secs());

    usleep(200 * 1000);  // milliseconds * 1000 (milliseconds to microseconds)
  }

  for (int i = 0; i < number_of_modules; i++) {
    free(modules[i].command);
    free(modules[i].output);
  }

  XFreeGC(dpy, gc);
  XDestroySubwindows(dpy, win);
  XCloseDisplay(dpy);

  return EXIT_SUCCESS;
}
