#ifndef __RAIDERPREFERENCES_H
#define __RAIDERPREFERENCES_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include "raider-window.h"


#define RAIDER_TYPE_PREFERENCES (raider_preferences_get_type ())
G_DECLARE_FINAL_TYPE (RaiderPreferences, raider_preferences, RAIDER, PREFERENCES, AdwPreferencesWindow)

#endif /* __RAIDERPREFERENCES_H */
