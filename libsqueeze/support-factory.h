#ifndef __LIBSQUEEZE_SUPPORT_FACTORY_H__
#define __LIBSQUEEZE_SUPPORT_FACTORY_H__

#define LSQ_TYPE_SUPPORT_FACTORY lsq_support_factory_get_type()

#define LSQ_SUPPORT_FACTORY(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_SUPPORT_FACTORY,      \
			LSQSupportFactory))

#define LSQ_IS_SUPPORT_FACTORY(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_SUPPORT_FACTORY))

#define LSQ_SUPPORT_FACTORY_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_SUPPORT_FACTORY,      \
			LSQSupportFactoryClass))

#define LSQ_IS_SUPPORT_FACTORY_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_SUPPORT_FACTORY))

typedef struct _LSQMimeSupport LSQMimeSupport;

struct _LSQMimeSupport
{
	gchar *mime;
	gchar **required_apps;
	gboolean supported;

	gchar **new_cmd_queue;
	gchar **add_cmd_queue;
	gchar **remove_cmd_queue;
	gchar **extract_cmd_queue;
	gchar **refresh_cmd_queue;
	LSQSupportType   support_mask;
};



typedef struct _LSQSupportFactory LSQSupportFactory;

struct _LSQSupportFactory
{
	GObject          parent;
	gchar           *id;
	GSList          *mime_support;
};

typedef struct _LSQSupportFactoryClass LSQSupportFactoryClass;

struct _LSQSupportFactoryClass
{
	GObjectClass parent;
};

GType                lsq_support_factory_get_type(void);
void                 lsq_support_factory_init_archive(LSQSupportFactory *builder, LSQArchive *archive);

#endif /* __LIBSQUEEZE_SUPPORT_FACTORY_H__ */
