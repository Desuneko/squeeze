#include <glib.h>
#include <glib-object.h>

#include "archive.h"
#include "archive-support.h"
#include "compression-support.h"

#include "internals.h"

gint
lookup_archive_support( gconstpointer support , gconstpointer type)
{
	if(support == 0)
		return 1;
	if(((const LXAArchiveSupport *)support)->type == *(LXAArchiveType *)type)
		return 0;
	else
		return 1;
}

gint
lookup_compression_support( gconstpointer support , gconstpointer type)
{
	if(support == 0)
		return 1;
	if(((const LXACompressionSupport *)support)->type == *(LXACompressionType *)type)
		return 0;
	else
		return 1;
}
