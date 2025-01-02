// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#pragma once

#include "base/outcome.h"
#include "iceberg/compatibility_types.h"
#include "iceberg/datatypes.h"

#include <seastar/util/bool_class.hh>

namespace iceberg {

/**
   check_types - Performs a basic type check between two Iceberg field types,
   enforcing the Primitive Type Promotion policy laid out in
   https://iceberg.apache.org/spec/#schema-evolution

   - For non-primitive types, checks strict equality - i.e. struct == struct,
     list != map
   - Unwraps and compares input types, returns the result. Does not account for
     nesting.

   @param src  - The type of some field in an existing schema
   @param dest - The type of some field in a new schema, possibly compatible
                 with src.
   @return The result of the type check:
             - type_promoted::yes - if src -> dest is a valid type promotion
               e.g. int -> long
             - type_promoted::no  - if src == dest
             - compat_errc::mismatch - if src != dest but src -> dest is not
               permitted e.g. int -> string or struct -> list
 */
type_check_result
check_types(const iceberg::field_type& src, const iceberg::field_type& dest);

/**
 * annotate_schema_transform - Answers whether a one schema can "evolve" into
 * another, structurally.
 *
 * Traverse the schemas in lockstep, in a recursive depth-first fashion, marking
 * nested fields in one of the following ways:
 *   - for a field from the source schema:
 *     - the field does not exist in the dest schema (i.e. drop column)
 *     - a backwards compatible field exists in the dest schema (update column)
 *   - for a field from the dest schema:
 *     - the field receives its ID from a compatible field in the source schema
 *     - the field does not appear in the source schema (i.e. add column)
 *
 * Considerations:
 *   - This function modifies nested_field metadata, but ID updates are deferred
 *     to validate_schema_transform (see below). This is mainly to allow
 *     constness throughout the traversal, ensuring that we do not make
 *     inadvertent structural changes to either input struct.
 *
 * @param source - the schema we want to evolve _from_ (probably the current
 *                 schema for some table)
 * @param dest   - the proposed schema (probably extracted from some incoming
 *                 record)
 *
 * @return schema_transform_state (indicating success), or an error code
 */
schema_transform_result
annotate_schema_transform(const struct_type& source, const struct_type& dest);

} // namespace iceberg
