/*
 * Copyright © 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file has been copied over from Raider.
 */


#include "raider-progress-paintable.h"

#include <adwaita.h>

#define SIZE 16

struct _RaiderProgressPaintable
{
    GObject parent_instance;

    GtkWidget *widget;

    double progress;
    char *icon_name;

    GtkIconPaintable *check_paintable;

    double check_progress;
};

static void raider_progress_paintable_paintable_init (GdkPaintableInterface *iface);
static void raider_progress_paintable_symbolic_paintable_init (GtkSymbolicPaintableInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (RaiderProgressPaintable, raider_progress_paintable, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                      raider_progress_paintable_paintable_init)
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_SYMBOLIC_PAINTABLE,
                                                      raider_progress_paintable_symbolic_paintable_init))

enum
{
    PROP_0,
    PROP_ICON_NAME,
    PROP_WIDGET,
    PROP_PROGRESS,
    LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

static void
cache_icons (RaiderProgressPaintable *self)
{
    if (self->icon_name == NULL)
    {
        return;
    }

    GdkDisplay *display = gtk_widget_get_display (self->widget);
    GtkIconTheme *theme = gtk_icon_theme_get_for_display (display);
    int scale = gtk_widget_get_scale_factor (self->widget);
    GtkTextDirection direction = gtk_widget_get_direction (self->widget);

    g_set_object (&self->check_paintable,
                  gtk_icon_theme_lookup_icon (theme, self->icon_name,
                                              NULL, SIZE, scale, direction,
                                              GTK_ICON_LOOKUP_FORCE_SYMBOLIC));
}

static void
scale_factor_changed_cb (RaiderProgressPaintable *self)
{
    cache_icons (self);
    gdk_paintable_invalidate_size (GDK_PAINTABLE (self));
}

static void
raider_progress_paintable_constructed (GObject *object)
{
    RaiderProgressPaintable *self = RAIDER_PROGRESS_PAINTABLE (object);

    g_signal_connect_swapped (self->widget, "notify::scale-factor",
                              G_CALLBACK (scale_factor_changed_cb), self);

    cache_icons (self);

    G_OBJECT_CLASS (raider_progress_paintable_parent_class)->constructed (object);
}

static void
raider_progress_paintable_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
    RaiderProgressPaintable *self = RAIDER_PROGRESS_PAINTABLE (object);

    switch (prop_id)
    {
        case PROP_ICON_NAME:
        {
            g_value_set_string (value, self->icon_name);
            break;
        }

        case PROP_WIDGET:
        {
            g_value_set_object (value, self->widget);
        }
        break;

        case PROP_PROGRESS:
        {
            g_value_set_double (value, self->progress);
        }
        break;

        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
    }
}

static void
raider_progress_paintable_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
    RaiderProgressPaintable *self = RAIDER_PROGRESS_PAINTABLE (object);

    switch (prop_id)
    {
        case PROP_ICON_NAME:
        {
            if (g_set_str (&self->icon_name, g_value_get_string (value)))
            {
                cache_icons (self);
            }
        }
        break;

        case PROP_WIDGET:
        {
            g_set_object (&self->widget, g_value_get_object (value));
        }
        break;

        case PROP_PROGRESS:
        {
            self->progress = g_value_get_double (value);
            gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
        }
        break;

        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
    }
}

static void
raider_progress_paintable_dispose (GObject *object)
{
    RaiderProgressPaintable *self = RAIDER_PROGRESS_PAINTABLE (object);

    printf("DESTROYED\n");

    g_clear_pointer (&self->icon_name, g_free);
    g_clear_object (&self->widget);
    g_clear_object (&self->check_paintable);

    G_OBJECT_CLASS (raider_progress_paintable_parent_class)->dispose (object);
}

