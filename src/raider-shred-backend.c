/* raider-shred-backend.c
 *
 * Copyright 2022 Alan Beveridge
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "raider-progress-icon.h"
#include "raider-progress-info-popover.h"

struct _RaiderShredBackend {
	GObject parent;

	GDataInputStream *data_stream;
};

G_DEFINE_TYPE(RaiderShredBackend, raider_shred_backend, G_TYPE_OBJECT )

enum {
	PROP_0,
	PROP_DATA_STREAM,
	N_PROPS
};

static GParamSpec *properties[N_PROPS];


static void raider_shred_backend_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	RaiderShredBackend *self = RAIDER_PROGRESS_ICON(object);

	switch (prop_id) {
	case PROP_DATA_STREAM:
		g_value_set_double(value, self->data_stream));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void raider_shred_backend_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	RaiderShredBackend *self = RAIDER_PROGRESS_ICON(object);

	switch (prop_id) {
	case PROP_DATA_STREAM:
		self->data_stream = g_value_get_object(value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void raider_shred_backend_class_init(RaiderShredBackendClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->get_property = raider_shred_backend_get_property;
	object_class->set_property = raider_shred_backend_set_property;

	properties[PROP_PROGRESS] =
		g_param_spec_double("data-stream",
				    "Data Stream",
				    "The input source for output from a cl program",
				    G_TYPE_OBJECT,
				    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_properties(object_class, N_PROPS, properties);
}

static void raider_shred_backend_init(RaiderShredBackend *icon)
{
}
