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
#ifndef __LIBXARCHIVER_COMPRESSION_SUPPORT_BZIP2_H__
#define __LIBXARCHIVER_COMPRESSION_SUPPORT_BZIP2_H__

G_BEGIN_DECLS


#define LXA_TYPE_COMPRESSION_SUPPORT_BZIP2 lxa_compression_support_bzip2_get_type()

#define LXA_COMPRESSION_SUPPORT_BZIP2(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LXA_TYPE_COMPRESSION_SUPPORT_BZIP2,      \
			LXACompressionSupportBzip2))

#define LXA_IS_COMPRESSION_SUPPORT_BZIP2(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LXA_TYPE_COMPRESSION_SUPPORT_BZIP2))

#define LXA_COMPRESSION_SUPPORT_BZIP2_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LXA_TYPE_COMPRESSION_SUPPORT_BZIP2,      \
			LXACompressionSupportBzip2Class))

#define LXA_IS_COMPRESSION_SUPPORT_BZIP2_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LXA_TYPE_COMPRESSION_SUPPORT_BZIP2))

typedef struct _LXACompressionSupportBzip2 LXACompressionSupportBzip2;

struct _LXACompressionSupportBzip2
{
	LXACompressionSupport parent;
};

typedef struct _LXACompressionSupportBzip2Class LXACompressionSupportBzip2Class;

struct _LXACompressionSupportBzip2Class
{
	LXACompressionSupportClass parent;
}; 

GType                    lxa_compression_support_bzip2_get_type(void);
LXACompressionSupport *  lxa_compression_support_bzip2_new();

G_END_DECLS

#endif /* __LIBXARCHIVER_COMPRESSION_SUPPORT_BZIP2_H__*/
