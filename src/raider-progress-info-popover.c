/* raider-progress-info-popover.c
 *
 * Copyright 2022 Alan Beveridge
 *
 * raider is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * raider is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "raider-progress-info-popover.h"
#include <glib/gi18n.h>
#include <gtk/gtk.h>

struct _RaiderProgressInfoPopover
{
    GtkPopover parent;
    GtkWidget *progress_bar;
    double progress;
};

G_DEFINE_TYPE(RaiderProgressInfoPopover, raider_progress_info_popover, GTK_TYPE_POPOVER)

enum
{
    PROP_0,
    PROP_PROGRESS,
    N_PROPS
};

static GParamSpec *obj_properties[N_PROPS] = {
    NULL,
};

/* This sets both the progress bar progress and its text. */
void raider_progress_info_popover_set_progress(RaiderProgressInfoPopover *popover, gdouble fraction)
{
    double percentage = fraction * 100;

    if (percentage == 0)
    {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(popover->progress_bar), _("Starting…"));
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(popover->progress_bar), 1);
    }
    else if (percentage < 100)
    {
        gchar *display = g_strdup_printf("%d%%", (int)percentage);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(popover->progress_bar), display);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(popover->progress_bar), fraction);
        g_free(display);
    }
    else
    {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(popover->progress_bar), _("Finishing up…"));
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(popover->progress_bar));
    }
}

static void raider_progress_info_popover_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    RaiderProgressInfoPopover *self = RAIDER_PROGRESS_INFO_POPOVER(object);
    if (property_id == PROP_PROGRESS)
        g_value_set_double(value, self->progress);
    else
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

static void raider_progress_info_popover_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    RaiderProgressInfoPopover *self = RAIDER_PROGRESS_INFO_POPOVER(object);
    if (property_id == PROP_PROGRESS)
    {
        self->progress = g_value_get_double(value);
        raider_progress_info_popover_set_progress(self, self->progress);
    }
    else
    {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void raider_progress_info_popover_init(RaiderProgressInfoPopover *popover)
{
    gtk_widget_init_template(GTK_WIDGET(popover));
    gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(popover->progress_bar), .02);
    popover->progress = 0.0;
}

static void raider_progress_info_popover_class_init(RaiderProgressInfoPopoverClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = raider_progress_info_popover_get_property;
    object_class->set_property = raider_progress_info_popover_set_property;

    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/Raider/raider-progress-info-popover.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderProgressInfoPopover, progress_bar);

    obj_properties[PROP_PROGRESS] = g_param_spec_double(
        "progress", "Progress", "Shredding progress",
        0.0, 1.0, 0.0, G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY
    );

    g_object_class_install_properties(object_class, N_PROPS, obj_properties);
}

RaiderProgressInfoPopover *raider_progress_info_popover_new()
{
    return g_object_new(raider_progress_info_popover_get_type(), NULL);
}

/* This is used when the spinner is shown instead of the progress icon. */
void raider_progress_info_popover_pulse(RaiderProgressInfoPopover *popover)
{
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(popover->progress_bar), _("Estimating…"));
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(popover->progress_bar));
}
