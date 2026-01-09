#ifndef CORE_H
#define CORE_H

#include <X11/Xlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define UNIX_USER getenv("USER")
#define randnum(max, min) ((rand() % ((max) - (min) + 1)) + (min))

extern uint32_t defbg;
extern uint32_t deffg;

extern uint32_t window_width;
extern uint32_t window_height;

extern uint32_t bg_color;
extern uint32_t fg_color;

extern uint32_t square_color;
extern uint16_t square_width;
extern uint16_t square_height;

extern int width, height;

extern char name[128];

struct Variable {
  int x;
  int y;
  int width;
  int heigth;
  uint32_t extra;
};

extern struct Variable background;
extern struct Variable foreground;
extern struct Variable input;
extern struct Variable input_foreground;
extern struct Variable square;
extern struct Variable window;

void die(int line_number, const char* msg, ...);

int strwid(const char* str, XFontStruct* font_struct);
int strhei(XFontStruct* font_struct);

void verify_conf_args();

char* txconf(char* util_name, char* args);
char* txname();

void conf_analyzer(char* util_name);

#endif
