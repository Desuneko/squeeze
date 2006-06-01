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

#define _(String) gettext(String)

void
lxa_compression_support_init(LXACompressionSupport *support);
void
lxa_compression_support_class_init(LXACompressionSupportClass *supportclass);

static guint lxa_compression_support_signals[2];

GType
lxa_compression_support_get_type ()
{
	static GType lxa_compression_support_type = 0;

 	if (!lxa_compression_support_type)
	{
 		static const GTypeInfo lxa_compression_support_info = 
		{
			sizeof (LXACompressionSupportClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_compression_support_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXACompressionSupport),
			0,
			(GInstanceInitFunc) lxa_compression_support_init,
		};

		lxa_compression_support_type = g_type_register_static (G_TYPE_OBJECT, "LXACompressionSupport", &lxa_compression_support_info, 0);
	}
	return lxa_compression_support_type;
}

int
lxa_compression_support_compress(LXAArchive *archive, gchar *filename)
{
	return -1;
}

int
lxa_compression_support_decompress(LXAArchive *archive, gchar *filename)
{
	return -1;
}

void
lxa_compression_support_init(LXACompressionSupport *support)
{
	support->compress = lxa_compression_support_compress;
	support->decompress = lxa_compression_support_decompress;
}

void
lxa_compression_support_class_init(LXACompressionSupportClass *supportclass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXACompressionSupportClass *klass = LXA_COMPRESSION_SUPPORT_CLASS (supportclass);

	lxa_compression_support_signals[0] = g_signal_new("lxa_compress_complete",
			G_TYPE_FROM_CLASS(supportclass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);

	lxa_compression_support_signals[1] = g_signal_new("lxa_decompress_complete",
			G_TYPE_FROM_CLASS(supportclass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);
}

LXACompressionSupport*
lxa_compression_support_new()
{
	LXACompressionSupport*support;

	support = g_object_new(LXA_TYPE_COMPRESSION_SUPPORT, NULL);
	
	return support;
}

void
lxa_compression_support_emit_signal(LXACompressionSupport *support, guint signal_id, LXAArchive *archive)
{
	g_signal_emit(G_OBJECT(support), lxa_compression_support_signals[signal_id], 0, archive);
}
