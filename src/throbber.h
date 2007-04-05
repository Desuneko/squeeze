/*-
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
 *
 * Modified by Stephan Arts <stephan@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SQ_THROBBER_H__
#define __SQ_THROBBER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS;

typedef struct _SQThrobberClass SQThrobberClass;
typedef struct _SQThrobber      SQThrobber;

#define SQ_TYPE_THROBBER            (sq_throbber_get_type ())
#define SQ_THROBBER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SQ_TYPE_THROBBER, SQThrobber))
#define SQ_THROBBER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SQ_TYPE_THROBBER, SQThrobberClass))
#define SQ_IS_THROBBER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SQ_TYPE_THROBBER))
#define SQ_IS_THROBBER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SQ_TYPE_THROBBER))
#define SQ_THROBBER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SQ_TYPE_THROBBER, SQThrobberClass))

GType      sq_throbber_get_type     (void) G_GNUC_CONST;

GtkWidget *sq_throbber_new          (void) G_GNUC_MALLOC;

gboolean   sq_throbber_get_animated (const SQThrobber *throbber);
void       sq_throbber_set_animated (SQThrobber       *throbber,
                                         gboolean              animated);

G_END_DECLS;

#endif /* !__SQ_THROBBER_H__ */
