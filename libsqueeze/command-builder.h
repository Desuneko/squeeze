#ifndef __LIBSQUEEZE_COMMAND_BUILDER_H__
#define __LIBSQUEEZE_COMMAND_BUILDER_H__

#define LSQ_TYPE_COMMAND_BUILDER lsq_command_builder_get_type()

#define LSQ_COMMAND_BUILDER(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_COMMAND_BUILDER,      \
			LSQCommandBuilder))

#define LSQ_IS_COMMAND_BUILDER(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_COMMAND_BUILDER))

#define LSQ_COMMAND_BUILDER_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_COMMAND_BUILDER,      \
			LSQCommandBuilderClass))

#define LSQ_IS_COMMAND_BUILDER_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_COMMAND_BUILDER))

typedef struct _LSQCommandBuilder LSQCommandBuilder;

struct _LSQCommandBuilder
{
	GObject       parent;
	gchar        *id;
	gchar       **mime_types;

	LSQArchiveCommand *(*build_add)    (LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);
	LSQArchiveCommand *(*build_extract)(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *files);
	LSQArchiveCommand *(*build_remove) (LSQCommandBuilder *builder, LSQArchive *archive, GSList *file_iters);
	LSQArchiveCommand *(*build_refresh)(LSQCommandBuilder *builder, LSQArchive *archive);
	LSQArchiveCommand *(*build_open)   (LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);
};

typedef struct _LSQCommandBuilderClass LSQCommandBuilderClass;

struct _LSQCommandBuilderClass
{
	GObjectClass parent;
};

GType                lsq_command_builder_get_type(void);

#endif /* __LIBSQUEEZE_COMMAND_BUILDER_H__ */
