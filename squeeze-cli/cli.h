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

#ifndef __SQUEEZE_CLI__H__
#define __SQUEEZE_CLI__H__
G_BEGIN_DECLS

#define SQ_TYPE_CLI sq_cli_get_type()

#define SQ_CLI(obj) (                       \
        G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
            SQ_TYPE_CLI,                    \
            SQCli))

#define SQ_IS_CLI(obj) (                    \
        G_TYPE_CHECK_INSTANCE_TYPE ((obj),  \
            SQ_TYPE_CLI))

#define SQ_CLI_CLASS(klass) (               \
        G_TYPE_CHECK_CLASS_CAST ((klass),   \
            SQ_TYPE_CLI,                    \
            SQCliClass))

#define SQ_IS_CLI_CLASS(klass) (            \
        G_TYPE_CHECK_CLASS_TYPE ((klass),   \
            SQ_TYPE_CLI()))	

typedef struct _SQCli SQCli;

typedef struct _SQCliClass SQCliClass;

GType sq_cli_get_type ( void );
SQCli * sq_cli_new ( void );

gint sq_cli_extract_archive ( SQCli *, GFile *, gchar * );
gint sq_cli_new_archive ( SQCli *, GFile *, GSList * );
gint sq_cli_add_archive ( SQCli *, GFile *, GSList * );
gint sq_cli_open_archive ( SQCli *, GFile * );

void sq_cli_set_silent ( SQCli *, gboolean );

G_END_DECLS
#endif /* __SQUEEZE_CLI__H__ */
