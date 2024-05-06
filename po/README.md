# Translating

## New Translations
To create a new translation of File Shredder, open POEdit and select `Create new translation from POT file`, and in the file selection dialog select the `com.github.ADBeveridge.Raider.pot` in the `po` directory. Once you have finished translations open a pull request from the forked repository to the source repository on the `develop` branch.

## Updating Translations
Whenever you add or update a translation, please also update the source translation from source code changes with this command in the project's root directory:
```sh
xgettext --from-code=UTF-8  --add-comments --keyword=_ --keyword=C_:1c,2 --output=po/com.github.ADBeveridge.Raider.pot -f po/POTFILES
```
Then open your translation in POEdit and click `Update from POT File`, which is under the `Translation` menu. Retranslate and open a pull request on GitHub, and I should merge it in a few days. 

Thank you for translating this project.
