#ifndef __RAIDERPREFERENCES_H
#define __RAIDERPREFERENCES_H

#include <gtk/gtk.h>
#include "raider-window.h"


#define RAIDER_PREFERENCES_TYPE (raider_preferences_get_type ())
G_DECLARE_FINAL_TYPE (RaiderPreferences, raider_preferences, RAIDER, PREFERENCES, GtkDialog)


RaiderPreferences        *raider_preferences_new          (RaiderWindow *win);


#endif /* __RAIDERPREFERENCES_H */
