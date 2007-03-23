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

#ifndef __LIBSQUEEZE_ARCHIVE_MIME_INFO_H__
#define __LIBSQUEEZE_ARCHIVE_MIME_INFO_H__

struct _LSQArchiveMimeInfo
{
	ThunarVfsMimeInfo *mime_info;
	GSList            *command_builders;
};

typedef struct _LSQArchiveMimeInfo LSQArchiveMimeInfo;

LSQArchiveMimeInfo *
lsq_archive_mime_info_new(const gchar *mime);


void
lsq_command_builder_register(const LSQCommandBuilder *builder);

LSQCommandBuilder *
lsq_archive_mime_get_default_builder(const gchar *mime);

#endif /* __LIBSQUEEZE_ARCHIVE_MIME_INFO_H__ */

