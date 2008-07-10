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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__ 
G_BEGIN_DECLS

#define LSQ_TYPE_ARCHIVE lsq_archive_get_type()

#define LSQ_ARCHIVE(obj)		 ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),	\
			LSQ_TYPE_ARCHIVE,	  \
			LSQArchive))

#define LSQ_IS_ARCHIVE(obj)	  ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),	\
			LSQ_TYPE_ARCHIVE))

#define LSQ_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),	 \
			LSQ_TYPE_ARCHIVE,	  \
			LSQArchiveClass))

#define LSQ_IS_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),		\
			LSQ_TYPE_ARCHIVE))

enum
{
	LSQ_ARCHIVE_PROP_FILENAME = 0,
	LSQ_ARCHIVE_PROP_MIME_TYPE,
	LSQ_ARCHIVE_PROP_USER
};

typedef enum
{
    LSQ_ARCHIVE_STATE_IDLE,
    LSQ_ARCHIVE_STATE_BUSY
} LSQArchiveState;

typedef struct _LSQArchivePrivate LSQArchivePrivate;

struct _LSQArchivePrivate
{
	ThunarVfsPath	  *path_info;
	ThunarVfsInfo	  *file_info;
	ThunarVfsMimeInfo  *mime_info;

	LSQSupportTemplate *s_template;

    LSQArchiveState state;
    const gchar *state_msg;
};


typedef struct _LSQArchive LSQArchive;

struct _LSQArchive
{
	GObject parent;
	LSQArchivePrivate  *priv;
	LSQArchiveEntry	*root_entry;
	LSQArchiveIterPool *pool;
	gchar *temp_dir;
	GSList *monitor_list;
	struct {
		guint64 archive_size;
		guint64 content_size;
		guint64 n_files;
		guint64 n_directories;
	} props;
};


typedef struct _LSQArchiveClass LSQArchiveClass;

struct _LSQArchiveClass
{
	GObjectClass parent;
};


GType		   lsq_archive_get_type(void);

gchar		  *lsq_archive_get_path(const LSQArchive *archive);
const gchar	*lsq_archive_get_filename(const LSQArchive *archive);
const gchar	*lsq_archive_get_mimetype(const LSQArchive *archive);
gboolean		lsq_archive_exists(const LSQArchive *archive);
LSQSupportType  lsq_archive_get_support_mask(const LSQArchive *archive);


LSQArchive		 *lsq_archive_new(gchar *, const gchar *) G_GNUC_INTERNAL;
void				lsq_archive_state_changed(const LSQArchive *archive) G_GNUC_INTERNAL;
void				lsq_archive_add_children(GSList *files);
gboolean			lsq_archive_remove_file(LSQArchive *, const gchar *);

ThunarVfsPath	  *lsq_archive_get_path_info(LSQArchive *);

gboolean		lsq_archive_operate(LSQArchive *archive, LSQCommandType type, const gchar **, const gchar *);

const gchar	*
lsq_archive_get_state_msg(const LSQArchive *archive);

LSQArchiveState
lsq_archive_get_state(const LSQArchive *archive);

void
lsq_archive_refreshed(const LSQArchive *archive);


G_END_DECLS

#endif /* __ARCHIVE_H__ */
