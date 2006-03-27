/*
 *  Copyright (c) 2006 Stephan Arts <stephan.arts@hva.nl>
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
#ifndef __LIBXARCHIVER_H__
#define __LIBXARCHIVER_H__

G_BEGIN_DECLS

typedef enum
{
	XARCHIVETYPE_UNKNOWN,
	XARCHIVETYPE_BZIP2,
	XARCHIVETYPE_GZIP,
	XARCHIVETYPE_RAR,
	XARCHIVETYPE_ZIP,
	XARCHIVETYPE_ARJ,
	XARCHIVETYPE_TAR,
	XARCHIVETYPE_RPM,
	XARCHIVETYPE_7ZIP,
	XARCHIVETYPE_ISO
} XArchiveType;

typedef struct _XArchive XArchive;

struct _XArchive
{
	XArchiveType type;
	gchar *path;
	int passwd;
};

typedef struct _XArchiveSupport XArchiveSupport;

struct _XArchiveSupport
{
	XArchiveType type;
	gboolean (*verify)  (XArchive *);
	gboolean (*add)     (XArchive *, GSList *);
	gboolean (*extract) (XArchive *, gchar *, GSList *);
};

void xarchiver_init();

int xarchiver_destroy();

XArchive *xarchiver_archive_new(gchar *path, XArchiveType type);
XArchiveSupport *xarchiver_find_archive_support(XArchive *archive);

G_END_DECLS

#endif /* __LIBXARCHIVER_H__ */