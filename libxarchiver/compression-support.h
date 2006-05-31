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
#ifndef __LIBXARCHIVER_COMPRESSIONSUPPORT_H__
#define __LIBXARCHIVER_COMPRESSIONSUPPORT_H__

G_BEGIN_DECLS


#define LXA_TYPE_COMPRESSIONSUPPORT lxa_compressionsupport_get_type()

#define LXA_COMPRESSIONSUPPORT(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LXA_TYPE_COMPRESSIONSUPPORT,      \
			LXACompressionSupport))

#define LXA_IS_COMPRESSIONSUPPORT(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LXA_TYPE_COMPRESSIONSUPPORT))

#define LXA_COMPRESSIONSUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LXA_TYPE_COMPRESSIONSUPPORT,      \
			LXACompressionSupportClass))

#define LXA_IS_COMPRESSIONSUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LXA_TYPE_COMPRESSIONSUPPORT))

typedef struct _LXACompressionSupport LXACompressionSupport;

struct _LXACompressionSupport
{
	GObject parent;
	LXACompressionType type;
};

typedef struct _LXACompressionSupportClass LXACompressionSupportClass;

struct _LXACompressionSupportClass
{
	GObjectClass parent;
}; 

GType                    lxa_compressionsupport_get_type(void);
LXACompressionSupport *  lxa_compressionsupport_new();

G_END_DECLS

#endif /* __LIBXARCHIVER_ARCHIVESUPPORT_H__ */
