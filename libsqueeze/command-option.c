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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h> 
#include <libxfce4util/libxfce4util.h>

#include "command-option.h"

#define LSQ_COMMAND_OPTION_GET_CLASS(obj) (	\
		G_TYPE_INSTANCE_GET_CLASS ((obj),  \
			LSQ_TYPE_COMMAND_OPTION,				 \
	  LSQCommandOptionClass))

typedef void (*LSQCommandOptionGetDefaultFunc)(LSQCommandOption*, GValue*);

typedef struct _LSQCommandOptionClass LSQCommandOptionClass;

struct _LSQCommandOptionClass
{
  GTypeClass parent;

  LSQCommandOptionGetDefaultFunc get_default;
};

static void lsq_command_option_class_init(LSQCommandOptionClass *);
static void lsq_command_option_bool_class_init(LSQCommandOptionClass *);
static void lsq_command_option_string_class_init(LSQCommandOptionClass *);
static void lsq_command_option_int_class_init(LSQCommandOptionClass *);
static void lsq_command_option_uint_class_init(LSQCommandOptionClass *);

GType
lsq_command_option_get_type(gint type_idx)
{
	static GType lsq_command_option_type[5] = {0};

 	if (!lsq_command_option_type[0])
	{
 		static const GTypeFundamentalInfo lsq_command_option_fund = 
    {
      G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE
    };
 		static const GTypeInfo lsq_command_option[5] = 
    {
      {
        sizeof (LSQCommandOptionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) lsq_command_option_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (LSQCommandOption),
        0,
        (GInstanceInitFunc) NULL,
        NULL
      },
      {
        sizeof (LSQCommandOptionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) lsq_command_option_bool_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (LSQCommandOptionBool),
        0,
        (GInstanceInitFunc) NULL,
        NULL
      },
      {
        sizeof (LSQCommandOptionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) lsq_command_option_string_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (LSQCommandOptionString),
        0,
        (GInstanceInitFunc) NULL,
        NULL
      },
      {
        sizeof (LSQCommandOptionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) lsq_command_option_int_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (LSQCommandOptionInt),
        0,
        (GInstanceInitFunc) NULL,
        NULL
      },
      {
        sizeof (LSQCommandOptionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) lsq_command_option_uint_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (LSQCommandOptionUint),
        0,
        (GInstanceInitFunc) NULL,
        NULL
      }
    };

		lsq_command_option_type[0] = g_type_register_fundamental (G_TYPE_GTYPE, "LSQCommandOption", &lsq_command_option[0], &lsq_command_option_fund, 0);
		lsq_command_option_type[1] = g_type_register_static (lsq_command_option_type[0], "LSQCommandOptionBool", &lsq_command_option[1], 0);
		lsq_command_option_type[2] = g_type_register_static (lsq_command_option_type[0], "LSQCommandOptionString", &lsq_command_option[2], 0);
		lsq_command_option_type[3] = g_type_register_static (lsq_command_option_type[0], "LSQCommandOptionInt", &lsq_command_option[3], 0);
		lsq_command_option_type[4] = g_type_register_static (lsq_command_option_type[0], "LSQCommandOptionUint", &lsq_command_option[4], 0);
	}
	return lsq_command_option_type[type_idx];
}

