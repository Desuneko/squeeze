/*
 *  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
 *
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


#define LSQ_TYPE_ARCHIVE_SUPPORT lsq_archive_support_get_type()

#define LSQ_ARCHIVE_SUPPORT(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT,      \
			LSQArchiveSupport))

#define LSQ_IS_ARCHIVE_SUPPORT(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT))

#define LSQ_ARCHIVE_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_ARCHIVE_SUPPORT,      \
			LSQArchiveSupportClass))

#define LSQ_IS_ARCHIVE_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_ARCHIVE_SUPPORT))

typedef struct _LSQCustomAction LSQCustomAction;
typedef struct _LSQCustomActionCallback LSQCustomActionCallback;
typedef struct _LSQArchiveSupport LSQArchiveSupport;

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

struct _LSQArchiveSupport
{
	GObject       parent;
	gchar        *id;
	GSList       *mime;
	GSList       *custom_action;
	guint64       max_n_files;
/*
 * The following functions should _NOT_ be called directly.
 *
 * lsq_archive_support_add()
 * lsq_archive_support_extract()
 * lsq_archive_support_remove()
 * lsq_archive_support_refresh()
 * 
 * should be called instead.
 */
	gint        (*add)(LSQArchive *archive, GSList *files);
	gint        (*extract)(LSQArchive *archive, gchar *dest_path, GSList *files);
	gint        (*remove)(LSQArchive *archive, GSList *files);
	gint        (*refresh)(LSQArchive *archive);
};

typedef struct _LSQArchiveSupportClass LSQArchiveSupportClass;

struct _LSQArchiveSupportClass
{
	GObjectClass  parent;
}; 

GType                lsq_archive_support_get_type(void);
LSQArchiveSupport *  lsq_archive_support_new();
void                 lsq_archive_support_add_mime(LSQArchiveSupport *support, gchar *mime);
gboolean             lsq_archive_support_mime_supported(LSQArchiveSupport *,const gchar *mime);

GSList *             lsq_get_registered_support_list();
gboolean             lsq_register_support(LSQArchiveSupport *support);
LSQArchiveSupport *  lsq_get_support_for_mime(ThunarVfsMimeInfo *mime_info);
LSQArchiveSupport *  lsq_get_support_for_mime_from_slist(GSList *list, const gchar *mime);

const gchar *        lsq_archive_support_get_id(LSQArchiveSupport *support);
gint                 lsq_archive_support_lookup_mime(gconstpointer support_mime, gconstpointer mime);
gint                 lsq_archive_support_lookup_support(gconstpointer support, gconstpointer mime);

gint                 lsq_archive_support_add(LSQArchiveSupport *, LSQArchive *, GSList *);
gint                 lsq_archive_support_extract(LSQArchiveSupport *, LSQArchive *, gchar *, GSList *);
gint                 lsq_archive_support_remove(LSQArchiveSupport *, LSQArchive *, GSList *);
gint                 lsq_archive_support_refresh(LSQArchiveSupport *, LSQArchive *);
guint64              lsq_archive_support_get_max_n_files(LSQArchiveSupport *);

gboolean             lsq_archive_support_can_stop(LSQArchiveSupport *support, LSQArchive *archive);

GSList *             lsq_archive_support_list_properties(LSQArchiveSupport *support, gchar *);

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
