# Translating
To create a new translation of File Shredder, open POEdit and select `Create new translation from POT file`, and in the file selection dialog select the `com.github.ADBeveridge.Raider.pot` in the `po` directory. Once you have finished translation open a pull request from the forked repository to the source repository on the `develop` branch.
___
To update the source translation from source code changes, please use this command in the project's root directory:
```sh
xgettext --from-code=UTF-8  --add-comments --keyword=_ --keyword=C_:1c,2 --output=po/com.github.ADBeveridge.Raider.pot -f po/POTFILES
```
