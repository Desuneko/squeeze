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

#ifndef __SUPPORT_TEMPLATE_H__
#define __SUPPORT_TEMPLATE_H__

typedef enum
{
	LSQ_SUPPORT_FILES	= 1 << 0x0,
	LSQ_SUPPORT_FOLDERS  = 1 << 0x1,
	LSQ_SUPPORT_MANY	 = 1 << 0x2
} LSQSupportType;

typedef enum
{
	LSQ_COMMAND_TYPE_ADD,
	LSQ_COMMAND_TYPE_REMOVE,
	LSQ_COMMAND_TYPE_EXTRACT,
	LSQ_COMMAND_TYPE_REFRESH,
	LSQ_COMMAND_TYPE_OPEN,
	LSQ_COMMAND_TYPE_TEST
} LSQCommandType;

typedef struct _LSQSupportTemplate LSQSupportTemplate;

struct _LSQSupportTemplate
{
	const gchar *id;
	ThunarVfsMimeInfo *mime_info;
	gchar **required_apps;
	gboolean supported;

	gchar **new_cmd_queue;
	gchar **add_cmd_queue;
	gchar **remove_cmd_queue;
	gchar **extract_cmd_queue;
	gchar **refresh_cmd_queue;
	LSQSupportType   support_mask;
};


#endif /* __SUPPORT_TEMPLATE_H__ */
