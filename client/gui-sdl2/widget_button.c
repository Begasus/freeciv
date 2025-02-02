/***********************************************************************
 Freeciv - Copyright (C) 2006 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* utility */
#include "log.h"

/* gui-sdl2 */
#include "colors.h"
#include "graphics.h"
#include "gui_tilespec.h"
#include "themespec.h"

#include "widget.h"
#include "widget_p.h"

static int (*baseclass_redraw)(struct widget *pwidget);

/**********************************************************************//**
  Create Icon Button image with text and Icon then blit to Dest(ination)
  on position icon_button->size.x, icon_button->size.y.
  WARNING: pdest must exist.

  Text with attributes is taken from icob_button->string_utf8 parameter.

  Graphic for button is taken from icon_button->theme surface
  and blit to new created image.

  Graphic for Icon is taken from icon_button->theme2 surface and blit to new
  created image.

  function return (-1) if there are no Icon nor Text.
  Else return 0.
**************************************************************************/
static int redraw_ibutton(struct widget *icon_button)
{
  SDL_Rect dest = { 0, 0, 0, 0 };
  utf8_str TMPString;
  SDL_Surface *button = NULL, *text = NULL, *icon = icon_button->theme2;
  Uint16 Ix, Iy, x;
  Uint16 y = 0; /* FIXME: possibly uninitialized */
  int ret;

  ret = (*baseclass_redraw)(icon_button);
  if (ret != 0) {
    return ret;
  }

  if (icon_button->string_utf8 != NULL) {
    /* make copy of string_utf8 */
    TMPString = *icon_button->string_utf8;

    if (get_wstate(icon_button) == FC_WS_NORMAL) {
      TMPString.fgcol = *get_theme_color(COLOR_THEME_WIDGET_NORMAL_TEXT);
    } else if (get_wstate(icon_button) == FC_WS_SELECTED) {
      TMPString.fgcol = *get_theme_color(COLOR_THEME_WIDGET_SELECTED_TEXT);
      TMPString.style |= TTF_STYLE_BOLD;
    } else if (get_wstate(icon_button) == FC_WS_PRESSED) {
      TMPString.fgcol = *get_theme_color(COLOR_THEME_WIDGET_PRESSED_TEXT);
    } else if (get_wstate(icon_button) == FC_WS_DISABLED) {
      TMPString.fgcol = *get_theme_color(COLOR_THEME_WIDGET_DISABLED_TEXT);
    }

    text = create_text_surf_from_utf8(&TMPString);
  }

  if (!text && !icon) {
    return -1;
  }

  /* create Button graphic */
  button = create_bcgnd_surf(icon_button->theme, get_wstate(icon_button),
                             icon_button->size.w, icon_button->size.h);

  clear_surface(icon_button->dst->surface, &icon_button->size);
  alphablit(button, NULL, icon_button->dst->surface, &icon_button->size, 255);
  FREESURFACE(button);

  if (icon) { /* Icon */
    if (text) {
      if (get_wflags(icon_button) & WF_ICON_CENTER_RIGHT) {
        Ix = icon_button->size.w - icon->w - 5;
      } else {
        if (get_wflags(icon_button) & WF_ICON_CENTER) {
          Ix = (icon_button->size.w - icon->w) / 2;
        } else {
          Ix = 5;
        }
      }

      if (get_wflags(icon_button) & WF_ICON_ABOVE_TEXT) {
        Iy = 3;
        y = 3 + icon->h + 3 + (icon_button->size.h -
                               (icon->h + 6) - text->h) / 2;
      } else {
        if (get_wflags(icon_button) & WF_ICON_UNDER_TEXT) {
          y = 3 + (icon_button->size.h - (icon->h + 3) - text->h) / 2;
          Iy = y + text->h + 3;
        } else { /* center */
          Iy = (icon_button->size.h - icon->h) / 2;
          y = (icon_button->size.h - text->h) / 2;
        }
      }
    } else { /* no text */
      Iy = (icon_button->size.h - icon->h) / 2;
      Ix = (icon_button->size.w - icon->w) / 2;
    }

    if (get_wstate(icon_button) == FC_WS_PRESSED) {
      Ix += 1;
      Iy += 1;
    }

    dest.x = icon_button->size.x + Ix;
    dest.y = icon_button->size.y + Iy;

    ret = alphablit(icon, NULL, icon_button->dst->surface, &dest, 255);
    if (ret) {
      FREESURFACE(text);
      return ret - 10;
    }
  }

  if (text) {
    if (icon) {
      if (!(get_wflags(icon_button) & WF_ICON_ABOVE_TEXT)
          && !(get_wflags(icon_button) & WF_ICON_UNDER_TEXT)) {
        if (get_wflags(icon_button) & WF_ICON_CENTER_RIGHT) {
          if (icon_button->string_utf8->style & SF_CENTER) {
            x = (icon_button->size.w - (icon->w + 5) - text->w) / 2;
          } else {
            if (icon_button->string_utf8->style & SF_CENTER_RIGHT) {
              x = icon_button->size.w - (icon->w + 7) - text->w;
            } else {
              x = 5;
            }
          }
          /* end WF_ICON_CENTER_RIGHT */
        } else {
          if (get_wflags(icon_button) & WF_ICON_CENTER) {
            /* text is blit on icon */
            goto Alone;
            /* end WF_ICON_CENTER */
          } else { /* icon center left - default */
            if (icon_button->string_utf8->style & SF_CENTER) {
              x = 5 + icon->w + ((icon_button->size.w -
                                  (icon->w + 5) - text->w) / 2);
            } else {
              if (icon_button->string_utf8->style & SF_CENTER_RIGHT) {
                x = icon_button->size.w - text->w - 5;
              } else { /* text center left */
                x = 5 + icon->w + 3;
              }
            }
          } /* end icon center left - default */
        }
        /* 888888888888888888 */
      } else {
        goto Alone;
      }
    } else {
      /* !icon */
      y = (icon_button->size.h - text->h) / 2;
    Alone:
      if (icon_button->string_utf8->style & SF_CENTER) {
        x = (icon_button->size.w - text->w) / 2;
      } else {
        if (icon_button->string_utf8->style & SF_CENTER_RIGHT) {
          x = icon_button->size.w - text->w - 5;
        } else {
          x = 5;
        }
      }
    }

    if (get_wstate(icon_button) == FC_WS_PRESSED) {
      x += 1;
    } else {
      y -= 1;
    }

    dest.x = icon_button->size.x + x;
    dest.y = icon_button->size.y + y;

    ret = alphablit(text, NULL, icon_button->dst->surface, &dest, 255);
  }

  FREESURFACE(text);

  return 0;
}

