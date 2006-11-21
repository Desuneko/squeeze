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
#ifndef __LIBXARCHIVER_ARCHIVE_SUPPORT_H__
#define __LIBXARCHIVER_ARCHIVE_SUPPORT_H__

G_BEGIN_DECLS


#define LXA_TYPE_ARCHIVE_SUPPORT lxa_archive_support_get_type()

#define LXA_ARCHIVE_SUPPORT(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LXA_TYPE_ARCHIVE_SUPPORT,      \
			LXAArchiveSupport))

#define LXA_IS_ARCHIVE_SUPPORT(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LXA_TYPE_ARCHIVE_SUPPORT))

#define LXA_ARCHIVE_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LXA_TYPE_ARCHIVE_SUPPORT,      \
			LXAArchiveSupportClass))

#define LXA_IS_ARCHIVE_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LXA_TYPE_ARCHIVE_SUPPORT))

typedef struct _LXACustomAction LXACustomAction;
typedef struct _LXAArchiveSupport LXAArchiveSupport;

typedef void (*LXACustomActionFunc) (LXAArchiveSupport *support, LXAArchive *, gpointer user_data);

struct _LXACustomAction
{
	gchar *name;
	gchar *nick;
	gchar *blurb;
	gchar *icon;
	LXACustomActionFunc func;
	LXAArchiveSupport *support;
	gpointer user_data;
};

struct _LXAArchiveSupport
{
	GObject       parent;
	gchar        *id;
	GSList       *mime;
	GSList       *custom_action;
/*
 * The following functions should _NOT_ be called directly.
 *
 * lxa_archive_support_add()
 * lxa_archive_support_extract()
 * lxa_archive_support_remove()
 * lxa_archive_support_refresh()
 * 
 * should be called instead.
 */
	gint        (*add)(LXAArchive *archive, GSList *files);
	gint        (*extract)(LXAArchive *archive, gchar *dest_path, GSList *files);
	gint        (*remove)(LXAArchive *archive, GSList *files);
	gint        (*refresh)(LXAArchive *archive);
};

typedef struct _LXAArchiveSupportClass LXAArchiveSupportClass;

struct _LXAArchiveSupportClass
{
	GObjectClass  parent;
}; 

GType                lxa_archive_support_get_type(void);
LXAArchiveSupport *  lxa_archive_support_new();
void                 lxa_archive_support_add_mime(LXAArchiveSupport *support, gchar *mime);
gboolean             lxa_archive_support_mime_supported(LXAArchiveSupport *,const gchar *mime);

gboolean             lxa_register_support(LXAArchiveSupport *);
LXAArchiveSupport *  lxa_get_support_for_mime(const gchar *mime);
LXAArchiveSupport *  lxa_get_support_for_mime_from_slist(GSList *list, const gchar *mime);

gint                 lxa_archive_support_lookup_mime(gconstpointer support_mime, gconstpointer mime);
gint                 lxa_archive_support_lookup_support(gconstpointer support, gconstpointer mime);

gint                 lxa_archive_support_add(LXAArchiveSupport *, LXAArchive *, GSList *);
gint                 lxa_archive_support_extract(LXAArchiveSupport *, LXAArchive *, gchar *, GSList *);
gint                 lxa_archive_support_remove(LXAArchiveSupport *, LXAArchive *, GSList *);
gint                 lxa_archive_support_refresh(LXAArchiveSupport *, LXAArchive *);

GSList *             lxa_archive_support_list_properties(LXAArchiveSupport *, gchar *);

void                 lxa_archive_support_install_action(LXAArchiveSupport *, LXACustomAction *);
LXACustomAction*       lxa_archive_support_find_action(LXAArchiveSupport *, const gchar *name);
LXACustomAction**      lxa_archive_support_list_actions(LXAArchiveSupport *, guint *n_actions);

LXACustomAction*       lxa_custom_action_new(const gchar *name, 
                                             const gchar *nick, 
																						 const gchar *blurb, 
																						 const gchar *icon, 
																						 LXACustomActionFunc func, 
																						 LXAArchiveSupport *support, 
																						 gpointer user_data);
const gchar*         lxa_custom_action_get_name(LXACustomAction*);
const gchar*         lxa_custom_action_get_nick(LXACustomAction*);
const gchar*         lxa_custom_action_get_blurb(LXACustomAction*);
const gchar*         lxa_custom_action_get_icon_name(LXACustomAction*);
void                 lxa_custom_action_execute(LXACustomAction*, LXAArchive *);

G_END_DECLS

#endif /* __LIBXARCHIVER_ARCHIVE_SUPPORT_H__ */
