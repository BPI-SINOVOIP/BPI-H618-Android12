Common library code used by Kotlin and JavaScript flicker library.

All elements in this package are shared between the Android codebase and
Winscope (development/tools/winscope), which is written in JavaScript.

We rely on Kotlin JS to translate this package into JavaScript, therefore, the classes
of this package must not rely on Java or Android specific elements.
