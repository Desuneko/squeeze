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

#ifndef __LSQ_COMMAND_OPTION_H__
#define __LSQ_COMMAND_OPTION_H__

#define LSQ_TYPE_COMMAND_OPTION           lsq_command_option_get_type(0)
#define LSQ_COMMAND_OPTION(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj),LSQ_TYPE_COMMAND_OPTION,LSQCommandOption))
#define LSQ_IS_COMMAND_OPTION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj),LSQ_TYPE_COMMAND_OPTION))

#define LSQ_TYPE_COMMAND_OPTION_BOOL      lsq_command_option_get_type(1)
#define LSQ_COMMAND_OPTION_BOOL(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),LSQ_TYPE_COMMAND_OPTION_BOOL,LSQCommandOptionBool))
#define LSQ_IS_COMMAND_OPTION_BOOL(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),LSQ_TYPE_COMMAND_OPTION_BOOL))

#define LSQ_TYPE_COMMAND_OPTION_STRING    lsq_command_option_get_type(2)
#define LSQ_COMMAND_OPTION_STRING(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj),LSQ_TYPE_COMMAND_OPTION_STRING,LSQCommandOptionString))
#define LSQ_IS_COMMAND_OPTION_STRING(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),LSQ_TYPE_COMMAND_OPTION_STRING))

#define LSQ_TYPE_COMMAND_OPTION_INT       lsq_command_option_get_type(3)
#define LSQ_COMMAND_OPTION_INT(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj),LSQ_TYPE_COMMAND_OPTION_INT,LSQCommandOptionInt))
#define LSQ_IS_COMMAND_OPTION_INT(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj),LSQ_TYPE_COMMAND_OPTION_INT))

#define LSQ_TYPE_COMMAND_OPTION_UINT      lsq_command_option_get_type(4)
#define LSQ_COMMAND_OPTION_UINT(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),LSQ_TYPE_COMMAND_OPTION_UINT,LSQCommandOptionUint))
#define LSQ_IS_COMMAND_OPTION_UINT(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),LSQ_TYPE_COMMAND_OPTION_UINT))

typedef struct _LSQCommandOptionPair    LSQCommandOptionPair;

typedef struct _LSQCommandOption        LSQCommandOption;
typedef struct _LSQCommandOptionString  LSQCommandOptionString;
typedef struct _LSQCommandOptionBool    LSQCommandOptionBool;
typedef struct _LSQCommandOptionInt     LSQCommandOptionInt;
typedef struct _LSQCommandOptionUint    LSQCommandOptionUint;

GType
lsq_command_option_get_type ( guint ) G_GNUC_CONST;

LSQCommandOptionPair **
lsq_command_option_create_pair ( LSQCommandOption **option_list ) G_GNUC_WARN_UNUSED_RESULT;

LSQCommandOption **
lsq_command_option_create_list (
        XfceRc *rc,
        gchar **option_names
    ) G_GNUC_WARN_UNUSED_RESULT;

gchar **
lsq_command_option_pair_get_args ( LSQCommandOptionPair **pair ) G_GNUC_WARN_UNUSED_RESULT;

void
lsq_command_option_get_default (
        const LSQCommandOption *option,
        GValue *value
    );

gint
lsq_command_option_get_args (
        const LSQCommandOption *option,
        GValue *value,
        gchar **argv
    );

struct _LSQCommandOptionPair
{
    GValue value;
    const LSQCommandOption *option;
};

struct _LSQCommandOption
{
    GTypeInstance parent;
    const gchar *name;
    const gchar *flag;
    const gchar *blurb;
    GType value_type;
};

struct _LSQCommandOptionString
{
    LSQCommandOption parent;

    const gchar *default_value;
    const gchar *filter;
};

struct _LSQCommandOptionBool
{
    LSQCommandOption parent;

    gboolean default_value;
};

struct _LSQCommandOptionInt
{
    LSQCommandOption parent;

    gint default_value;
    gint min_value;
    gint max_value;
};

struct _LSQCommandOptionUint
{
    LSQCommandOption parent;

    guint default_value;
    guint min_value;
    guint max_value;
};

#endif /* __LSQ_COMMAND_OPTION_H__ */
