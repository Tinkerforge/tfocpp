#include "OcppConfiguration.h"

#include "OcppTools.h"

#include <string.h>

OcppConfiguration OcppConfiguration::integer(int32_t value,
                                             bool readonly,
                                             bool requires_reboot,
                                             int32_t min_,
                                             int32_t max_) {
    return OcppConfiguration{OcppConfigurationValueType::Integer, {.integer = {value,min_, max_}}, readonly, requires_reboot};
}

OcppConfiguration OcppConfiguration::boolean(bool value,
                          bool readonly,
                          bool requires_reboot) {
    return OcppConfiguration{OcppConfigurationValueType::Integer, {.boolean = {value}}, readonly, requires_reboot};
}

OcppConfiguration OcppConfiguration::csl(const char *value,
                                 size_t max_len,
                                 size_t max_elements,
                                 bool readonly,
                                 bool requires_reboot,
                                 const char **allowed_values,
                                 size_t allowed_values_len,
                                 bool prefix_index) {
    size_t len = sizeof(char) * max_len;

    auto result = OcppConfiguration{OcppConfigurationValueType::CSL, {.csl = {(char *)malloc(len), max_len, (size_t*)malloc(sizeof(size_t) * max_elements), 0, allowed_values, allowed_values_len, prefix_index, max_elements}}, readonly, requires_reboot};

    memset(result.value.csl.c, 0, len);
    strncpy(result.value.csl.c, value, len);

    return result;
}

ChangeConfigurationResponseStatus OcppConfiguration::setValue(const char *newValue) {
    if (readonly)
        return ChangeConfigurationResponseStatus::REJECTED;

    switch (type) {
        case OcppConfigurationValueType::Integer: {
                Opt<int32_t> opt = parse_int(newValue);
                if (!opt.is_set())
                    return ChangeConfigurationResponseStatus::REJECTED;

                int32_t parsed = opt.get();

                if (!(parsed >= value.integer.min_ && parsed <= value.integer.max_))
                    return ChangeConfigurationResponseStatus::REJECTED;

                value.integer.i = parsed;
                return requires_reboot ? ChangeConfigurationResponseStatus::REBOOT_REQUIRED : ChangeConfigurationResponseStatus::ACCEPTED;
            }
            break;
        case OcppConfigurationValueType::Boolean: {
                StaticJsonDocument<10> doc;
                if (deserializeJson(doc, newValue) != DeserializationError::Ok)
                    return ChangeConfigurationResponseStatus::REJECTED;

                value.boolean.b = doc.as<bool>();
                return requires_reboot ? ChangeConfigurationResponseStatus::REBOOT_REQUIRED : ChangeConfigurationResponseStatus::ACCEPTED;
            }
            break;
        case OcppConfigurationValueType::CSL: {
                size_t len = strlen(newValue);

                if (len > value.csl.len)
                    return ChangeConfigurationResponseStatus::REJECTED;

                std::unique_ptr<char[]> buf{new char[value.csl.len]};
                std::unique_ptr<size_t[]> parsed_buf{new size_t[value.csl.allowed_values_len]};

                memset(buf.get(), 0, value.csl.len);
                memcpy(buf.get(), newValue, len);

                size_t next_parsed_buf_insert = 0;
                size_t new_parsed_len = 0;

                char *context;
                char *token = strtok_r(buf.get(), ",", &context);
                if (token == nullptr)
                    return ChangeConfigurationResponseStatus::REJECTED;

                do {
                    while(isspace(*token))
                        ++token;

                    if (value.csl.prefix_index) {
                        char *num = strtok(token, "."); // This insers a null terminator. undo later
                        Opt<int32_t> opt = parse_int(num);
                        if (!opt.is_set())
                            return ChangeConfigurationResponseStatus::REJECTED;

                        if (opt.get() < 0 || opt.get() >= value.csl.allowed_values_len)
                            return ChangeConfigurationResponseStatus::REJECTED;

                        next_parsed_buf_insert = opt.get();
                        token += strlen(num) + 1; // Skip over number and .
                        num[strlen(num)] = '.'; // Reinsert . so that the next strtok_r call does not trip over the null terminator.
                    }

                    size_t idx;
                    if (!lookup_key(&idx, token, value.csl.allowed_values, value.csl.allowed_values_len))
                        return ChangeConfigurationResponseStatus::REJECTED;

                    parsed_buf[next_parsed_buf_insert] = idx;
                    ++next_parsed_buf_insert; //if prefix_index is set this will be overwritten anyway.
                    ++new_parsed_len;
                } while ((token = strtok_r(nullptr, ",", &context)) != nullptr);

                if (new_parsed_len > value.csl.max_num_allowed_values)
                    return ChangeConfigurationResponseStatus::REJECTED;

                // Reinsert , removed by outer strtok_r calls.
                for(size_t i = 0; i < len; ++i)
                    if (buf[i] == '\0')
                        buf[i] = ',';

                memcpy(value.csl.c, buf.get(), len);
                value.csl.len = len;

                memcpy(value.csl.parsed, parsed_buf.get(), sizeof(size_t) * new_parsed_len);
                value.csl.parsed_len = new_parsed_len;
                return requires_reboot ? ChangeConfigurationResponseStatus::REBOOT_REQUIRED : ChangeConfigurationResponseStatus::ACCEPTED;
            }
            break;
    }
    return ChangeConfigurationResponseStatus::REJECTED;
}
