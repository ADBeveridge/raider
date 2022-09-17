# Raider Source Code Notes
## Application
 - Files supplied on the commandline are opened in `raider_application_open`. It converts the double array of files into a `GList` and sends that `GList` to a newly created window.
 - When the `GAction` to exit is activated, it calls `raider_application_try_exit` which queries each open window to exit. If any window returns `TRUE` then shredding is ongoing, and cannot exit.
 - Files opened in the file chooser are converted into a `GList` and are sent to the active window.

## Preferences
 - This is simply a control window for the `GSettings` file. It's preferences regard the options that are passed to the `shred` executable.

## Window
 - `RaiderWindow` handles drag-and-drop. It converts the list of files dragged in and converts it to `GList` and sends it to its file loader.
 - `raider_window_open_files` is the file loader. It takes a `GList` of `GFile`s from the callee. It runs its internal worker function `raider_window_open_files_thread` asynchronously, which in turn calls a function that opens a single `GFile`. That function runs tests on that file, and if it passes, a `RaiderFileRow` is created and added to the window.

## File Row
 -

## Shred Backend
