/*
** mouse:~ppr/src/include/filter_dotmatrix.h
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 2 August 1999.
*/

/* If defined, PostScript comments are included to assist in debuging. */
/* #define DEBUG_COMMENTS 1 */

#define INCH 72.0

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !FALSE
#endif

/* Possible duplex modes: */
#define DUPLEX_UNDEF 0
#define DUPLEX_NONE 1
#define DUPLEX_DUPLEX 2
#define DUPLEX_TUMBLE 3

/* Procedure set names */
#define DOTMATRIX "(TrinColl-PPR-Dotmatrix) 1 2"
#define DOTMATRIXG1 "(TrinColl-PPR-Dotmatrix-G1) 1 0"
#define DOTMATRIXG2 "(TrinColl-PPR-Dotmatrix-G2) 1 0"
#define REENCODE "(TrinColl-PPR-ReEncode) 1.1 0"
#define METRICSEPSON1 "(TrinColl-PPR-Dotmatrix-MetricsEpson1) 1 0"
#define METRICSEPSON2 "(TrinColl-PPR-Dotmatrix-MetricsEpson2) 1 0"
#define METRICSEPSON3 "(TrinColl-PPR-Dotmatrix-MetricsEpson3) 1 0"
#define METRICSEPSON4 "(TrinColl-PPR-Dotmatrix-MetricsEpson4) 1 0"
#define NEWMETRICS "(TrinColl-PPR-NewMetrics) 1 0"
#define COLOUR "(TrinColl-PPR-Dotmatrix-Colour) 1 0"

/* Size of input buffer. */
#define INPUT_BUFFER_SIZE 8192

/* These are the master mode select codes. */
#define MODE_15PITCH 512		/* my hack */
#define MODE_3X_HORIZONTAL 1024		/* my hack */
#define MODE_4X_HORIZONTAL 2048		/* my hack */
#define MODE_2X_VERTICAL 4096		/* my hack */
#define MODE_4X_VERTICAL 8192		/* my hack */
#define MODE_2X_VERTICAL_BASELINE 16384	/* my hack */
#define MODE_UNDERLINE 128
#define MODE_ITALIC 64
#define MODE_EXPANDED 32
#define MODE_DOUBLE_STRIKE 16
#define MODE_EMPHASIZED 8
#define MODE_CONDENSED 4
#define MODE_PROPORTIONAL 2
#define MODE_ELITE 1
#define MODE_PICA 0

/* The graphics and italic character sets. */
#define CHARSET_ITALIC 0
#define CHARSET_EXTENDED 1

/* NLQ fonts for variable "nlq_font". */
#define NLQ_ROMAN 0			/* Courier */
#define NLQ_SANS_SERIF 1
#define NLQ_ORATOR_SMALL_CAPS 2
#define NLQ_ORATOR 3

/* Superscript and Subscript modes */
#define SCRIPT_NONE 0
#define SCRIPT_SUPER 1
#define SCRIPT_SUB 2

/* How wide are various modes? */
/* All of these must be floating point numbers. */
#define FACTOR_ELITE (10.0/12.0)
#define FACTOR_CONDENSED (10.0/17.0)
#define FACTOR_EXPANDED 2.0
#define FACTOR_SCRIPT 0.5
#define FACTOR_15PITCH (10.0/15.0)

/* The graphics modes. */
#define GRAPHICS_60   0
#define GRAPHICS_120a 1
#define GRAPHICS_120b 2
#define GRAPHICS_240  3
#define GRAPHICS_80   4
#define GRAPHICS_72   5
#define GRAPHICS_90   6
#define GRAPHICS_24_60 32
#define GRAPHICS_24_120 33
#define GRAPHICS_24_90 38
#define GRAPHICS_24_180 39
#define GRAPHICS_24_360 40
#define PINS_8or24 0
#define PINS_9 1

/* Output style descriptions for communication between */
/* main.c and linebuf.c */
#define OSTYLE_UNDERLINE 1		/* Underline */
#define OSTYLE_OBLIQUE 2		/* Italic */
#define OSTYLE_BOLD 4			/* for double strike and emphasized */
#define OSTYLE_HALFRAISE 8		/* for superscript */
#define OSTYLE_FULLDROP 16		/* for double height characters */
#define OSTYLE_PROPORTIONAL 32		/* for proportional type */

