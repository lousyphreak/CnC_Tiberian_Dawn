# Command and Conquer: Red Alert SDL3 Port

current progress is in `docs/PORTING_PROGRESS.md`, use it as reference, and keep it updated as you go. Make sure to reference it regularly to know what to do next. Also keep it up to date with new findings and detailed progress notes.

## Technology used

- SDL3
- CMake

## Building and testing

- use CMake to build the project, and make sure to set up the build system correctly for both Windows and Linux.
- use the original game data files for testing, and make sure to run the game to find any early problems with the port, such as crashes
- use asan and ubsan to catch memory errors and undefined behavior during testing, and fix any issues found as soon as possible.
- **ALWAYS** make sure all builds are clean and working


- **DO NOT COMMIT** - the user will do that
- **DO NOT CHANGE THE ORIGINAL GAME CODE** - unless needed to port to new functionality, like changing the file system to use SDL3, or changing the input handling to use SDL3, but do not change the original game logic or behavior unless absolutely necessary. if you need to change something in the original code, make sure to document it in the `PORTING_PROGRESS.md` and `PORTING_KNOWLEDGE.md` files, and explain why the change was needed and how it was implemented.
