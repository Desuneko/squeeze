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
#include <glib.h>
#include <glib-object.h> 
#include <signal.h>

#include "libsqueeze.h"

#include "parser-context.h"
#include "parser.h"

G_DEFINE_ABSTRACT_TYPE(LSQParser, lsq_parser, G_TYPE_OBJECT);

static void
lsq_parser_init(LSQParser *self)
{
}

static void
lsq_parser_class_init(LSQParserClass *klass)
{
	klass->get_context = NULL;
}

LSQParserContext*
lsq_parser_get_context(LSQParser *self, LSQArchive *archive)
{
	LSQParserClass *klass = LSQ_PARSER_GET_CLASS(self);

	g_return_val_if_fail(klass->get_context, NULL);

	return klass->get_context(self, archive);
}

