#include <X11/Xlib.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define UNIX_USER getenv("USER")
#define randnum(max, min) ((rand() % ((max) - (min) + 1)) + (min))

int width = 0, height = 0;

char name[128];

struct Variable {
  int x;
  int y;
  int width;
  int heigth;
  uint32_t extra;
};

struct Variable background;
struct Variable foreground;
struct Variable input;
struct Variable input_foreground;
struct Variable square;

void die(char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

int strwid(char *str, XFontStruct *font_struct) {
  return XTextWidth(font_struct, str, strlen(str));
}

int strhei(XFontStruct *font_info) {
  return font_info->ascent + font_info->descent;
}

char *txconf(char *util_name, char *args) {
  char copy_util_name[100];
  strcpy(copy_util_name, util_name);

  char not_needed_chars[10] = "./,";
  for (int i = 0; i < strlen(copy_util_name); i++) {
    for (int j = 0; j < strlen(not_needed_chars); j++) {
      if (not_needed_chars[j] == copy_util_name[i]) {
        memmove(copy_util_name + i, copy_util_name + i + 1,
                strlen(copy_util_name) - i);
      }
    }
  }
  static char path_to_config[512];
  sprintf(path_to_config, "/home/%s/.config/%s/%s", UNIX_USER, copy_util_name,
          args);
  return path_to_config;
}

char *txname() {
  static char name[256];
  static char path[4096];
  ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (len != 1) {
    path[len] = '\0';
  } else {
    perror("readlink()");
  }

  int slash_n = 0;

  for (int i = 0; i < strlen(path); i++) {
    if (path[i] == '/') {
      slash_n++;
    }
  }

  int j = 0;
  char *token = strtok(path, "/");
  while (token != NULL) {
    if (j++ == slash_n - 1) {
      strcpy(name, token);
    }
    token = strtok(NULL, "/");
  }
  return name;
}

void set_values(struct Variable *var, int arg_n, char *token) {
  switch (arg_n) {
  case 1:
    var->x = atoi(token);
  case 2:
    var->y = atoi(token);
  case 3:
    if (strcmp(token, "MAX") == 0)
      var->width = width;
    else
      var->width = atoi(token);
  case 4:
    if (strcmp(token, "MAX") == 0)
      var->heigth = height;
    else
      var->heigth = atoi(token);
  case 5:
    var->extra = strtol(token, NULL, 0);
  }
}

void conf_analyzer(char *util_name) {
  char copy_util_name[100];
  strcpy(copy_util_name, util_name);

  char not_needed_chars[10] = "./,";
  for (int i = 0; i < strlen(copy_util_name); i++) {
    for (int j = 0; j < strlen(not_needed_chars); j++) {
      if (not_needed_chars[j] == copy_util_name[i]) {
        memmove(copy_util_name + i, copy_util_name + i + 1,
                strlen(copy_util_name) - i);
      }
    }
  }

  char path_to_config[512];
  sprintf(path_to_config, "/home/%s/.config/%s/config", UNIX_USER,
          copy_util_name);
  printf("%s\n", path_to_config);

  char output[4096];
  int arg_n = 0;

  FILE *rf;
  rf = fopen(path_to_config, "r");

  while (fgets(output, 4096, rf)) {
    char option_type[100];
    char *token = strtok(output, " ={},");
    while (token != NULL) {
      if (strcmp(token, "\n") != 0) {
        printf("(%d) arg: %s\n", arg_n, token);
        if (arg_n == 0) {
          strcat(option_type, token);
        }

        if (strcmp(option_type, "background") == 0) {
          set_values(&background, arg_n, token);
        } else if (strcmp(option_type, "foreground") == 0) {
          set_values(&foreground, arg_n, token);
        } else if (strcmp(option_type, "input") == 0) {
          set_values(&input, arg_n, token);
        } else if (strcmp(option_type, "input_foreground") == 0) {
          set_values(&input_foreground, arg_n, token);
        } else if (strcmp(option_type, "square") == 0) {
          set_values(&square, arg_n, token);
        }

        arg_n++;
      }
      token = strtok(NULL, " ={},");
    }
    arg_n = 0;
    option_type[0] = '\0';
  }

  fclose(rf);
}
