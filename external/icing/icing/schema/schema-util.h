// Copyright (C) 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ICING_SCHEMA_SCHEMA_UTIL_H_
#define ICING_SCHEMA_SCHEMA_UTIL_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/proto/schema.pb.h"

namespace icing {
namespace lib {

class SchemaUtil {
 public:
  using TypeConfigMap =
      std::unordered_map<std::string, const SchemaTypeConfigProto>;

  // Maps from a child type to the parent types that depend on it.
  // Ex. type A has a single property of type B
  // The dependency map will be { { "B", { "A" } } }
  using DependencyMap =
      std::unordered_map<std::string_view,
                         std::unordered_set<std::string_view>>;

  struct SchemaDelta {
    // Whether an indexing config has changed, requiring the index to be
    // regenerated. We don't list out all the types that make the index
    // incompatible because our index isn't optimized for that. It's much easier
    // to reset the entire index and reindex every document.
    bool index_incompatible = false;

    // Which schema types were present in the old schema, but were deleted from
    // the new schema.
    std::unordered_set<std::string> schema_types_deleted;

    // Which schema types had their SchemaTypeConfigProto changed in a way that
    // could invalidate existing Documents of that schema type.
    std::unordered_set<std::string> schema_types_incompatible;

    bool operator==(const SchemaDelta& other) const {
      return index_incompatible == other.index_incompatible &&
             schema_types_deleted == other.schema_types_deleted &&
             schema_types_incompatible == other.schema_types_incompatible;
    }
  };

  struct ParsedPropertyConfigs {
    // Mapping of property name to PropertyConfigProto
    std::unordered_map<std::string_view, const PropertyConfigProto*>
        property_config_map;

    // Total number of properties that have an indexing config
    int32_t num_indexed_properties = 0;

    // Total number of properties that were REQUIRED
    int32_t num_required_properties = 0;
  };

  // This function validates:
  //   1. SchemaTypeConfigProto.schema_type's must be unique
  //   2. Properties within one SchemaTypeConfigProto must be unique
  //   3. SchemaTypeConfigProtos.schema_type must be non-empty
  //   4. PropertyConfigProtos.property_name must be non-empty
  //   5. PropertyConfigProtos.property_name's must be unique within one
  //      SchemaTypeConfigProto
  //   6. PropertyConfigProtos.data_type cannot be UNKNOWN
  //   7. PropertyConfigProtos.data_type of DOCUMENT must also have a
  //      schema_type
  //   8. PropertyConfigProtos.cardinality cannot be UNKNOWN
  //   9. PropertyConfigProtos.schema_type's must correspond to a
  //      SchemaTypeConfigProto.schema_type
  //  10. Property names can only be alphanumeric.
  //  11. Any STRING data types have a valid string_indexing_config
  //  12. A SchemaTypeConfigProto cannot have a property whose schema_type is
  //      itself, thus creating an infinite loop.
  //  13. Two SchemaTypeConfigProtos cannot have properties that reference each
  //      other's schema_type, thus creating an infinite loop.
  //
  //  TODO(b/171996137): Clarify 12 and 13 are only for indexed properties, once
  //  document properties can be opted out of indexing.
  //
  // Returns:
  //   On success, a dependency map from each child types to all parent types
  //   that depend on it directly or indirectly.
  //   ALREADY_EXISTS for case 1 and 2
  //   INVALID_ARGUMENT for 3-13
  static libtextclassifier3::StatusOr<DependencyMap> Validate(
      const SchemaProto& schema);

  // Creates a mapping of schema type -> schema type config proto. The
  // type_config_map is cleared, and then each schema-type_config_proto pair is
  // placed in the given type_config_map parameter.
  static void BuildTypeConfigMap(const SchemaProto& schema,
                                 TypeConfigMap* type_config_map);

  // Parses the given type_config and returns a struct of easily-parseable
  // information about the properties.
  static ParsedPropertyConfigs ParsePropertyConfigs(
      const SchemaTypeConfigProto& type_config);