static void
raider_progress_paintable_class_init (RaiderProgressPaintableClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->constructed = raider_progress_paintable_constructed;
    object_class->get_property = raider_progress_paintable_get_property;
    object_class->set_property = raider_progress_paintable_set_property;
    object_class->dispose = raider_progress_paintable_dispose;

    properties[PROP_ICON_NAME] =
        g_param_spec_string ("icon-name",
                             NULL, NULL,
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_WIDGET] =
        g_param_spec_object ("widget",
                             NULL, NULL,
                             GTK_TYPE_WIDGET,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    properties[PROP_PROGRESS] =
        g_param_spec_double ("progress",
                             NULL, NULL,
                             0, 1, 0,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
raider_progress_paintable_init (RaiderProgressPaintable *self)
{
}

static int
raider_progress_paintable_get_intrinsic_width (GdkPaintable *paintable)
{
    RaiderProgressPaintable *self = RAIDER_PROGRESS_PAINTABLE (paintable);

    return SIZE * gtk_widget_get_scale_factor (self->widget);
}

static int
raider_progress_paintable_get_intrinsic_height (GdkPaintable *paintable)
{
    RaiderProgressPaintable *self = RAIDER_PROGRESS_PAINTABLE (paintable);

    return SIZE * gtk_widget_get_scale_factor (self->widget);
}

static void
raider_progress_paintable_paintable_init (GdkPaintableInterface *iface)
{
    iface->get_intrinsic_width = raider_progress_paintable_get_intrinsic_width;
    iface->get_intrinsic_height = raider_progress_paintable_get_intrinsic_height;
}

static void
raider_progress_paintable_snapshot_symbolic (GtkSymbolicPaintable *paintable,
                                               GdkSnapshot          *gdk_snapshot,
                                               double                width,
                                               double                height,
                                               const GdkRGBA        *colors,
                                               gsize                 n_colors)
{
    RaiderProgressPaintable *self = RAIDER_PROGRESS_PAINTABLE (paintable);
    GtkSnapshot *snapshot = GTK_SNAPSHOT (gdk_snapshot);
    cairo_t *cr;
    double arc_end;
    GdkRGBA rgba;

    if (self->check_progress < 1)
    {
        gtk_snapshot_save (snapshot);
        gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (width / 2.0f, height / 2.0f));
        gtk_snapshot_scale (snapshot, 1.0f - self->check_progress, 1.0f - self->check_progress);
        gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (-width / 2.0f, -height / 2.0f));
        gtk_snapshot_restore (snapshot);
    }

    if (self->check_progress > 0)
    {
        gtk_snapshot_save (snapshot);
        gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (width / 2.0f, height / 2.0f));
        gtk_snapshot_scale (snapshot, self->check_progress, self->check_progress);
        gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (-width / 2.0f, -height / 2.0f));
        gtk_symbolic_paintable_snapshot_symbolic (GTK_SYMBOLIC_PAINTABLE (self->check_paintable),
                                                  gdk_snapshot, width, height, colors, n_colors);
        gtk_snapshot_restore (snapshot);
    }

    cr = gtk_snapshot_append_cairo (snapshot, &GRAPHENE_RECT_INIT (-2, -2, width + 4, width + 4));
    arc_end = self->progress * G_PI * 2 - G_PI / 2;

    cairo_translate (cr, width / 2.0, height / 2.0);

    gdk_cairo_set_source_rgba (cr, colors);
    cairo_arc (cr, 0, 0, width / 2.0 + 1, -G_PI / 2, arc_end);
    cairo_stroke (cr);

    rgba = *colors;
    rgba.alpha *= 0.25f;
    gdk_cairo_set_source_rgba (cr, &rgba);
    cairo_arc (cr, 0, 0, width / 2.0 + 1, arc_end, 3.0 * G_PI / 2.0);
    cairo_stroke (cr);
}

static void
raider_progress_paintable_symbolic_paintable_init (GtkSymbolicPaintableInterface *iface)
{
    iface->snapshot_symbolic = raider_progress_paintable_snapshot_symbolic;
}

GdkPaintable *raider_progress_paintable_new (GtkWidget *widget)
{
    g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

    return GDK_PAINTABLE (g_object_new (RAIDER_TYPE_PROGRESS_PAINTABLE, "widget", widget, NULL));
}

