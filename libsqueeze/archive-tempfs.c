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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h> 
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <thunar-vfs/thunar-vfs.h>

#include "libsqueeze.h"
#include "libsqueeze-archive.h"
#include "archive-iter.h"
#include "support-factory.h"
#include "archive.h"
#include "archive-tempfs.h"

#include "internals.h"

static guint suffix = 0;
/*
typedef struct
{
	gchar *filename;
	struct timespec mod_time;
} LSQTempFileMonitor;
*/

static void lsq_tempfs_clean_dir(const gchar *path)
{
	const gchar *file;

	if(!path)
		return;

	GDir *dir = g_dir_open(path, 0, NULL);

	if(dir)
	{
		file = g_dir_read_name(dir);

		while(file)
		{
			file = g_strconcat(path, "/", file, NULL);
			lsq_tempfs_clean_dir(file);
			g_free((gchar*)file);
			file = g_dir_read_name(dir);
		}

		g_dir_close(dir);
	}

	g_remove(path);
}

void lsq_tempfs_clean_root_dir(LSQArchive *archive)
{
	if(!archive->temp_dir)
		return;

	lsq_tempfs_clean_dir(archive->temp_dir);

#ifdef DEBUG
	g_debug("clean %s", archive->temp_dir);
#endif

	GSList *iter = archive->monitor_list;
	while(iter)
	{
	/*	g_free(((LSQTempFileMonitor*)iter->data)->filename); */
		g_free(iter->data);
		iter = g_slist_next(iter);
	}
	g_slist_free(archive->monitor_list);
	archive->monitor_list = NULL;

	g_free(archive->temp_dir);
	archive->temp_dir = NULL;
}

const gchar* lsq_tempfs_get_root_dir(LSQArchive *archive)
{
	if(!archive->temp_dir)
		if(!lsq_tempfs_make_root_dir(archive))
			return NULL;

	return archive->temp_dir;
}

gboolean lsq_tempfs_make_root_dir(LSQArchive *archive)
{
	if(archive->temp_dir)
		return TRUE;

	gboolean error = FALSE;
	gchar dirname[256];

	g_snprintf(dirname, 256, "%s/" PACKAGE "-%s/", g_get_tmp_dir(), g_get_user_name());
	if(g_mkdir_with_parents(dirname, 0700))
		return FALSE;

	do
	{
		g_snprintf(dirname, 256, "%s/" PACKAGE "-%s/cache-%d/", g_get_tmp_dir(), g_get_user_name(), suffix++);
		error = g_mkdir(dirname, 0700);
	}
	while(error && errno == EEXIST);

	if(!error)
	{
		archive->temp_dir = g_strdup(dirname);
		return TRUE;
	}

	return FALSE;
}

gchar *lsq_archive_request_temp_file(LSQArchive *archive, const gchar *suffix)
{
	gchar dirname[256];
	gint handle;

	g_snprintf(dirname, 256, "%s/" PACKAGE "-%s/", g_get_tmp_dir(), g_get_user_name());
	if(g_mkdir_with_parents(dirname, 0700))
		return FALSE;

	g_snprintf(dirname, 256, "%s/" PACKAGE "-%s/file-XXXXXX%s", g_get_tmp_dir(), g_get_user_name(), suffix?suffix:"");

	handle = g_mkstemp(dirname);
	if(handle == -1)
		return NULL;
	close(handle);

	return strdup(dirname);
}

gboolean lsq_tempfs_make_dir(LSQArchive *archive, const gchar *path, gint mode)
{
	if(!archive->temp_dir)
		if(!lsq_tempfs_make_root_dir(archive))
			return FALSE;

	gchar *full_path = g_strconcat(archive->temp_dir, "/", path, NULL);

	gboolean error = g_mkdir_with_parents(full_path, mode);

	g_free(full_path);

	return !error;
}

gboolean lsq_tempfs_chmod(LSQArchive *archive, const gchar *path, gint mode)
{
	if(!archive->temp_dir)
		if(!lsq_tempfs_make_root_dir(archive))
			return FALSE;

	gchar *full_path = g_strconcat(archive->temp_dir, "/", path, NULL);

	gboolean error = g_chmod(full_path, mode);

	g_free(full_path);

	return !error;
}

/*
gboolean
lsq_tempfs_monitor_file(LSQArchive *archive, const gchar *path)
{
	if(!archive->temp_dir)
		if(!lsq_tempfs_make_root_dir(archive))
			return FALSE;
	
	struct stat status;

	if(g_stat(path, &status))
		return FALSE;
	
	LSQTempFileMonitor *monitor = g_new(LSQTempFileMonitor, 1);
	monitor->filename = g_strdup(path);
	monitor->mod_time = status.st_mtim;

	archive->monitor_list = g_slist_prepend(archive->monitor_list, monitor);

	return TRUE;
}

gboolean
lsq_tempfs_changed_file(LSQArchive *archive, const gchar *path)
{
	if(!archive->temp_dir)
		if(!lsq_tempfs_make_root_dir(archive))
			return FALSE;
	
	struct stat status;

	if(g_stat(path, &status))
		return FALSE;

	GSList *iter = archive->monitor_list;
	gboolean changed = FALSE;

	while(iter)
	{
		if(strcmp(path, ((LSQTempFileMonitor*)iter->data)->filename) == 0)
		{
			if((((LSQTempFileMonitor*)iter->data)->mod_time.tv_sec > status.st_mtim.tv_sec) ||
				(((LSQTempFileMonitor*)iter->data)->mod_time.tv_nsec > status.st_mtim.tv_nsec &&
				((LSQTempFileMonitor*)iter->data)->mod_time.tv_sec >= status.st_mtim.tv_sec))
			{
				changed = TRUE;
			}

			archive->monitor_list = g_slist_remove(archive->monitor_list, iter);
			break;
		}
		
		iter = g_slist_next(iter);
	}
	return changed;
}
*/