LSQCommandOption **
lsq_command_option_create_list(XfceRc *rc, gchar **option_names)
{
  LSQCommandOption **command_options = g_new0(LSQCommandOption*, g_strv_length(option_names));
  LSQCommandOption **cmd_opt_iter = command_options;
  gchar **option_iter;
  for(option_iter = option_names; *option_iter; option_iter++)
  {
    gchar *option_group = g_strconcat("Squeeze-Option-", *option_iter, NULL);
    xfce_rc_set_group(rc, option_group);

    const gchar *type = xfce_rc_read_entry(rc, "X-Squeeze-Type", "");
    if(0==g_ascii_strcasecmp("Bool", type))
    {
      *cmd_opt_iter = LSQ_COMMAND_OPTION(g_type_create_instance(LSQ_TYPE_COMMAND_OPTION_BOOL));
      (*cmd_opt_iter)->value_type = G_TYPE_BOOLEAN;
      LSQ_COMMAND_OPTION_BOOL(*cmd_opt_iter)->default_value = xfce_rc_read_bool_entry(rc, "X-Squeeze-Value", FALSE);
    }
    else if(0==g_ascii_strcasecmp("String", type))
    {
      *cmd_opt_iter = LSQ_COMMAND_OPTION(g_type_create_instance(LSQ_TYPE_COMMAND_OPTION_STRING));
      (*cmd_opt_iter)->value_type = G_TYPE_STRING;
      LSQ_COMMAND_OPTION_STRING(*cmd_opt_iter)->default_value = xfce_rc_read_entry(rc, "X-Squeeze-Value", NULL);
      LSQ_COMMAND_OPTION_STRING(*cmd_opt_iter)->filter = xfce_rc_read_entry(rc, "X-Squeeze-Filter", NULL);
    }
    else if(0==g_ascii_strcasecmp("Int", type))
    {
      *cmd_opt_iter = LSQ_COMMAND_OPTION(g_type_create_instance(LSQ_TYPE_COMMAND_OPTION_INT));
      (*cmd_opt_iter)->value_type = G_TYPE_INT;
      LSQ_COMMAND_OPTION_INT(*cmd_opt_iter)->default_value = xfce_rc_read_int_entry(rc, "X-Squeeze-Value", 0);
      LSQ_COMMAND_OPTION_INT(*cmd_opt_iter)->min_value = xfce_rc_read_int_entry(rc, "X-Squeeze-Min", 0);
      LSQ_COMMAND_OPTION_INT(*cmd_opt_iter)->max_value = xfce_rc_read_int_entry(rc, "X-Squeeze-Max", 0);
    }
    else if(0==g_ascii_strcasecmp("Uint", type))
    {
      *cmd_opt_iter = LSQ_COMMAND_OPTION(g_type_create_instance(LSQ_TYPE_COMMAND_OPTION_UINT));
      (*cmd_opt_iter)->value_type = G_TYPE_UINT;
      LSQ_COMMAND_OPTION_UINT(*cmd_opt_iter)->default_value = xfce_rc_read_int_entry(rc, "X-Squeeze-Value", 0);
      LSQ_COMMAND_OPTION_UINT(*cmd_opt_iter)->min_value = xfce_rc_read_int_entry(rc, "X-Squeeze-Min", 0);
      LSQ_COMMAND_OPTION_UINT(*cmd_opt_iter)->max_value = xfce_rc_read_int_entry(rc, "X-Squeeze-Max", 0);
    }
    else
    {
      continue;
    }

    (*cmd_opt_iter)->name = g_strdup(*option_iter);
    (*cmd_opt_iter)->flag = xfce_rc_read_entry(rc, "X-Squeeze-Flag", NULL);
    (*cmd_opt_iter)->blurb = xfce_rc_read_entry(rc, "X-Squeeze-Description", NULL);

    cmd_opt_iter++;
  }

  return command_options;
}

LSQCommandOptionPair **
lsq_command_option_create_pair(LSQCommandOption **option_list)
{
  int length = 0;
  LSQCommandOption **option_iter;

  for(option_iter = option_list; *option_iter; option_iter++)
  {
    length++;
  }

  LSQCommandOptionPair **option_pair = g_new0(LSQCommandOptionPair*, length);
  LSQCommandOptionPair **pair_iter = option_pair;

  for(option_iter = option_list; *option_iter; option_iter++)
  {
    LSQCommandOption *option = *option_iter;

    *pair_iter = g_new0(LSQCommandOptionPair, 1);

    (*pair_iter)->option = option;
    g_value_init(&(*pair_iter)->value, option->value_type);
    lsq_command_option_get_default(option, &(*pair_iter)->value);

    pair_iter++;
  }

  return option_pair;
}

void
lsq_command_option_get_default(LSQCommandOption *option, GValue *value)
{
  g_return_if_fail(LSQ_COMMAND_OPTION_GET_CLASS(option)->get_default);
  LSQ_COMMAND_OPTION_GET_CLASS(option)->get_default(option, value);
}

static void lsq_command_option_bool_get_default(LSQCommandOptionBool *option, GValue *value)
{
  g_value_set_boolean(value, option->default_value);
}

static void lsq_command_option_string_get_default(LSQCommandOptionString *option, GValue *value)
{
  g_value_set_string(value, option->default_value);
}

static void lsq_command_option_int_get_default(LSQCommandOptionInt *option, GValue *value)
{
  g_value_set_int(value, option->default_value);
}

static void lsq_command_option_uint_get_default(LSQCommandOptionUint *option, GValue *value)
{
  g_value_set_uint(value, option->default_value);
}

static void lsq_command_option_class_init(LSQCommandOptionClass *klass)
{
}

static void lsq_command_option_bool_class_init(LSQCommandOptionClass *klass)
{
  klass->get_default = (LSQCommandOptionGetDefaultFunc)lsq_command_option_bool_get_default;
}

static void lsq_command_option_string_class_init(LSQCommandOptionClass *klass)
{
  klass->get_default = (LSQCommandOptionGetDefaultFunc)lsq_command_option_string_get_default;
}

static void lsq_command_option_int_class_init(LSQCommandOptionClass *klass)
{
  klass->get_default = (LSQCommandOptionGetDefaultFunc)lsq_command_option_int_get_default;
}

static void lsq_command_option_uint_class_init(LSQCommandOptionClass *klass)
{
  klass->get_default = (LSQCommandOptionGetDefaultFunc)lsq_command_option_uint_get_default;
}