  // Computes the delta between the old and new schema. There are a few
  // differences that'll be reported:
  //   1. The derived index would be incompatible. This is held in
  //      `SchemaDelta.index_incompatible`.
  //   2. Some schema types existed in the old schema, but have been deleted
  //      from the new schema. This is held in
  //      `SchemaDelta.schema_types_deleted`
  //   3. A schema type's new definition would mean any existing data of the old
  //      definition is now incompatible.
  //
  // For case 1, the two schemas would result in an incompatible index if:
  //   1.1. The new SchemaProto has a different set of indexed properties than
  //        the old SchemaProto.
  //
  // For case 3, the two schemas would result in incompatible data if:
  //   3.1. A SchemaTypeConfig exists in the old SchemaProto, but is not in the
  //        new SchemaProto
  //   3.2. A property exists in the old SchemaTypeConfig, but is not in the new
  //        SchemaTypeConfig
  //   3.3. A property in the new SchemaTypeConfig and has a REQUIRED
  //        PropertyConfigProto.cardinality, but is not in the old
  //        SchemaTypeConfig
  //   3.4. A property is in both the old and new SchemaTypeConfig, but its
  //        PropertyConfigProto.data_type is different
  //   3.5. A property is in both the old and new SchemaTypeConfig, but its
  //        PropertyConfigProto.schema_type is different
  //   3.6. A property is in both the old and new SchemaTypeConfig, but its new
  //        PropertyConfigProto.cardinality is more restrictive. Restrictive
  //        scale defined as:
  //          LEAST <REPEATED - OPTIONAL - REQUIRED> MOST
  //
  // A property is defined by the combination of the
  // SchemaTypeConfig.schema_type and the PropertyConfigProto.property_name.
  //
  // Returns a SchemaDelta that captures the aforementioned differences.
  static const SchemaDelta ComputeCompatibilityDelta(
      const SchemaProto& old_schema, const SchemaProto& new_schema,
      const DependencyMap& new_schema_dependency_map);

  // Validates the 'property_name' field.
  //   1. Can't be an empty string
  //   2. Can only contain alphanumeric characters
  //
  // NOTE: schema_type is only used for logging. It is not necessary to populate
  // it.
  //
  // RETURNS:
  //   - OK if property_name is valid
  //   - INVALID_ARGUMENT if property name is empty or contains an
  //     non-alphabetic character.
  static libtextclassifier3::Status ValidatePropertyName(
      std::string_view property_name, std::string_view schema_type = "");

 private:
  // Validates the 'schema_type' field
  //
  // Returns:
  //   INVALID_ARGUMENT if 'schema_type' is an empty string.
  //   OK on success
  static libtextclassifier3::Status ValidateSchemaType(
      std::string_view schema_type);

  // Validates the 'data_type' field.
  //
  // Returns:
  //   INVALID_ARGUMENT if it's UNKNOWN
  //   OK on success
  static libtextclassifier3::Status ValidateDataType(
      PropertyConfigProto::DataType::Code data_type,
      std::string_view schema_type, std::string_view property_name);

  // Validates the 'cardinality' field.
  //
  // Returns:
  //   INVALID_ARGUMENT if it's UNKNOWN
  //   OK on success
  static libtextclassifier3::Status ValidateCardinality(
      PropertyConfigProto::Cardinality::Code cardinality,
      std::string_view schema_type, std::string_view property_name);

  // Checks that the 'string_indexing_config' satisfies the following rules:
  //   1. Only STRING data types can be indexed
  //   2. An indexed property must have a valid tokenizer type
  //
  // Returns:
  //   INVALID_ARGUMENT if any of the rules are not followed
  //   OK on success
  static libtextclassifier3::Status ValidateStringIndexingConfig(
      const StringIndexingConfig& config,
      PropertyConfigProto::DataType::Code data_type,
      std::string_view schema_type, std::string_view property_name);
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_SCHEMA_SCHEMA_UTIL_H_
