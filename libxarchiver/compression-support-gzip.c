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

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <glib.h>
#include <glib-object.h>
#include <libintl.h>
#include "archive.h"
#include "compression-support.h"
#include "compression-support-gzip.h"

#define _(String) gettext(String)

void
lxa_compression_support_gzip_init(LXACompressionSupportGzip *support);
void
lxa_compression_support_gzip_class_init(LXACompressionSupportGzipClass *supportclass);

GType
lxa_compression_support_gzip_get_type ()
{
	static GType lxa_compression_support_gzip_type = 0;

 	if (!lxa_compression_support_gzip_type)
	{
 		static const GTypeInfo lxa_compression_support_gzip_info = 
		{
			sizeof (LXACompressionSupportGzipClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_compression_support_gzip_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXACompressionSupportGzip),
			0,
			(GInstanceInitFunc) lxa_compression_support_gzip_init,
		};

		lxa_compression_support_gzip_type = g_type_register_static (LXA_TYPE_COMPRESSION_SUPPORT, "LXACompressionSupportGzip", &lxa_compression_support_gzip_info, 0);
	}
	return lxa_compression_support_gzip_type;
}

gint
lxa_compression_support_gzip_compress(LXAArchive *archive)
{
	return 0;
}

gint
lxa_compression_support_gzip_decompress(LXAArchive *archive)
{
	return 0;
}

void
lxa_compression_support_gzip_init(LXACompressionSupportGzip *support)
{
	LXA_COMPRESSION_SUPPORT(support)->id = "gzip";
	LXA_COMPRESSION_SUPPORT(support)->type = LXA_COMPRESSIONTYPE_GZIP;
	LXA_COMPRESSION_SUPPORT(support)->compress = lxa_compression_support_gzip_compress;
	LXA_COMPRESSION_SUPPORT(support)->decompress = lxa_compression_support_gzip_decompress;
}

void
lxa_compression_support_gzip_class_init(LXACompressionSupportGzipClass *supportclass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXACompressionSupportGzipClass *klass = LXA_COMPRESSION_SUPPORT_GZIP_CLASS (supportclass);
}

LXACompressionSupport*
lxa_compression_support_gzip_new()
{
	LXACompressionSupportGzip* support;

	support = g_object_new(LXA_TYPE_COMPRESSION_SUPPORT_GZIP, NULL);
	
	return LXA_COMPRESSION_SUPPORT(support);
}
