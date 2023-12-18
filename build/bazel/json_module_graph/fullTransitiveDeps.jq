include "library";

[((moduleGraphNoVariants | removeSelfEdges) as $m |
  [$arg] |
  transitiveDeps($m)) as $names |
  .[] |
  select (IN(.Name; $names | .[]))] |
  sort_by(.Name)


