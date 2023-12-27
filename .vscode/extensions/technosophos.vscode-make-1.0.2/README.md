# Make: A VS Code extension for working with `make`

Many programming languages, such as C and Go, still rely upon `Makefile`s to handle builds. This extension provides features to ease working with Make in VS Code.

## Features

![Choose a target](https://github.com/technosophos/vscode-make/raw/master/images/make-commands.png)

- Run any `Makefile` target with ease. Just run CMD-SHIFT-P and type `make`. You will be prompted for a target.
- Don't remember all your `Makefile` targets? Run CMD-SHIFT-P `target` and you will be prompted with a list.

![Choose a target](https://github.com/technosophos/vscode-make/raw/master/images/choose-target.png)

## Requirements

- A version of `make` that supports table printing with `-p`.
- The `egrep` command (which is fairly standard)
- Currently, this is only tested on macOS, though it should work on Linux. Untested on Windows WSL.

## Known Issues

- None so far

## Release Notes

### 1.0.2

Refactored to use async/await, and to clean up code.

### 1.0.1

Icon added

### 1.0.0

Initial release of `Make` extension.

## Notes

Icon CC BY-NC-ND 2.5 https://www.iconfinder.com/icons/9109/advanced_makefile_options_settings_setup_text_icon