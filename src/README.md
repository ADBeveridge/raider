# Raider Source Code Notes

Note: Some code is commented with the message `NOT USED BECAUSE FLATPAK REMOVES ACCESS TO DEVICE FILES`. This code shows a handy list of thumb drives that are plugged into the computer. But to shred them, Raider needs administrator access to the device files that correspond with those thumb drives. And that is not possible in flatpak because it is sandboxed. Therefore they are commented out.

## Application
 - Files supplied on the commandline are opened in `raider_application_open`. It converts the double array of files into a `GList` and sends that `GList` to a newly created window.
 - When the `GAction` to exit is activated, it calls `raider_application_try_exit` which queries each open window to exit. If any window returns `TRUE` then shredding is ongoing, and cannot exit.
 - Files opened in the file chooser are converted into a `GList` and are sent to the active window.

## Window
 - `RaiderWindow` handles drag-and-drop. It converts the list of files dragged in and converts it to `GList` and sends it to its file loader.
 - `raider_window_open_files` is the file loader. It takes a `GList` of `GFile`s from the callee. It runs its internal worker function `raider_window_open_files_thread` asynchronously, which in turn calls a function that opens a single `GFile`. That function runs tests on that file, and if it passes, a `RaiderFileRow` is created and added to the window.
 - Whenever a `RaiderFileRow` destroys itself, it calls `raider_window_close_file`, which updates the user interface if necessary.
 - Launching shredding and aborting shredding and adding files are done asynchronously with `GTask` because it can take a while. See `raider_window_start_shredding` and `raider_window_abort_shredding` for more details.
 - When `RaiderWindow` is queried to exit, it returns `TRUE` if shredding. `TRUE` tells GTK+ internally to not call the default `close-request` signal handler. Otherwise, `FALSE` is returned, and `RaiderWindow` is closed by internal system of GTK+. Before `TRUE` is returned, and if `TRUE` will be returned, a dialog will be shown to the user if he wants to exit. If so, the shredding is aborted, to clean up running processes, and in the `raider_window_abort_files_finish` function, the return result from the message dialog that has been passed along is tested, and if it equals `exit`, the window is destroyed.

## File Row
 - When the `Shred` button is activated, `RaiderWindow` calls `raider_file_row_launch_shredding` on each item it has in its `GtkListBox`. This function creates a `GSubprocess` and an unique instance of `RaiderShredBackend`, which gives progress info, and also sets up a function that will be called when the process exits.

## Shred Backend
 - 
