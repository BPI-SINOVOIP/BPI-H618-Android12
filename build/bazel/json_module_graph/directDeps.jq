include "library";

[.[] | select(.Name == $arg) | .Deps | map(.Name)] | flatten | unique | sort