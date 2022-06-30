/* raider.h
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

#ifndef _RAIDER_
#define _RAIDER_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RAIDER_TYPE_APPLICATION             (raider_get_type ())
#define RAIDER_APPLICATION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), RAIDER_TYPE_APPLICATION, Raider))
#define RAIDER_APPLICATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), RAIDER_TYPE_APPLICATION, RaiderClass))
#define RAIDER_IS_APPLICATION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RAIDER_TYPE_APPLICATION))
#define RAIDER_IS_APPLICATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), RAIDER_TYPE_APPLICATION))
#define RAIDER_APPLICATION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), RAIDER_TYPE_APPLICATION, RaiderClass))

typedef struct _RaiderClass RaiderClass;
typedef struct _Raider Raider;


struct _RaiderClass
{
    GtkApplicationClass parent_class;
};

struct _Raider
{
    GtkApplication parent_instance;
};

GType raider_get_type (void) G_GNUC_CONST;
Raider *raider_new (void);

/* Callbacks */
void
about_activated (GSimpleAction *simple,
                 GVariant      *parameter,
                 gpointer       user_data);

void
new_window_activated (GSimpleAction *simple,
                      GVariant *parameter,
                      gpointer user_data);

void
help_activated (GSimpleAction *simple,
                GVariant *parameter,
                gpointer user_data);

void
quit_activated (GSimpleAction *simple,
                GVariant *parameter,
                gpointer user_data);
void
preferences_activated (GSimpleAction *simple,
                       GVariant *parameter,
                       gpointer user_data);
G_END_DECLS

#endif /* _APPLICATION_H_ */

