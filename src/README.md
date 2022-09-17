# Raider Source Code Notes
## Application
 - Files supplied on the commandline are opened in `raider_application_open`. It converts the double array of files into a `GList` and sends that `GList` to a newly created window.
 - When the `GAction` to exit is activated, it calls `raider_application_try_exit` which queries each open window to exit. If any window returns `TRUE` then shredding is ongoing, and cannot exit.
 - Files opened in the file chooser are converted into a `GList` and are sent to the active window.

## Preferences
 - This is simply a control window for the `GSettings` file. It's preferences regard the options that are passed to the `shred` executable.

## Window

## File Row

## Shred Backend
