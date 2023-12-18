# JSON module graph queries

This directory contains `jq` scripts that query Soong's module graph.

It uses the JSON module graph that Soongs dumps when the
`SOONG_DUMP_JSON_MODULE_GRAPH` environment variable is set.

Usage:

```
SOONG_DUMP_JSON_MODULE_GRAPH=<some file> m nothing
query.sh [-C] <command> <some file> [argument]
```

The following commands are available:
* `printModule` prints all variations of a given module
* `filterSubtree` dumps only those modules that are in the given subtree of the
  source tree
* `directDeps` prints the names of the direct dependencies of the given module
* `transitiveDeps` prints the names of the transitive dependencies of the given
  module
* `fullTransitiveDeps` returns the full transitive dependencies of the given
   module
* `distanceFromLeaves` prints the longest distance each module has from a leaf
  in the module graph within the transitive closure of given module
* `usedVariations` returns a map that shows which variations are used in the
  input and what values they take
* `variantTransitions`  summarizes the variant transitions in the transitive
  closure of the given module

It's best to filter the full module graph to the part you are interested in
because `jq` isn't too fast on the full graph.
