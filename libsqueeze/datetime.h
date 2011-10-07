/*
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#ifndef __LIBSQUEEZE_DATETIME_H__
#define __LIBSQUEEZE_DATETIME_H__ 
G_BEGIN_DECLS


#define LSQ_TYPE_DATETIME lsq_datetime_get_type()
#define LSQ_DATETIME(p) ((LSQDateTime*)(p))

#define G_VALUE_HOLDS_DATETIME(value) (G_TYPE_CHECK_VALUE_TYPE((value), LSQ_TYPE_DATETIME))

typedef struct tm LSQDateTime;

GType
lsq_datetime_get_type ( void ) G_GNUC_CONST;

void
lsq_datetime_register_type ( void );

LSQDateTime *
lsq_datetime_new_from_string (
        const gchar *,
        const gchar *,
        gchar ** );

gchar *
lsq_datetime_from_string (
        LSQDateTime *,
        const gchar *,
        const gchar * );

gint
lsq_datetime_cmp (
        const LSQDateTime *,
        const LSQDateTime * );

const LSQDateTime *
g_value_get_datetime ( const GValue * );

LSQDateTime *
g_value_dup_datetime ( const GValue * );

void
g_value_set_datetime (
        GValue *,
        const LSQDateTime * );

void
g_value_set_static_datetime (
        GValue *,
        const gchar * );

void
g_value_take_datetime (
        GValue *,
        LSQDateTime * );


G_END_DECLS

#endif /* __LIBSQUEEZE_DATETIME_H__ */
