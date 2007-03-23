#ifndef __LIBSQUEEZE_BUILDER_SETTINGS_H__
#define __LIBSQUEEZE_BUILDER_SETTINGS_H__

#define LSQ_TYPE_BUILDER_SETTINGS lsq_builder_settings_get_type()

#define LSQ_BUILDER_SETTINGS(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_BUILDER_SETTINGS,      \
			LSQBuilderSettings))

#define LSQ_IS_BUILDER_SETTINGS(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_BUILDER_SETTINGS))

#define LSQ_BUILDER_SETTINGS_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_BUILDER_SETTINGS,      \
			LSQBuilderSettingsClass))

#define LSQ_IS_BUILDER_SETTINGS_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_BUILDER_SETTINGS))

typedef struct _LSQBuilderSettings LSQBuilderSettings;

struct _LSQBuilderSettings
{
	GObject       parent;
	guint         n_properties;
	GType        *property_types;
	gchar       **property_names;
};

typedef struct _LSQBuilderSettingsClass LSQBuilderSettingsClass;

struct _LSQBuilderSettingsClass
{
	GObjectClass parent;
};

GType                lsq_builder_settings_get_type(void);
LSQBuilderSettings  *lsq_builder_settings_new();
guint                lsq_builder_settings_get_n_properties(LSQBuilderSettings *settings);
GType                lsq_builder_settings_get_property_type(LSQBuilderSettings *settings, guint n);
const gchar         *lsq_builder_settings_get_property_name(LSQBuilderSettings *settings, guint n);

#endif /* __LIBSQUEEZE_BUILDER_SETTINGS_H__ */
