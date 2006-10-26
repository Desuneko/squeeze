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

#ifdef DEBUG
//#define LXA_TRACE_ALLOCATION 1
#endif /* DEBUG */

#ifdef LXA_TRACE_ALLOCATION

void lxa_trace_init();
gpointer lxa_trace_add(const gchar *methode, guint size, gpointer pointer, const gchar *function, const gchar *file, guint line);
void lxa_trace_del(const gchar *methode, gpointer pointer, const gchar *function, const gchar *file, guint line);

#define LXA_TRACE_INIT lxa_trace_init()

#define LXA_NEW(type, count) lxa_trace_add("g_new", sizeof(type)*(count), g_new(type, count), __FUNCTION__, __FILE__, __LINE__)
#define LXA_NEW0(type, count) lxa_trace_add("g_new0", sizeof(type)*(count), g_new0(type, count), __FUNCTION__, __FILE__, __LINE__)
#define LXA_MALLOC(size) lxa_trace_add("g_malloc", size, g_malloc(size), __FUNCTION__, __FILE__, __LINE__)
#define LXA_MALLOC0(size) lxa_trace_add("g_malloc0", size, g_malloc0(size), __FUNCTION__, __FILE__, __LINE__)
#define LXA_FREE(pointer) { lxa_trace_del("g_free", pointer, __FUNCTION__, __FILE__, __LINE__); g_free(pointer); }

#ifdef USE_G_SLICE
#define LXA_SLICE_NEW(type) g_slice_new(type)
#define LXA_SLICE_NEW0(type) g_slice_new0(type)
#define LXA_SLICE_ALLOC(size) g_slice_alloc(size)
#define LXA_SLICE_ALLOC0(size) g_slice_alloc0(size)
#define LXA_SLICE_FREE(type, pointer) g_slice_free(type, pointer)
#define LXA_SLICE_FREE1(size, pointer) g_silce_free1(size, pointer)
#else
#define LXA_SLICE_NEW(type) LXA_NEW(type, 1)
#define LXA_SLICE_NEW0(type) LXA_NEW0(type, 1)
#define LXA_SLICE_ALLOC(size) LXA_MALLOC(size)
#define LXA_SLICE_ALLOC0(size) LXA_MALLOC0(size)
#define LXA_SLICE_FREE(type, pointer) LXA_FREE(pointer)
#define LXA_SLICE_FREE1(size, pointer) LXA_FREE(pointer)
#endif /* USE_G_SLICE */

#else /* LXA_TRACE_ALLOCATION */

#define LXA_TRACE_INIT

#define LXA_NEW(type, count) g_new(type, count)
#define LXA_NEW0(type, count) g_new0(type, count)
#define LXA_MALLOC(size) g_malloc(size)
#define LXA_MALLOC0(size) g_malloc0(size)
#define LXA_FREE(pointer) g_free(pointer)

#ifdef USE_G_SLICE
#define LXA_SLICE_NEW(type) g_slice_new(type)
#define LXA_SLICE_NEW0(type) g_slice_new0(type)
#define LXA_SLICE_ALLOC(size) g_slice_alloc(size)
#define LXA_SLICE_ALLOC0(size) g_slice_alloc0(size)
#define LXA_SLICE_FREE(type, pointer) g_slice_free(type, pointer)
#define LXA_SLICE_FREE1(size, pointer) g_silce_free1(size, pointer)
#else
#define LXA_SLICE_NEW(type) LXA_NEW(type, 1)
#define LXA_SLICE_NEW0(type) LXA_NEW0(type, 1)
#define LXA_SLICE_ALLOC(size) LXA_MALLOC(size)
#define LXA_SLICE_ALLOC0(size) LXA_MALLOC0(size)
#define LXA_SLICE_FREE(type, pointer) LXA_FREE(pointer)
#define LXA_SLICE_FREE1(size, pointer) LXA_FREE(pointer)
#endif /* USE_G_SLICE */

#endif /* LXA_TRACE_ALLOCATION */

const gchar            *lxa_tmp_dir;
GSList                 *lxa_archive_support_list;

/*
 * gint
 * lxa_execute(gchar *command)
 *
 * general function for executing child-apps
 */
gint 
lxa_execute(
            gchar *command, 
            LXAArchive *archive, 
            GChildWatchFunc function, 
            GIOFunc f_in, 
            GIOFunc f_out, 
            GIOFunc f_err);

gchar *
lxa_concat_filenames(GSList *filenames);
