#pragma once

// Main state handlers file - includes all separated state handler files
// Organized by logical groupings and dependency order:
// - common_states.h: Root/entry point states (no dependencies)
// - variable_states.h: Variable keyword parsing (depends on common_states)
// - identifier_states.h: Identifier parsing (depends on variable_states)
// - type_annotation_states.h: Type annotation and equals parsing (depends on identifier_states)
// - expression_states.h: Expression parsing (depends on common_states)

#include "common_states.h"
#include "variable_states.h"
#include "identifier_states.h"
#include "type_annotation_states.h"
#include "expression_states.h"
#include "function_states.h"
