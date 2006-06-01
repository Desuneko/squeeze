/*
 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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

#include <glib.h>
#include <glib-object.h>
#include "libxarchiver.h"
#include "archive-support.h"
#include "archive-support-gnu-tar.h"

#include "compression-support.h"
#include "compression-support-gzip.h"
#include "compression-support-bzip2.h"

void
lxa_init()
{

}

int
lxa_destroy()
{

}

/*
 * XAArchive* lxa_new_archive(gchar *path, LXAArchiveType type)
 *
 */
LXAArchive *
lxa_new_archive(gchar *path, LXAArchiveType type, LXACompressionType compression)
{
	LXAArchive *archive = lxa_archive_new(path, type, compression);
	return archive;
}

LXAArchive *
lxa_open_archive(gchar *path)
{
	LXAArchive *archive = lxa_archive_new(path, LXA_ARCHIVETYPE_UNKNOWN, LXA_COMPRESSIONTYPE_NONE);
	return archive;
}

void
lxa_close_archive(LXAArchive *archive)
{

}
