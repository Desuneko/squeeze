/*
 *  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
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

#ifndef __SQRCHIVER_BUTTON_DRAG_BOX_H__
#define __SQRCHIVER_BUTTON_DRAG_BOX_H__
G_BEGIN_DECLS

#define SQ_TYPE_BUTTON_DRAG_BOX sq_button_drag_box_get_type()

#define SQ_BUTTON_DRAG_BOX(obj)(				\
		G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
			SQ_TYPE_BUTTON_DRAG_BOX,				  \
			SQButtonDragBox))

#define SQ_IS_BUTTON_DRAG_BOX(obj)	  ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),	\
			SQ_TYPE_BUTTON_DRAG_BOX))

#define SQ_BUTTON_DRAG_BOX_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),	 \
			SQ_TYPE_BUTTON_DRAG_BOX,	  \
			SQButtonDragBoxClass))

#define SQ_IS_BUTTON_DRAG_BOX_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),		\
			SQ_TYPE_BUTTON_DRAG_BOX()))	

typedef struct _SQButtonDragBox SQButtonDragBox;

struct _SQButtonDragBox
{
	GtkVBox parent;
	GtkWidget *visible_box;
	GtkWidget *hidden_box;
	GtkTargetEntry entry;
	GdkWindow *indicator;
	guint locked_buttons;
};

typedef struct _SQButtonDragBoxClass SQButtonDragBoxClass;

struct _SQButtonDragBoxClass
{
	GtkVBoxClass parent_class;
};

GType	  sq_button_drag_box_get_type();
GtkWidget *sq_button_drag_box_new();

void	   sq_button_drag_box_lock_buttons(SQButtonDragBox *box, guint buttons);
void	   sq_button_drag_box_add_fixed_button(SQButtonDragBox *box, const gchar *label, gpointer user_data);
void	   sq_button_drag_box_add_button(SQButtonDragBox *box, const gchar *label, gboolean visible, gpointer user_data);

GSList	*sq_button_drag_box_get_visible(SQButtonDragBox *box);
GSList	*sq_button_drag_box_get_hidden(SQButtonDragBox *box);

G_END_DECLS
#endif /* __SQRCHIVER_BUTTON_DRAG_BOX_H__*/
