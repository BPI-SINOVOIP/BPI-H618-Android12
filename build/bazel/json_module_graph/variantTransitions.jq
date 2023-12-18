include "library";

# This filters out modules with "interesting" deps
def filterVariantTransitions:
  .[] | transformModuleReferences(emptyIfNull | removeLinkVariation | removeEmptyVariations) |
    filterMatchingDeps | select(.Deps | length > 0) |
    depDelta(.Variations) | depDelta(.DependencyVariations) |
    transformModule(flattenVariations) |
    deleteDependencyVariations |
    .Deps |= map(deleteDependencyVariations) |
    .Deps |= groupDeps
;

[filterVariantTransitions] | sort_by(.Name) | sort_by(.Type) | .[]
