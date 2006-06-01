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
#ifndef __LIBXARCHIVER_COMPRESSION_SUPPORT_H__
#define __LIBXARCHIVER_COMPRESSION_SUPPORT_H__

G_BEGIN_DECLS


#define LXA_TYPE_COMPRESSION_SUPPORT lxa_compression_support_get_type()

#define LXA_COMPRESSION_SUPPORT(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LXA_TYPE_COMPRESSION_SUPPORT,      \
			LXACompressionSupport))

#define LXA_IS_COMPRESSION_SUPPORT(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LXA_TYPE_COMPRESSION_SUPPORT))

#define LXA_COMPRESSION_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LXA_TYPE_COMPRESSION_SUPPORT,      \
			LXACompressionSupportClass))

#define LXA_IS_COMPRESSION_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LXA_TYPE_COMPRESSION_SUPPORT))

typedef struct _LXACompressionSupport LXACompressionSupport;

struct _LXACompressionSupport
{
	GObject parent;
	LXACompressionType type;
	int (*compress) (LXAArchive *, gchar *filename);
	int (*decompress) (LXAArchive *, gchar *filename);
};

typedef struct _LXACompressionSupportClass LXACompressionSupportClass;

struct _LXACompressionSupportClass
{
	GObjectClass parent;
}; 

#define              LXA_C_S_COMPRESS_COMPLETE     0
#define              LXA_C_S_DECOMPRESS_COMPLETE   1

GType                    lxa_compression_support_get_type(void);
LXACompressionSupport *  lxa_compression_support_new();
void                     lxa_compression_support_emit_signal(LXACompressionSupport *support, guint signal_id, LXAArchive *archive);

G_END_DECLS

#endif /* __LIBXARCHIVER_COMPRESSION_SUPPORT_H__ */