/**********************************************************************//**
  Create Icon Button image with text and Icon then blit to Dest(ination)
  on position theme_icon_button->size.x, theme_icon_button->size.y.
  WARNING: pdest must exist.

  Text with attributes is taken from theme_icon_button->string_utf8 parameter.

  Graphic for button is taken from theme_icon_button->theme surface
  and blit to new created image.

  Graphic for Icon Theme is taken from theme_icon_button->theme2 surface
  and blit to new created image.

  function return (-1) if there are no Icon and Text.  Else return 0.
**************************************************************************/
static int redraw_tibutton(struct widget *theme_icon_button)
{
  int ret;
  SDL_Surface *icon;
  SDL_Surface *copy_of_icon_theme;

  ret = (*baseclass_redraw)(theme_icon_button);
  if (ret != 0) {
    return ret;
  }

  icon = create_icon_from_theme(theme_icon_button->theme2,
                                get_wstate(theme_icon_button));
  copy_of_icon_theme = theme_icon_button->theme2;

  theme_icon_button->theme2 = icon;

  ret = redraw_ibutton(theme_icon_button);

  FREESURFACE(theme_icon_button->theme2);
  theme_icon_button->theme2 = copy_of_icon_theme;

  return ret;
}

/**********************************************************************//**
  Create ( malloc ) Icon (theme)Button Widget structure.

  Icon graphic is taken from 'icon' surface (don't change with button
  changes );  Button Theme graphic is taken from current_theme->button surface;
  Text is taken from 'pstr'.

  This function determinate future size of Button ( width, height ) and
  save this in: pwidget->size rectangle ( SDL_Rect )

  function return pointer to allocated Button Widget.
**************************************************************************/
struct widget *create_icon_button(SDL_Surface *icon, struct gui_layer *pdest,
                                  utf8_str *pstr, Uint32 flags)
{
  SDL_Rect buf = {0, 0, 0, 0};
  int w = 0, h = 0;
  struct widget *button;

  if (!icon && !pstr) {
    return NULL;
  }

  button = widget_new();

  button->theme = current_theme->button;
  button->theme2 = icon;
  button->string_utf8 = pstr;
  set_wflag(button, (WF_FREE_STRING | flags));
  set_wstate(button, FC_WS_DISABLED);
  set_wtype(button, WT_I_BUTTON);
  button->mod = KMOD_NONE;
  button->dst = pdest;

  baseclass_redraw = button->redraw;
  button->redraw = redraw_ibutton;

  if (pstr) {
    button->string_utf8->style |= SF_CENTER;
    /* if BOLD == true then longest wight */
    if (!(pstr->style & TTF_STYLE_BOLD)) {
      pstr->style |= TTF_STYLE_BOLD;
      utf8_str_size(pstr, &buf);
      pstr->style &= ~TTF_STYLE_BOLD;
    } else {
      utf8_str_size(pstr, &buf);
    }

    w = MAX(w, buf.w);
    h = MAX(h, buf.h);
  }

  if (icon) {
    if (pstr) {
      if ((flags & WF_ICON_UNDER_TEXT) || (flags & WF_ICON_ABOVE_TEXT)) {
        w = MAX(w, icon->w + adj_size(2));
        h = MAX(h, buf.h + icon->h + adj_size(4));
      } else {
        w = MAX(w, buf.w + icon->w + adj_size(20));
        h = MAX(h, icon->h + adj_size(2));
      }
    } else {
      w = MAX(w, icon->w + adj_size(2));
      h = MAX(h, icon->h + adj_size(2));
    }
  } else {
    w += adj_size(10);
    h += adj_size(2);
  }

  correct_size_bcgnd_surf(current_theme->button, &w, &h);

  button->size.w = w;
  button->size.h = h;

  return button;
}

