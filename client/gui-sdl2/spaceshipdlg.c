/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
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

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "packets.h"
#include "victory.h"

/* client */
#include "client_main.h"
#include "text.h"

/* gui-sdl2 */
#include "graphics.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "widget.h"

#include "spaceshipdlg.h"

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct small_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct small_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list = NULL;
static bool dialog_list_has_been_initialised = FALSE;

/**********************************************************************//**
  Find spaceship dialog related to specified player.
**************************************************************************/
static struct small_dialog *get_spaceship_dialog(struct player *pplayer)
{
  if (!dialog_list_has_been_initialised) {
    dialog_list = dialog_list_new();
    dialog_list_has_been_initialised = TRUE;
  }

  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->end_widget_list->data.player == pplayer) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/**********************************************************************//**
  User interacted with spaceship dialog window.
**************************************************************************/
static int space_dialog_window_callback(struct widget *pwindow)
{
  if (PRESSED_EVENT(main_data.event)) {
    move_window_group(pwindow->private_data.small_dlg->begin_widget_list, pwindow);
  }

  return -1;
}

/**********************************************************************//**
  User interacted with spaceship dialog close button.
**************************************************************************/
static int exit_space_dialog_callback(struct widget *pwidget)
{
  if (PRESSED_EVENT(main_data.event)) {
    popdown_spaceship_dialog(pwidget->data.player);
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with spaceship dialog launch button.
**************************************************************************/
static int launch_spaceship_callback(struct widget *pwidget)
{
  if (PRESSED_EVENT(main_data.event)) {
    send_packet_spaceship_launch(&client.conn);
  }

  return -1;
}

/**********************************************************************//**
  Refresh (update) the spaceship dialog for the given player.
**************************************************************************/
void refresh_spaceship_dialog(struct player *pplayer)
{
  struct small_dialog *pSpaceShp;
  struct widget *pbuf;

  if (!(pSpaceShp = get_spaceship_dialog(pplayer))) {
    return;
  }

  /* launch button */
  pbuf = pSpaceShp->end_widget_list->prev->prev;
  if (victory_enabled(VC_SPACERACE)
      && pplayer == client.conn.playing
      && pplayer->spaceship.state == SSHIP_STARTED
      && pplayer->spaceship.success_rate > 0.0) {
    set_wstate(pbuf, FC_WS_NORMAL);
  }

  /* update text info */
  pbuf = pbuf->prev;
  copy_chars_to_utf8_str(pbuf->string_utf8,
                         get_spaceship_descr(&pplayer->spaceship));
  /* ------------------------------------------ */

  /* redraw */
  redraw_group(pSpaceShp->begin_widget_list, pSpaceShp->end_widget_list, 0);
  widget_mark_dirty(pSpaceShp->end_widget_list);

  flush_dirty();
}

/**********************************************************************//**
  Popup (or raise) the spaceship dialog for the given player.
**************************************************************************/
void popup_spaceship_dialog(struct player *pplayer)
{
  struct small_dialog *pSpaceShp;

  if (!(pSpaceShp = get_spaceship_dialog(pplayer))) {
    struct widget *buf, *pwindow;
    utf8_str *pstr;
    char cbuf[128];
    SDL_Rect area;

    pSpaceShp = fc_calloc(1, sizeof(struct small_dialog));

    fc_snprintf(cbuf, sizeof(cbuf), _("The %s Spaceship"),
                nation_adjective_for_player(pplayer));
    pstr = create_utf8_from_char(cbuf, adj_font(12));
    pstr->style |= TTF_STYLE_BOLD;

    pwindow = create_window_skeleton(NULL, pstr, 0);

    pwindow->action = space_dialog_window_callback;
    set_wstate(pwindow, FC_WS_NORMAL);
    pwindow->data.player = pplayer;
    pwindow->private_data.small_dlg = pSpaceShp;
    add_to_gui_list(ID_WINDOW, pwindow);
    pSpaceShp->end_widget_list = pwindow;

    area = pwindow->area;

    /* ---------- */
    /* create exit button */
    buf = create_themeicon(current_theme->small_cancel_icon, pwindow->dst,
                           WF_WIDGET_HAS_INFO_LABEL
                           | WF_RESTORE_BACKGROUND);
    buf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                            adj_font(12));
    buf->data.player = pplayer;
    buf->action = exit_space_dialog_callback;
    set_wstate(buf, FC_WS_NORMAL);
    buf->key = SDLK_ESCAPE;
    area.w = MAX(area.w, (buf->size.w + adj_size(10)));

    add_to_gui_list(ID_BUTTON, buf);

    buf = create_themeicon_button_from_chars(current_theme->ok_icon, pwindow->dst,
                                             _("Launch"), adj_font(12), 0);

    buf->action = launch_spaceship_callback;
    area.w = MAX(area.w, buf->size.w);
    area.h += buf->size.h + adj_size(20);
    add_to_gui_list(ID_BUTTON, buf);

    pstr = create_utf8_from_char(get_spaceship_descr(NULL), adj_font(12));
    pstr->bgcol = (SDL_Color) {0, 0, 0, 0};
    buf = create_iconlabel(NULL, pwindow->dst, pstr, WF_RESTORE_BACKGROUND);
    area.w = MAX(area.w, buf->size.w);
    area.h += buf->size.h + adj_size(20);
    add_to_gui_list(ID_LABEL, buf);

    pSpaceShp->begin_widget_list = buf;
    /* -------------------------------------------------------- */

    area.w = MAX(area.w, adj_size(300) - (pwindow->size.w - pwindow->area.w));

    resize_window(pwindow, NULL, NULL,
                  (pwindow->size.w - pwindow->area.w) + area.w,
                  (pwindow->size.h - pwindow->area.h) + area.h);

    area = pwindow->area;

    widget_set_position(pwindow,
                        (main_window_width() - pwindow->size.w) / 2,
                        (main_window_height() - pwindow->size.h) / 2);

    /* exit button */
    buf = pwindow->prev;
    buf->size.x = area.x + area.w - buf->size.w - 1;
    buf->size.y = pwindow->size.y + adj_size(2);

    /* launch button */
    buf = buf->prev;
    buf->size.x = area.x + (area.w - buf->size.w) / 2;
    buf->size.y = area.y + area.h - buf->size.h - adj_size(7);

    /* info label */
    buf = buf->prev;
    buf->size.x = area.x + (area.w - buf->size.w) / 2;
    buf->size.y = area.y + adj_size(7);

    dialog_list_prepend(dialog_list, pSpaceShp);

    refresh_spaceship_dialog(pplayer);
  } else {
    if (select_window_group_dialog(pSpaceShp->begin_widget_list,
                                   pSpaceShp->end_widget_list)) {
      widget_flush(pSpaceShp->end_widget_list);
    }
  }
}

/**********************************************************************//**
  Close the spaceship dialog for the given player.
**************************************************************************/
void popdown_spaceship_dialog(struct player *pplayer)
{
  struct small_dialog *pSpaceShp;

  if ((pSpaceShp = get_spaceship_dialog(pplayer))) {
    popdown_window_group_dialog(pSpaceShp->begin_widget_list,
                                pSpaceShp->end_widget_list);
    dialog_list_remove(dialog_list, pSpaceShp);
    FC_FREE(pSpaceShp);
  }
}
