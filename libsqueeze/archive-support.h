/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
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
typedef struct _LSQArchiveSupport LSQArchiveSupport;

typedef gboolean (*LSQCustomActionPreFunc) (LSQCustomAction *);
typedef gboolean (*LSQCustomActionFunc) (LSQArchiveSupport *support, LSQArchive *, gpointer user_data);
typedef gboolean (*LSQCustomActionPostFunc) (LSQCustomAction *, gboolean);

struct _LSQCustomAction
{
	gchar *name;
	gchar *nick;
	gchar *blurb;
	gchar *icon;
	LSQCustomActionFunc func;
	LSQCustomActionPostFunc post_func;
	LSQArchiveSupport *support;
	gpointer user_data;
};

struct _LSQArchiveSupport
{
	GObject       parent;
	gchar        *id;
	GSList       *mime;
	GSList       *custom_action;
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

gboolean             lsq_register_support(LSQArchiveSupport *);
LSQArchiveSupport *  lsq_get_support_for_mime(const gchar *mime);
LSQArchiveSupport *  lsq_get_support_for_mime_from_slist(GSList *list, const gchar *mime);

gint                 lsq_archive_support_lookup_mime(gconstpointer support_mime, gconstpointer mime);
gint                 lsq_archive_support_lookup_support(gconstpointer support, gconstpointer mime);

gint                 lsq_archive_support_add(LSQArchiveSupport *, LSQArchive *, GSList *);
gint                 lsq_archive_support_extract(LSQArchiveSupport *, LSQArchive *, gchar *, GSList *);
gint                 lsq_archive_support_remove(LSQArchiveSupport *, LSQArchive *, GSList *);
gint                 lsq_archive_support_refresh(LSQArchiveSupport *, LSQArchive *);

GSList *             lsq_archive_support_list_properties(LSQArchiveSupport *, gchar *);

void                 lsq_archive_support_install_action(LSQArchiveSupport *, LSQCustomAction *);
LSQCustomAction*       lsq_archive_support_find_action(LSQArchiveSupport *, const gchar *name);
LSQCustomAction**      lsq_archive_support_list_actions(LSQArchiveSupport *, guint *n_actions);

LSQCustomAction*       lsq_custom_action_new(const gchar *name, 
                                             const gchar *nick, 
																						 const gchar *blurb, 
																						 const gchar *icon, 
																						 LSQCustomActionFunc func, 
																						 LSQArchiveSupport *support, 
																						 gpointer user_data);
const gchar*         lsq_custom_action_get_name(LSQCustomAction*);
const gchar*         lsq_custom_action_get_nick(LSQCustomAction*);
const gchar*         lsq_custom_action_get_blurb(LSQCustomAction*);
const gchar*         lsq_custom_action_get_icon_name(LSQCustomAction*);
void                 lsq_custom_action_execute(LSQCustomAction*, LSQArchive *, LSQCustomActionPreFunc, LSQCustomActionPostFunc);

G_END_DECLS

#endif /* __LIBSQUEEZE_ARCHIVE_SUPPORT_H__ */