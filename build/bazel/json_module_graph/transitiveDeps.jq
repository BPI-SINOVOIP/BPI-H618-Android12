include "library";

(moduleGraphNoVariants | removeSelfEdges) as $m |
  [$arg] |
  transitiveDeps($m)