/**********************************************************************//**
  Create ( malloc ) Theme Icon (theme)Button Widget structure.

  Icon Theme graphic is taken from 'icon_theme' surface ( change with
  button changes ); Button Theme graphic is taken from current_theme->button
  surface; Text is taken from 'pstr'.

  This function determinate future size of Button ( width, height ) and
  save this in: pwidget->size rectangle ( SDL_Rect )

  function return pointer to allocated Button Widget.
**************************************************************************/
struct widget *create_themeicon_button(SDL_Surface *icon_theme,
                                       struct gui_layer *pdest,
                                       utf8_str *pstr,
                                       Uint32 flags)
{
  /* extract a single icon */
  SDL_Surface *icon = create_icon_from_theme(icon_theme, 1);
  struct widget *button = create_icon_button(icon, pdest, pstr, flags);

  FREESURFACE(button->theme2);
  button->theme2 = icon_theme;
  set_wtype(button, WT_TI_BUTTON);

  button->redraw = redraw_tibutton;

  return button;
}

/**********************************************************************//**
  Create Button image with text and Icon. Then blit to Main.screen on
  position start_x, start_y.

  Text with atributes is taken from button->string_utf8 parameter.

  Graphic for button is taken from button->theme surface and blit to new
  created image.

  Graphic for Icon theme is taken from button->theme2 surface and blit to
  new created image.

  function return (-1) if there are no Icon and Text.
  Else return 0.
**************************************************************************/
int draw_tibutton(struct widget *button, Sint16 start_x, Sint16 start_y)
{
  button->size.x = start_x;
  button->size.y = start_y;

  return redraw_tibutton(button);
}

/**********************************************************************//**
  Create Button image with text and Icon.
  Then blit to Main.screen on position start_x, start_y.

  Text with atributes is taken from button->string_utf8 parameter.

  Graphic for button is taken from button->theme surface
  and blit to new created image.

  Graphic for Icon is taken from button->theme2 surface
  and blit to new created image.

  function return (-1) if there are no Icon and Text.
  Else return 0.
**************************************************************************/
int draw_ibutton(struct widget *button, Sint16 start_x, Sint16 start_y)
{
  button->size.x = start_x;
  button->size.y = start_y;

  return redraw_ibutton(button);
}
