#ifndef CORE_H
#define CORE_H

#include <X11/Xlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIX_USER getenv("USER")
#define randnum(max, min) ((rand() % ((max) - (min) + 1)) + (min))

struct Variable {
  int x;
  int y;
  int width;
  int heigth;
  uint32_t extra;
};

extern int width, height;

extern char name[128];

extern struct Variable background;
extern struct Variable foreground;
extern struct Variable input;
extern struct Variable input_foreground;
extern struct Variable square;

void die(const char *msg);

int strwid(const char *str, XFontStruct *font_struct);
int strhei(XFontStruct *font_struct);

char *txconf(char *util_name, char *args);
char* txname();

void conf_analyzer(char *util_name);

#endif
