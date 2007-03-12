/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __LIBSQUEEZE_ARCHIVE_SUPPORT_H__
#define __LIBSQUEEZE_ARCHIVE_SUPPORT_H__

G_BEGIN_DECLS

typedef struct _LSQCustomAction LSQCustomAction;
typedef struct _LSQCustomActionCallback LSQCustomActionCallback;


typedef gboolean (*LSQCustomActionPreFunc) (LSQCustomAction *);
typedef gboolean (*LSQCustomActionFunc) (LSQArchiveSupport *support, LSQArchive *, LSQCustomAction*, gpointer user_data);
typedef gboolean (*LSQCustomActionPostFunc) (LSQCustomAction *, gboolean);
typedef gboolean (*LSQCustomActionNotifyFunc) (LSQCustomAction *, const gchar*);


struct _LSQCustomActionCallback
{
	LSQCustomActionNotifyFunc notify_func;
	LSQCustomActionPostFunc post_func;
	LSQCustomActionPreFunc pre_func;
};

struct _LSQCustomAction
{
	gchar *name;
	gchar *nick;
	gchar *blurb;
	gchar *icon;
	LSQCustomActionFunc func;
	LSQCustomActionCallback *callback;
	LSQArchiveSupport *support;
	gpointer user_data;
};

GType                lsq_archive_support_get_type(void);

gint                 lsq_archive_support_lookup_mime(gconstpointer support_mime, gconstpointer mime);
gint                 lsq_archive_support_lookup_support(gconstpointer support, gconstpointer mime);


void                 lsq_archive_support_install_action(LSQArchiveSupport *support, LSQCustomAction *action);
LSQCustomAction*       lsq_archive_support_find_action(LSQArchiveSupport *support, const gchar *name);
LSQCustomAction**      lsq_archive_support_list_actions(LSQArchiveSupport *support, guint *n_actions);

LSQCustomAction*       lsq_custom_action_new(const gchar *name, 
                                             const gchar *nick, 
																						 const gchar *blurb, 
																						 const gchar *icon, 
																						 LSQCustomActionFunc func, 
																						 LSQArchiveSupport *support, 
																						 gpointer user_data);
const gchar*         lsq_custom_action_get_name(LSQCustomAction *action);
const gchar*         lsq_custom_action_get_nick(LSQCustomAction *action);
const gchar*         lsq_custom_action_get_blurb(LSQCustomAction *action);
const gchar*         lsq_custom_action_get_icon_name(LSQCustomAction *action);
void                 lsq_custom_action_execute(LSQCustomAction *action, LSQArchive *archive, LSQCustomActionCallback *callback);
void                 lsq_custom_action_notify(LSQCustomAction *action, const gchar *);

G_END_DECLS

#endif /* __LIBSQUEEZE_ARCHIVE_SUPPORT_H__ */
