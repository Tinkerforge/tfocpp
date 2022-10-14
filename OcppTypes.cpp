#include "OcppTypes.h"

const char * const CallErrorCodeStrings[] {
    "NotImplemented",
    "NotSupported",
    "InternalError",
    "ProtocolError",
    "SecurityError",
    "FormationViolation",
    "PropertyConstraintViolation",
    "OccurenceConstraintViolation",
    "TypeConstraintViolation",
    "GenericError",
};

const char * const CallErrorCodeStringAliases[] {
    "FormatViolation",
    "OccurrenceConstraintViolation"
};

const size_t CallErrorCodeStringAliasIndices[] {
    (size_t) CallErrorCode::FormationViolation,
    (size_t) CallErrorCode::OccurenceConstraintViolation
};

const size_t CallErrorCodeStringAliasLength = 2;
