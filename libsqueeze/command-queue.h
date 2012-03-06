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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#ifndef __LIBSQUEEZE_COMMAND_QUEUE_H__
#define __LIBSQUEEZE_COMMAND_QUEUE_H__ 

G_BEGIN_DECLS

#define LSQ_TYPE_COMMAND_QUEUE lsq_command_queue_get_type()

#define LSQ_COMMAND_QUEUE(obj) ( \
        G_TYPE_CHECK_INSTANCE_CAST ((obj), \
            LSQ_TYPE_COMMAND_QUEUE, \
            LSQCommandQueue))

#define LSQ_IS_COMMAND_QUEUE(obj) ( \
        G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
            LSQ_TYPE_COMMAND_QUEUE))

#define LSQ_COMMAND_QUEUE_CLASS(klass) ( \
        G_TYPE_CHECK_CLASS_CAST ((klass), \
            LSQ_TYPE_COMMAND_QUEUE, \
            LSQCommandQueueClass))

#define LSQ_IS_COMMAND_QUEUE_CLASS(klass) ( \
        G_TYPE_CHECK_CLASS_TYPE ((klass), \
            LSQ_TYPE_COMMAND_QUEUE))

#define LSQ_COMMAND_QUEUE_GET_CLASS(obj) ( \
        G_TYPE_INSTANCE_GET_CLASS ((obj), \
            LSQ_TYPE_COMMAND_QUEUE, \
            LSQCommandQueueClass))

#if 0
typedef struct _LSQCommandQueue LSQCommandQueue;
#endif

typedef struct _LSQCommandQueueClass LSQCommandQueueClass;

typedef struct _LSQExecuteContext LSQExecuteContext;


GType
lsq_command_queue_get_type ( void ) G_GNUC_CONST;

LSQCommandQueue *
lsq_command_queue_new ( const gchar *command_string ) G_GNUC_WARN_UNUSED_RESULT;

LSQExecuteContext *
lsq_command_queue_execute (
        LSQCommandQueue *queue,
        LSQArchive *archive,
        gchar **files,
        const gchar *direcotry,
        LSQParser *parser
    ) G_GNUC_WARN_UNUSED_RESULT;


G_END_DECLS

#endif /* __LIBSQUEEZE_COMMAND_QUEUE_H__ */