/* Various printers we can emulate.  It may be ok to enable multiple */
/* emulations to include features from several printers. */
#define EMULATION_CONFLICTING_IBM 1	/* Proprinter differences */
#define EMULATION_P6_INTERPRETATION 4
#define EMULATION_24PIN_UNITS 8
#define EMULATION_8IN_LINE 16		/* Narrow carriage */

/* Print colours. */
#define COLOUR_BLACK 0
#define COLOUR_MAGENTA 1
#define COLOUR_CYAN 2
#define COLOUR_VIOLET 3
#define COLOUR_YELLOW 4
#define COLOUR_ORANGE 5
#define COLOUR_GREEN 6
#define COLOUR_BROWN 7

/* PostScript encodings */
#define ENCODING_STANDARD 0
#define ENCODING_CP437 1
#define ENCODING_ISOLATIN1 2

/* Globals in main.c */
extern int noisy;
extern int colour_ok;
extern int level2;
extern int encoding;
extern int HORIZONTAL_UNITS;
extern int VERTICAL_UNITS;
extern int VFACTOR;
extern double phys_pu_width;
extern double phys_pu_height;
extern char *PageSize;
extern const char *MediaColor;
extern const char *MediaType;
extern double MediaWeight;
extern int duplex_mode;
extern int page_width;
extern int page_length;
extern int xshift;
extern int yshift;
extern int current_char_spacing;
extern int current_line_spacing;
extern int line_feed_direction;
extern int line_spacing_multiplier;
extern int xpos,ypos;
extern double postscript_xpos;
extern int postscript_ypos;
extern int postscript_print_colour;
extern int in_page;
extern int current_page;
extern int current_charmode;
extern int script_mode;
extern int justification;
extern int international_char_set;
extern int nlq_mode;
extern int nlq_font;
extern int extra_dot_spacing;
extern int one_line_expanded;		/* for ESC SO */
extern int simple_compressed;		/* for ESC SI */
extern int charset;
extern int emode;
extern int print_colour;
extern int out_style;
extern double out_vscale;
extern double out_hscale;
extern int emulation;
extern int upper_controls;
extern int graphic_mode_K;
extern int graphic_mode_L;
extern int graphic_mode_Y;
extern int graphic_mode_Z;
extern int tabs_vertical[8][16];
extern int tabs_horizontal[32];
extern int vertical_tab_channel;
extern int left_margin;
extern int right_margin;
extern int perforation_skip;
extern int top_margin;
extern int auto_lf;
extern int auto_cr;

/* in main.c */
void achieve_position(void);
void line_feed(int howfar);
void reset(int hard);
void select_font(void);

/* in linebuf.c */
void buffer_top_of_page_reset(void);
void buffer_delete(int howmuch);
void buffer_add(int c);
void empty_buffer(void);
void string_break(void);

/* in postscript.c */
void top_of_document(void);
void bottom_of_document(void);
void top_of_page(void);
void bottom_of_page(void);
void achive_position(void);

/* In tabbing.c */
void reset_tabs(void);
void vertical_tab(void);
void horizontal_tab(void);
void horizontal_tabs_set(void);
void vertical_tabs_set(int channel);
void horizontal_tab_increment(int inc);
void vertical_tab_increment(int inc);

/* In escape.c */
void escape(void);
void fs(void);
void escape_pass1(void);
void fs_pass1(void);

/* In graphics.c */
void graphic(int mode, int pins, int length);
void eat_graphic(int mode, int pins, int length);

/* In inbuf.c */
int input(void);
extern int set8th;
extern int clear8th;
void rewind_input(void);

/* In pass1.c */
extern int uses_proportional1;
extern int uses_proportional2;
extern int uses_proportional3;
extern int uses_proportional4;
extern int uses_graphics;
extern int uses_colour;
extern int uses_24pin_commands;
extern int uses_normal;
extern int uses_bold;
extern int uses_oblique;
extern int uses_boldoblique;
extern int uses_nonascii_normal;
extern int uses_nonascii_bold;
extern int uses_nonascii_oblique;
extern int uses_nonascii_boldoblique;
void pass1(void);

/* In prop.c */
int width(int c, int italic);

/* end of file */
