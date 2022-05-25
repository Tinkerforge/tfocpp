#pragma once

#include <stdint.h>
#include <stddef.h>

#include <memory>
#include <limits>

#include "OcppMessages.h"

enum class OcppConfigurationValueType {
    Integer,
    Boolean,
    CSL
};

struct OcppConfiguration {
    OcppConfigurationValueType type;
    union {
        struct {
            int32_t i;
            int32_t min_;
            int32_t max_;
        } integer;
        struct {
            bool b;
        } boolean;
        struct {
            // csl as string ready to be sent with getconfiguration etc.
            char *c;
            size_t len;

            // parsed enum indices. buffer has to have size max_num_allowed_values
            size_t *parsed;
            size_t parsed_len;

            // valid enum value strings
            const char **allowed_values;
            size_t allowed_values_len;

            // is every value prefixed with the index? See ConnectorPhaseRotation
            bool prefix_index;

            // maximum number of allowed values. this is what gets reported as for example ConnectorPhaseRotationMaxLength
            size_t max_num_allowed_values;
        } csl;
    } value;

    bool readonly;
    bool requires_reboot;

    static OcppConfiguration integer(int32_t value,
                                     bool readonly,
                                     bool requires_reboot,
                                     int32_t min_=std::numeric_limits<int32_t>::min(),
                                     int32_t max_=std::numeric_limits<int32_t>::max());

    static OcppConfiguration boolean(bool value,
                                     bool readonly,
                                     bool requires_reboot);

    static OcppConfiguration csl(const char *value,
                                 size_t max_len,
                                 size_t max_elements,
                                 bool readonly,
                                 bool requires_reboot,
                                 const char **allowed_values,
                                 size_t allowed_values_len,
                                 bool prefix_index = false);

    ChangeConfigurationResponseStatus setValue(const char *newValue);
};
