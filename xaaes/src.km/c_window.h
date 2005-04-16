/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _c_window_h
#define _c_window_h

#include "global.h"
#include "xa_types.h"
#include "xa_global.h"

struct xa_window *create_window(enum locks lock,
				SendMessage *message_handler,
				DoWinMesag *message_doer,
				struct xa_client *client,
				bool nolist,
				XA_WIND_ATTR tp,
				WINDOW_TYPE dial,
				int thinframe,
				bool thinwork,
				const RECT r, const RECT *max, RECT *rem);

void change_window_attribs(enum locks lock,
			   struct xa_client *client,
			   struct xa_window *w,
			   XA_WIND_ATTR tp,
			   bool r_is_wa,
			   RECT r, RECT *remember);

void wi_remove(struct win_base *b, struct xa_window *w);
void wi_put_first(struct win_base *b, struct xa_window *w);

struct xa_window *find_window(enum locks lock, short x, short y);
struct xa_window *get_wind_by_handle(enum locks lock, short h);
struct xa_window *pull_wind_to_top(enum locks lock, struct xa_window *w);
struct xa_window *top_w(enum locks lock);
struct xa_window *root_w(enum locks lock);

bool	close_window(enum locks lock, struct xa_window *wind);
int	open_window(enum locks lock, struct xa_window *w, RECT r);
void	send_wind_to_bottom(enum locks lock, struct xa_window *w);
void	move_window(enum locks lock, struct xa_window *wind, bool blit, short newstate, short x, short y, short w, short h);
void	delete_window(enum locks lock, struct xa_window *wind);
void	delayed_delete_window(enum locks lock, struct xa_window *wind);
void	do_delayed_delete_window(enum locks lock);
void	display_window(enum locks lock, int which, struct xa_window *w, RECT *clip);

DoWinMesag	do_rootwind_msg;

void	draw_window(enum locks lock, struct xa_window *wind, const RECT *clip);
void	update_all_windows(enum locks lock, struct xa_window *wl);
void	update_windows_below(enum locks lock, const RECT *old, RECT *new, struct xa_window *wl, struct xa_window *wend);
void	redraw_client_windows(enum locks lock, struct xa_client *client);

RECT	free_icon_pos(enum locks lock, struct xa_window *ignore);
RECT	calc_window(enum locks lock, struct xa_client *client, int request,
		    unsigned long tp, WINDOW_TYPE dial, int thinframe, bool thinwork,
		    RECT r);

void	top_window(enum locks lock, bool snd_untopped, bool snd_ontop, struct xa_window *w, struct xa_window *oldtop);
void	bottom_window(enum locks lock, bool snd_untopped, bool snd_ontop, struct xa_window *w);
void	after_top(enum locks lock, bool untop);
void	remove_windows(enum locks lock, struct xa_client *client);
void	remove_all_windows(enum locks lock, struct xa_client *client);
void	inside_root(RECT *r, struct options *o);

void	iconify_window(enum locks lock, struct xa_window *wind, RECT *r);
void	uniconify_window(enum locks lock, struct xa_window *wind, RECT *r);

void	hide_window(enum locks lock, struct xa_window *wind);
void	unhide_window(enum locks lock, struct xa_window *wind, bool check);
void	clear_wind_handles(void);
void	clear_wind_rectlist(struct xa_window *wind);

void	send_moved	(enum locks lock, struct xa_window *wind, short amq, RECT *r);
void	send_sized	(enum locks lock, struct xa_window *wind, short amq, RECT *r);
void	send_reposed	(enum locks lock, struct xa_window *wind, short amq, RECT *r);
void	send_vslid	(enum locks lock, struct xa_window *wind, short offs);
void	send_hslid	(enum locks lock, struct xa_window *wind, short offs);
void	send_redraw	(enum locks lock, struct xa_window *wind, RECT *r);
void	send_iredraw	(enum locks lock, struct xa_window *wind, short xaw, RECT *r);
void	generate_redraws(enum locks lock, struct xa_window *wind, RECT *r, short flags);

void	send_ontop(enum locks lock);
//void	send_untop(enum locks lock, struct xa_window *wind);
void	send_topped(enum locks lock, struct xa_window *wind);
void	send_bottomed(enum locks lock, struct xa_window *wind);
void	setwin_untopped(enum locks lock, struct xa_window *wind, bool snd_untopped);
void	setwin_ontop(enum locks lock, bool snd_ontop);

bool	is_topped(struct xa_window *wind);
bool	is_hidden(struct xa_window *wind);
bool	unhide(struct xa_window *w, short *x, short *y);

void	set_window_title(struct xa_window *wind, const char *title, bool redraw);
void	get_window_title(struct xa_window *wind, char *dst);
void	set_window_info(struct xa_window *wind, const char *info, bool redraw);
void	get_window_info(struct xa_window *wind, char *dst);

void	set_and_update_window(struct xa_window *wind, bool blit, bool only_wa, RECT *new);

XA_WIND_ATTR hide_move(struct options *o);

#endif /* _c_window_h */
