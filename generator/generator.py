from statham.schema.elements import Element, Object, Array, String, Integer, Number, Boolean
from statham.schema.property import Property
from typing import List
import re
import os

from schema import *

import inspect

from collections import namedtuple

# Not specified for OCPPJ 1.6
# However OCPPS 1.6 implies 32 bits
# And OCPP 2.0 specifies 32 bit signed for everything except transaction IDs
# those can be anything m(
# Some chargers do strange stuff, see e.g.
# https://github.com/RWTH-i5-IDSG/steve/issues/740
OCPP_INTEGER_TYPE = "int32_t"
OCPP_FLOAT_TYPE = "float"

struct_name_replacements = {
    "StopTransactionTransactionData": "MeterValue",
    "MeterValuesMeterValue": "MeterValue"
}

enum_name_replacements = {
    "EntriesChargingProfileKind": "ChargingProfileKind",
    "EntriesRecurrencyKind": "RecurrencyKind",
}


core_profile = [[ #send
        schema.Authorize.AuthorizeRequest,
        schema.BootNotification.BootNotificationRequest,
        schema.ChangeAvailabilityResponse.ChangeAvailabilityResponse,
        schema.ChangeConfigurationResponse.ChangeConfigurationResponse,
        schema.ClearCacheResponse.ClearCacheResponse,
        schema.DataTransfer.DataTransferRequest,
        schema.DataTransferResponse.DataTransferResponse,
        schema.GetConfigurationResponse.GetConfigurationResponse,
        schema.Heartbeat.HeartbeatRequest,
        schema.MeterValues.MeterValuesRequest,
        schema.RemoteStartTransactionResponse.RemoteStartTransactionResponse,
        schema.RemoteStopTransactionResponse.RemoteStopTransactionResponse,
        schema.ResetResponse.ResetResponse,
        schema.StartTransaction.StartTransactionRequest,
        schema.StatusNotification.StatusNotificationRequest,
        schema.StopTransaction.StopTransactionRequest,
        schema.UnlockConnectorResponse.UnlockConnectorResponse,
    ], [ #recv
        schema.AuthorizeResponse.AuthorizeResponse,
        schema.BootNotificationResponse.BootNotificationResponse,
        schema.ChangeAvailability.ChangeAvailabilityRequest,
        schema.ChangeConfiguration.ChangeConfigurationRequest,
        schema.ClearCache.ClearCacheRequest,
        schema.DataTransfer.DataTransferRequest,
        schema.DataTransferResponse.DataTransferResponse,
        schema.GetConfiguration.GetConfigurationRequest,
        schema.HeartbeatResponse.HeartbeatResponse,
        schema.MeterValuesResponse.MeterValuesResponse,
        schema.RemoteStartTransaction.RemoteStartTransactionRequest,
        schema.RemoteStopTransaction.RemoteStopTransactionRequest,
        schema.Reset.ResetRequest,
        schema.StartTransactionResponse.StartTransactionResponse,
        schema.StatusNotificationResponse.StatusNotificationResponse,
        schema.StopTransactionResponse.StopTransactionResponse,
        schema.UnlockConnector.UnlockConnectorRequest,
]]

firmware_management_profile = [[ #send
        schema.GetDiagnosticsResponse.GetDiagnosticsResponse,
        schema.DiagnosticsStatusNotification.DiagnosticsStatusNotificationRequest,
        schema.FirmwareStatusNotification.FirmwareStatusNotificationRequest,
        schema.UpdateFirmwareResponse.UpdateFirmwareResponse,
    ], [ #recv
        schema.GetDiagnostics.GetDiagnosticsRequest,
        schema.DiagnosticsStatusNotificationResponse.DiagnosticsStatusNotificationResponse,
        schema.FirmwareStatusNotificationResponse.FirmwareStatusNotificationResponse,
        schema.UpdateFirmware.UpdateFirmwareRequest,
]]

local_auth_list_management_profile = [[ #send
        schema.GetLocalListVersionResponse.GetLocalListVersionResponse,
        schema.SendLocalListResponse.SendLocalListResponse,
    ], [ #recv
        schema.GetLocalListVersion.GetLocalListVersionRequest,
        schema.SendLocalList.SendLocalListRequest, # TODO: figure out how to specify that list version may not be 0 or -1. See errata 4.0 3.52
]]

# Read Errata v4.0 section 3.2 before changing anything in the next three profiles! Don't undo the switcheroo
reservation_profile = [[ #send
        schema.CancelReservationResponse.CancelReservationResponse,
        schema.ReserveNowResponse.ReserveNowResponse,
    ], [ #recv
        schema.CancelReservation.CancelReservationRequest,
        schema.ReserveNow.ReserveNowRequest,
]]

smart_charging_profile = [[ #send
        schema.ClearChargingProfileResponse.ClearChargingProfileResponse,
        schema.GetCompositeScheduleResponse.GetCompositeScheduleResponse,
        schema.SetChargingProfileResponse.SetChargingProfileResponse,
    ], [ #recv
        schema.ClearChargingProfile.ClearChargingProfileRequest,
        schema.GetCompositeSchedule.GetCompositeScheduleRequest,
        schema.SetChargingProfile.SetChargingProfileRequest,
]]

remote_trigger_profile = [[ #send
        schema.TriggerMessageResponse.TriggerMessageResponse,
    ], [ #recv
        schema.TriggerMessage.TriggerMessageRequest,
]]


all_profiles = [core_profile,
                firmware_management_profile,
                local_auth_list_management_profile,
                reservation_profile,
                smart_charging_profile,
                remote_trigger_profile]

supported_profiles = [core_profile, smart_charging_profile]

# String:
# - enum
# - format: date-time, format: uri
# - max-length

# Integer: no special cases

# Number: always multipleOf=0.1

# Boolean: no special cases

# objects:
# - IdTagInfo

# Array unbounded ?!?
# - String
# - Object

def specialize_template(template_filename, destination_filename, replacements, check_completeness=True, remove_template=False):
    lines = []
    replaced = set()

    with open(template_filename, 'r') as f:
        for line in f.readlines():
            for key in replacements:
                replaced_line = line.replace(key, replacements[key])

                if replaced_line != line:
                    replaced.add(key)

                line = replaced_line

            lines.append(line)

    if check_completeness and replaced != set(replacements.keys()):
        raise GeneratorError('Not all replacements for {0} have been applied'.format(template_filename))

    with open(destination_filename, 'w') as f:
        f.writelines(lines)

    if remove_template:
        os.remove(template_filename)

def wrap_non_empty(prefix, middle, suffix):
    if len(middle) == 0:
        return ''

    return prefix + middle + suffix

def camel(*args):
    return "".join(x[0].upper() + x[1:] for x in args)

def req_param_check(message: str, name: str, p: Property):
    if isinstance(p.element, String):
        if p.element.enum and p.required:
            return 'if ({name} == {{{message}{name_camel}}}::NONE) {{ platform_printfln("Required {name} missing."); return DynamicJsonDocument{{0}}; }}'.format(message=message, name=name, name_camel=camel(name))
        if p.element.format:
            if p.element.format == "date-time":
                return 'if ({name} == OCPP_DATETIME_NOT_PASSED) {{ platform_printfln("Required {name} missing."); return DynamicJsonDocument{{0}}; }}'.format(name=name)
            elif p.element.format == "uri":
                raise Exception("URIs should only be received, not sent to the central station")
            else:
                raise Exception("Unknown format")
        return 'if ({name} == nullptr) {{ platform_printfln("Required {name} missing."); return DynamicJsonDocument{{0}}; }}'.format(name=name)
    elif isinstance(p.element, Integer):
        result = 'if ({name} == OCPP_INTEGER_NOT_PASSED) {{ platform_printfln("Required {name} missing."); return DynamicJsonDocument{{0}}; }}'.format(name=name) # will be removed later
        if p.element.minimum:
            result += '\n    if ({name} <= {minimum}) {{ platform_printfln("Required {name} missing."); return DynamicJsonDocument{{0}}; }}'.format(name=name, minimum=p.element.minimum)
        return result
    elif isinstance(p.element, Number):
        return 'if ({name} == OCPP_DECIMAL_NOT_PASSED) {{ platform_printfln("Required {name} missing."); return DynamicJsonDocument{{0}}; }}'.format(name=name) # will be removed later
    elif isinstance(p.element, Boolean):
        return '' # every boolean parameter is requred. no need to generate a check here: we will remove these checks anyway later, as non-required params don't have a default value.
    elif inspect.isclass(p.element):
        return 'if ({name} == nullptr) {{ platform_printfln("Required {name} missing."); return DynamicJsonDocument{{0}}; }}'.format(name=name)
    elif isinstance(p.element, Array):
        return 'if ({name} == nullptr) {{ platform_printfln("Required {name} missing."); return DynamicJsonDocument{{0}}; }}'.format(name=name)

    raise Exception("Not implemented yet")

def flatten(list_of_lists):
    return sum(list_of_lists, [])

def param_size(param_name, e: Element):
    if isinstance(e, String):
        if e.enum:
            # values are passed as const char *, ArduinoJson will not copy those.
            #return [0] #[max([len(x) for x in e.enum]) + 1]
            return ""
        if e.format:
            if e.format == "date-time":
                return 'param_size += OCPP_ISO_8601_MAX_LEN;'
            elif e.format == "uri":
                raise Exception("URIs should only be received, not sent to the central station")
            else:
                raise Exception("Unknown format")

        if e.maxLength:
             # values are passed as const char *, ArduinoJson will not copy those.
            #return [0] # [e.maxLength + 1]
            return ""

        # values are passed as const char *, ArduinoJson will not copy those.
        #return [0] #[256]
        return ""
    elif isinstance(e, Integer):
        #return [0]
        return ""
    elif isinstance(e, Number):
        #return [0]
        return ""
    elif isinstance(e, Boolean):
        #return [0]
        return ""
    elif inspect.isclass(e):
        #return flatten([param_size("lalala" + propname, prop.element) for propname, prop in e.properties.items()]) + ["JSON_OBJECT_SIZE({})".format(len(e.properties))]
        return """param_size += {}.jsonSize();""".format(param_name)
    elif isinstance(e, Array):
        return """
        param_size += JSON_ARRAY_SIZE({name}_length);
        for(size_t i = 0; i < {name}_length; ++i) {{
            {inner}
        }}
        """.format(name=param_name, inner=param_size("{}[i]".format(param_name), e.items))
        #return ["({}) * {}_length".format(param_size("lalala2", e.items), name)] + ["JSON_ARRAY_SIZE({}_length)".format(param_name)] # assume 10 items per array for now
        #pass

    return "2000"
    raise Exception("Not implemented yet")

def param_insertion(message: str, name: str, p: Property):
    if isinstance(p.element, String):
        if p.element.enum:
            return 'if ({name} != {{{message}{name_camel}}}::NONE) json.addMemberString("{name}", {{{message}{name_camel}}}Strings[(size_t){name}]);'.format(message=message, name=name, name_camel=camel(name))

        if p.element.format:
            if p.element.format == "date-time":
                return 'if ({name} != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string({name}, json, "{name}");'.format(name=name)
            elif p.element.format == "uri":
                raise Exception("URIs should only be received, not sent to the central station")
            else:
                raise Exception("Unknown format")

        return 'if ({name} != nullptr) json.addMemberString("{name}", {name});'.format(name=name)
    elif isinstance(p.element, Integer):
        return 'if ({name} != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("{name}", {name});'.format(name=name)
    elif isinstance(p.element, Number):
        return 'if (!isnan({name})) json.addMemberNumber("{name}", {name}, "%.1f");'.format(name=name)
    elif isinstance(p.element, Boolean):
        if not p.required:
            raise Exception("Non-required bools are not supported")
        return 'json.addMemberBoolean("{name}", {name});'.format(name=name)
    elif inspect.isclass(p.element):
        #return 'if ({name} != nullptr) {name}->serializeInto(json, "{name}");'.format(name=name)
        return 'if ({name} != nullptr) {{ json.addMemberObject("{name}"); {name}->serializeInto(json); json.endObject(); }}'.format(name=name)
    elif isinstance(p.element, Array):
        if not isinstance(p.element.items, String):
            return 'if ({name} != nullptr) {{ json.addMemberArray("{name}"); for(size_t i = 0; i < {name}_length; ++i) {{ json.addObject(); {name}[i].serializeInto(json); json.endObject(); }} json.endArray(); }}'.format(name=name)
        else:
            return 'if ({name} != nullptr) {{ json.addMemberArray("{name}"); for(size_t i = 0; i < {name}_length; ++i) {{ json.addString({name}[i]); }} json.endArray(); }}'.format(name=name)

    raise Exception("Not implemented yet")

EnumReq = namedtuple("EnumReq", "name entries add_none")

enums_to_generate: List[EnumReq] = []
structs_to_generate = {}

def param_arg(message: str, name: str, p: Property, strings_as_arrays=True, default_values=True):
    name_camel=camel(name)

    if isinstance(p.element, String):
        if p.element.enum:
            enums_to_generate.append(EnumReq("{message}{name_camel}".format(message=message, name_camel=name_camel), p.element.enum, True))
            return "{{{message}{name_camel}}} {name}{default}".format(message=message, name=name, name_camel=name_camel, default=" = {{{message}{name_camel}}}::NONE".format(message=message, name_camel=name_camel) if default_values and not p.required else "")

        if p.element.format:
            if p.element.format == "date-time":
                return 'time_t {name}{default}'.format(name=name, default=" = OCPP_DATETIME_NOT_PASSED" if default_values and not p.required else "")
            elif p.element.format == "uri":
                raise Exception("URIs should only be received, not sent to the central station")
            else:
                raise Exception("Unknown format")

        if not p.element.maxLength or not strings_as_arrays:
            return "const char *{}{}".format(name, " = nullptr" if default_values and not p.required else "")
        return "const char {}[{}]{}".format(name, p.element.maxLength + 1, " = nullptr" if default_values and not p.required else "")
    elif isinstance(p.element, Integer):
        return '{} {}{}'.format(OCPP_INTEGER_TYPE, name, " = OCPP_INTEGER_NOT_PASSED" if default_values and not p.required else "")
    elif isinstance(p.element, Number):
        return '{} {}{}'.format(OCPP_FLOAT_TYPE, name, " = OCPP_DECIMAL_NOT_PASSED" if default_values and not p.required else "")
    elif isinstance(p.element, Boolean):
        if default_values and not p.required:
            raise Exception("Non-required bools are not supported")
        return 'bool {}'.format(name)
    elif inspect.isclass(p.element):
        structs_to_generate["{message}{name_camel}".format(message=message, name_camel=name_camel)] = p.element
        return "{message}{name_camel} *{name}{default}".format(message=message, name=name, name_camel=name_camel, default=" = nullptr" if default_values and not p.required else "")
        #pass # at least the core profile does not pass objects, only arrays of objects
    elif isinstance(p.element, Array):
        if not isinstance(p.element.items, String):
            structs_to_generate["{message}{name_camel}".format(message=message, name_camel=name_camel)] = p.element.items
            return "{message}{name_camel} *{name}{default}, size_t {name}_length{len_default}".format(message=message, name_camel=name_camel, name=name, default=" = nullptr" if default_values and not p.required else "", len_default=" = 0" if default_values and not p.required else "")
        else:
            return "const char **{name}{default}, size_t {name}_length{len_default}".format(message=message, name_camel=name_camel, name=name, default=" = nullptr" if default_values and not p.required else "", len_default=" = 0" if default_values and not p.required else "")

    raise Exception("Not implemented yet")

def camel_to_upper_snake(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    name = name.replace(".", "")
    name = name.replace("-", "")
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).upper()

def generate_enum(name: str, entries: List[str], add_none=True):
    entries_set = []
    for x in entries:
        if x in entries_set:
            continue
        entries_set.append(x)

    s = len(entries_set) + (1 if add_none else 0)
    if s <= 255:
        type_ = 'uint8_t'
    elif s <= 65535:
        type_ = 'uint16_t'
    else:
        type_ = 'uint32_t'

    template = """extern const char * const {name}Strings[];

enum class {name} : {type_} {{
    {entries}{none}
}};
"""
    string_template =  """const char * const {name}Strings[] = {{
    {entry_strings}
}};
"""
    h_content = template.format(name=name, type_=type_, entries=",\n    ".join(camel_to_upper_snake(x) for x in entries_set), none=",\n    NONE" if add_none else "")
    cpp_content = string_template.format(name=name, entry_strings=",\n    ".join('"{}"'.format(x) for x in entries_set))

    return h_content, cpp_content

def generate_struct(name, obj: Object):
    template = """
struct {name} {{
    {members}

    void serializeInto(TFJsonSerializer &json);
}};
"""

    impl_template = """void {name}::serializeInto(TFJsonSerializer &json) {{
        {serialization}
    }}
"""

    h_content = template.format(name=name,
        members=wrap_non_empty("", ";\n    ".join([param_arg(name, k, v, strings_as_arrays=False).replace(",", ";") for k, v in obj.properties.items()]), ";"))
    cpp_content = impl_template.format(name=name,
                                       serialization="\n        ".join([param_insertion(name, k, v) for k, v in obj.properties.items()]),
                                       member_count=len(obj.properties.items()),
                                       param_sizes="\n".join([param_size(k, v.element) for k, v in obj.properties.items()]))

    return h_content, cpp_content

def get_member_validator(obj_name, param_name, element):
    helper_def = []
    helper_impl = ""
    parse_member = []

    impl_template = """static CallResponse parse{name}(JsonVariant var) {{
    {parse_member}

    return CallResponse{{CallErrorCode::OK, nullptr}};
}}"""

    check_type = """
    if (!var.is<{type}>())
        return CallResponse{{CallErrorCode::TypeConstraintViolation, "{member}: wrong type"}};
"""

    check_enum = """
    {{
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE({{{inner_member}}}Strings); ++i) {{
            if (strcmp(var.as<const char *>(), {{{inner_member}}}Strings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }}

        if (!found)
            return CallResponse{{CallErrorCode::PropertyConstraintViolation, "{member}: unknown enum value received"}};
    }}
"""

    check_datetime = """
    {{
        time_t result;
        if (!iso_string_to_unix_timestamp(var.as<const char *>(), &result))
            return CallResponse{{CallErrorCode::TypeConstraintViolation, "{member}: failed to parse as ISO 8601 date-time string"}};

        var.set(result);
    }}

"""
    check_uri = """"""

    check_string_len = """
    if (strlen(var.as<const char *>()) > {max_len})
        return CallResponse{{CallErrorCode::PropertyConstraintViolation, "{member}: string too long"}};
    """

    check_object = """
    {{
        CallResponse inner_result = parse{inner_member}Entries(var);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }}
"""

    check_array = """
    {{
        for(size_t i = 0; i < var.as<JsonArray>().size(); ++i) {{
            CallResponse inner_result = parse{inner_member}Entry(var[i]);
            if (inner_result.result != CallErrorCode::OK)
                return inner_result;
        }}
    }}"""

    type_ = None
    if isinstance(element, String):
        type_ = "const char *"
    elif isinstance(element, Integer):
        type_ = "int32_t"
    elif isinstance(element, Number):
        type_ = "float"
    elif isinstance(element, Boolean):
        type_ = "bool"
    elif inspect.isclass(element):
        type_ = "JsonObject"
    elif isinstance(element, Array):
        type_ = "JsonArray"

    if type_ is None:
        breakpoint()

    parse_member.append(check_type.format(member=param_name,
                                            type=type_))

    if isinstance(element, Integer) and element.minimum:
        pass # Check minimum constraint here

    if isinstance(element, String):
        if element.enum:
            parse_member.append(check_enum.format(inner_member=camel(obj_name, param_name), member=param_name))
            enums_to_generate.append(EnumReq(camel(obj_name, param_name), element.enum, False))
        if element.format:
            if element.format == "date-time":
                parse_member.append(check_datetime.format(member=param_name))
            elif element.format == "uri":
                parse_member.append(check_uri.format())
            else:
                raise Exception("Unknown format")

        if element.maxLength:
            parse_member.append(check_string_len.format(member=param_name,
                                                            max_len=element.maxLength))

    if inspect.isclass(element):
        parse_member.append(check_object.format(inner_member=camel(obj_name, param_name)))
        hdef, helper_impl = generate_recv_function(element, camel(obj_name, param_name)+"Entries")
        helper_def.append(hdef)

    if isinstance(element, Array):
        hdef, helper_impl = get_member_validator(obj_name, param_name + "Entry", element.items)
        helper_def.append(hdef)

        parse_member.append(check_array.format(inner_member=camel(obj_name, param_name)))

        #helper_def, helper_impl = generate_recv_function(element.items)

    return "\n".join(helper_def), "\n".join([helper_impl] + [impl_template.format(name=camel(obj_name, param_name), parse_member="\n    ".join(parse_member))])


views_to_generate = {}

def generate_view(name: str, obj: Object):
    print(name)
    template = """struct {name}View {{
    JsonObject _obj;

    {methods}
}};"""

    primitive_template = """
    {ret_type} {name}() {{
        {opt_check}
        return _obj["{name}"].as<{ret_type_plain}>();
    }}
"""

    enum_template = """
    {ret_type} {name}() {{
        {opt_check}
        return ({ret_type})_obj["{name}"].as<size_t>();
    }}
"""

    enum_opt_template = """
    {ret_type} {name}() {{
        {opt_check}
        return {ret_type}{{({ret_type_plain})_obj["{name}"].as<size_t>()}};
    }}
"""

    object_template = """
    {ret_type} {name}() {{
        {opt_check}
        return {ret_type}{{_obj["{name}"].as<JsonObject>()}};
    }}
"""

    object_opt_template = """
    {ret_type} {name}() {{
        {opt_check}
        return {ret_type}{{{ret_type_plain}{{_obj["{name}"].as<JsonObject>()}}}};
    }}
"""

    array_template = """
    size_t {name}_count() {{
        {size_opt_check}
        return _obj["{name}"].size();
    }}

    {ret_type} {name}(size_t i) {{
        {opt_check}
        return _obj["{name}"][i].as<{ret_type_plain}>();
    }}
"""

    object_array_template = """
    size_t {name}_count() {{
        {size_opt_check}
        return _obj["{name}"].size();
    }}

    {ret_type} {name}(size_t i) {{
        {opt_check}
        return {ret_type}{{_obj["{name}"][i]}};
    }}
"""

    methods = []

    def get_primitive_type(p):
        if isinstance(p, String):
            if p.enum:
                raise Exception("non-primitive param passed")
            elif p.format:
                if p.format == "date-time":
                    return "time_t"
                elif p.format == "uri":
                    return "/*todo uri*/const char *"
                else:
                    raise Exception("Unknown format")
            else:
                return "const char *"
        elif isinstance(p, Integer):
            return "int32_t"
        elif isinstance(p, Number):
            return "float"
        elif isinstance(p, Boolean):
            return "bool"
        else:
            raise Exception("non-primitive param passed")

    def add_opt(ret_type, param):
        if param.required:
            return ret_type
        return "Option<{}>".format(ret_type)

    def opt_check(param_name, param):
        if param.required:
            return ""
        return """if (!_obj.containsKey("{}"))
                return {{}};
            """.format(param_name)

    for param_name, param in obj.properties.items():
        if isinstance(param.element, String):
            if param.element.enum:
                t = enum_template if param.required else enum_opt_template
                methods.append(t.format(ret_type=add_opt("{{{}}}".format(camel(name, param_name)), param),
                                                    ret_type_plain="{{{}}}".format(camel(name, param_name)),
                                                    name=param_name,
                                                    opt_check=opt_check(param_name, param)))
            elif param.element.format:
                if param.element.format == "date-time":
                    methods.append(primitive_template.format(ret_type=add_opt("time_t", param), ret_type_plain="time_t",
                                                             name=param_name,
                                                             opt_check=opt_check(param_name, param)))
                elif param.element.format == "uri":
                    methods.append(primitive_template.format(ret_type=add_opt("/*todo uri*/ const char *", param), ret_type_plain="/*todo uri*/ const char *",
                                                             name=param_name,
                                                             opt_check=opt_check(param_name, param)))
                else:
                    raise Exception("Unknown format")
            else:
                methods.append(primitive_template.format(ret_type=add_opt(get_primitive_type(param.element), param), ret_type_plain=get_primitive_type(param.element),
                                                         name=param_name,
                                                         opt_check=opt_check(param_name, param)))

        elif isinstance(param.element, Integer) or isinstance(param.element, Number) or isinstance(param.element, Boolean):
            methods.append(primitive_template.format(ret_type=add_opt(get_primitive_type(param.element), param), ret_type_plain=get_primitive_type(param.element),
                                                     name=param_name,
                                                     opt_check=opt_check(param_name, param)))
        elif inspect.isclass(param.element):
            t = object_template if param.required else object_opt_template
            methods.append(t.format(ret_type=add_opt(camel(name, param_name, "EntriesView"), param),
                                    ret_type_plain=camel(name, param_name, "EntriesView"),
                                                  name=param_name,
                                                  opt_check=opt_check(param_name, param)))
        elif isinstance(param.element, Array):
            if inspect.isclass(param.element.items):
                methods.append(object_array_template.format(ret_type=add_opt(camel(name, param_name, "EntryEntriesView"), param),
                                                            name=param_name,
                                                            opt_check=opt_check(param_name, param),
                                                            size_opt_check=""))
            else:
                methods.append(array_template.format(ret_type=add_opt(get_primitive_type(param.element.items), param), ret_type_plain=get_primitive_type(param.element.items),
                                                     name=param_name,
                                                     opt_check=opt_check(param_name, param),
                                                     size_opt_check=""))
        else:
            raise Exception("Unknown param type")

    return template.format(name=name, methods="\n".join(methods))

def generate_recv_function(obj: Object, nameoverride=None, top_level=False):
    name = nameoverride if nameoverride else obj.__name__.removesuffix("Request")
    helper_defs = []
    helper_impls = []

    views_to_generate[name] = obj

    template = """CallResponse parse{name}(JsonObject obj);"""

    impl_template = """{static}CallResponse parse{name}(JsonObject obj) {{
    size_t keys_handled = 0;

    {parse_members}

    if (obj.size() != keys_handled) {{
        return CallResponse{{CallErrorCode::FormationViolation, "{name}: unknown members passed"}};
    }}

    return CallResponse{{CallErrorCode::OK, nullptr}};
}}"""

    check_required = """
    if (!obj.containsKey("{member}"))
        return CallResponse{{CallErrorCode::OccurenceConstraintViolation, "{member}: required, but missing"}};
"""

    check_optional_start = """if (obj.containsKey("{member}")) {{"""
    check_optional_end = """}}"""

    check_done = """++keys_handled;"""

    check_member = """{{
        CallResponse inner_result = parse{inner_member}(obj["{member}"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }}"""

    parse_members = []

    for param_name, param in obj.properties.items():
        if param.required:
            parse_members.append(check_required.format(member=param_name))
        else:
            parse_members.append(check_optional_start.format(member=param_name))

        helper_def, helper_impl = get_member_validator(name, param_name, param.element)
        if helper_def is not None:
            helper_defs.append(helper_def)
        if helper_impl is not None:
            helper_impls.append(helper_impl)

        parse_members.append(check_member.format(inner_member=camel(name, param_name), member=param_name))

        parse_members.append(check_done)

        if not param.required:
            parse_members.append(check_optional_end.format(member=param_name))

    if top_level:
        helper_defs.append(template.format(name=name))

    helper_impls.append(impl_template.format(name=name, parse_members = "\n    ".join(parse_members), static="static " if not top_level else ""))

    return "\n".join(helper_defs), "\n".join(helper_impls)


def generate_send_function(obj: Object):
    template = """struct {action} final : public ICall {{
    {attrs}

    {action}({call_id_param}{params});
    {action}(const {action}&) = delete;
    {action} &operator=(const {action}&) = delete;

    size_t serializeJson(char *buf, size_t buf_len) const override;
}};"""

    impl2_template = """
{action}::{action}({call_id_param}{params}) :
    ICall(CallAction::{action_upper}, {call_id}){attr_init}
{{}}

size_t {action}::serializeJson(char *buf, size_t buf_len) const {{
    TFJsonSerializer json{{buf, buf_len}};
    json.addArray();
        json.addNumber((int32_t)OcppRpcMessageType::CALL{result});
        {add_id}
        {add_action}
        json.addObject();
            {param_insertions}
        json.endObject();
    json.endArray();

    return json.end();
}}
"""

    is_response = not obj.__name__.endswith("Request")

    message = obj.__name__.removesuffix("Request")


    req_params = []
    opt_params = []
    attrs = []
    for param_name, param in obj.properties.items():
        if param.required:
            req_params.append(param_arg(message, param_name, param))
        else:
            opt_params.append(param_arg(message, param_name, param))
        attrs.append(param_arg(message, param_name, param, strings_as_arrays=False, default_values=False).replace(",", ";") + ";")

    params = req_params + opt_params

    h_content = template.format(
        action=obj.__name__.removesuffix("Request"),
        attrs="\n    ".join(attrs),
        call_id_param="const char *call_id,\n        " if is_response else "",
        params=",\n        ".join(params))


    req_params = []
    opt_params = []

    param_sizes = []
    param_insertions = []

    attr_init = []

    for param_name, param in obj.properties.items():

        if param.required:
            req_params.append(param_arg(message, param_name, param, default_values=False))
        else:
            opt_params.append(param_arg(message, param_name, param, default_values=False))

        param_sizes.append(param_size(param_name, param.element))
        param_insertions.append(param_insertion(message, param_name, param))
        attr_init.append("{name}({name})".format(name=param_name))
        if isinstance(param.element, Array):
            attr_init.append("{name}_length({name}_length)".format(name=param_name))

    params = req_params + opt_params

    cpp_content = impl2_template.format(
        action=obj.__name__.removesuffix("Request"),
        action_upper=camel_to_upper_snake(obj.__name__.removesuffix("Request")),
        call_id_param="const char *call_id,\n        " if is_response else "",
        params=",\n        ".join(params),
        attr_init=wrap_non_empty(",\n    ",",\n    ".join(attr_init),""),
        result="RESULT" if is_response else "",
        call_id="call_id" if is_response else "next_call_id++",
        add_id = "json.addString(this->ocppJcallId);" if is_response else "json.addNumber(this->ocppJmessageId, true);",
        add_action ="" if is_response else "json.addString(CallActionStrings[(size_t)this->action]);",
        param_count=len(obj.properties),
        param_sizes="\n".join(param_sizes),
        param_insertions="\n            ".join(param_insertions))

    return h_content, cpp_content

def newline_remove(s):
    for i in range(10):
        s = re.sub("\n\s*\n\s*\n", "\n\n", s)
    return s

def generate_on_call(all_messages: List[Object], supported_to_recv: List[Object]):
    h_content = "CallResponse callHandler(const char *uid, const char *action_string, JsonObject obj, OcppChargePoint *cp);\n"
    cpp_content = ""


    template = """CallResponse callHandler(const char *uid, const char *action_string, JsonObject obj, OcppChargePoint *cp) {{
    size_t action_idx = 0;
    if (!lookup_key(&action_idx, action_string, CallActionStrings, ARRAY_SIZE(CallActionStrings)))
        return CallResponse{{CallErrorCode::NotImplemented, "unknown action passed"}};

    CallAction action = (CallAction) action_idx;

    switch(action) {{{cases}
        {default_cases}
            return CallResponse{{CallErrorCode::NotSupported, "action not supported"}};
    }}

    SILENCE_GCC_UNREACHABLE();
}}"""

    case_template = """
        case CallAction::{actionNameUpper}: {{
            CallResponse res = parse{actionName}(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handle{actionName}(uid, {actionName}View{{obj}});
        }}
"""

    default_cases=["case CallAction::{}:".format(camel_to_upper_snake(x.__name__.removesuffix("Request"))) for x in all_messages]

    cases = [case_template.format(
                    actionNameUpper=camel_to_upper_snake(obj.__name__.removesuffix("Request")),
                    actionName=obj.__name__.removesuffix("Request"))
                for obj in supported_to_recv
                    if obj.__name__.endswith("Request")]

    to_remove = ["case CallAction::{}:".format(camel_to_upper_snake(obj.__name__.removesuffix("Request"))) for obj in supported_to_recv if obj.__name__.endswith("Request")]

    default_cases = [x for x in default_cases if x not in to_remove]
    default_cases.remove("case CallAction::DATA_TRANSFER_RESPONSE:") # this is duplicated because data transfers can be sent and received by both parties

    cpp_content += template.format(cases="\n".join(cases),
                                   default_cases="\n        ".join(default_cases))

    return h_content, cpp_content


def generate_on_call_response(all_messages: List[Object], supported_to_recv: List[Object]):
    h_content = "CallResponse callResultHandler(int32_t connectorId, CallAction resultTo, JsonObject obj, OcppChargePoint *cp);\n"
    cpp_content = ""

    template = """CallResponse callResultHandler(int32_t connectorId, CallAction resultTo, JsonObject obj, OcppChargePoint *cp) {{

    switch(resultTo) {{{cases}
        {default_cases}
            return CallResponse{{CallErrorCode::NotSupported, "action not supported"}};
    }}

    SILENCE_GCC_UNREACHABLE();
}}"""

    case_template = """
        case CallAction::{actionNameUpper}: {{
            CallResponse res = parse{actionName}(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handle{actionName}(connectorId, {actionName}View{{obj}});
        }}
"""

    default_cases=["case CallAction::{}:".format(camel_to_upper_snake(x.__name__.removesuffix("Request"))) for x in all_messages]

    cases = [case_template.format(
                    actionNameUpper=camel_to_upper_snake(obj.__name__.removesuffix("Response")),
                    actionName=obj.__name__)
                for obj in supported_to_recv
                    if obj.__name__.endswith("Response")]

    to_remove = ["case CallAction::{}:".format(camel_to_upper_snake(obj.__name__.removesuffix("Response"))) for obj in supported_to_recv if obj.__name__.endswith("Response")]

    default_cases = [x for x in default_cases if x not in to_remove]
    default_cases.remove("case CallAction::DATA_TRANSFER_RESPONSE:") # this is duplicated because data transfers can be sent and received by both parties

    cpp_content += template.format(cases="\n".join(cases),
                                   default_cases="\n        ".join(default_cases))

    return h_content, cpp_content

def common_suffix(a, b):
    suffix = ''
    for l, r in zip(reversed(a), reversed(b)):
        if l != r:
            break
        suffix = l + suffix
    return suffix

if __name__ == "__main__":
    send_fn_decls = []
    send_fn_impls = []
    enum_decls = []
    enum_impls = []
    struct_decls = []
    struct_impls = []

    recv_fn_decls = []
    recv_fn_impls = []

    for p in supported_profiles:
        for x in p[0]:
            decl, impl = generate_send_function(x)
            send_fn_decls.append(decl)
            send_fn_impls.append(impl)
        for x in p[1]:
            decl, impl = generate_recv_function(x, top_level=True)
            recv_fn_decls.append(decl)
            recv_fn_impls.append(impl)

    # generating structs can find more structs to generate.
    # Run generate_struct until we hit the fixpoint (i.e. all structs to be generated are discovered)
    # This looks bad, but should run at most two times for the core profile
    cpy = {}
    while cpy != structs_to_generate:
        cpy = structs_to_generate.copy()
        for name, prop in cpy.items():
            decl, impl = generate_struct(name, prop)

    cpy = {}
    for name, prop in structs_to_generate.items():
        for to_replace, replacement in struct_name_replacements.items():
            if name.startswith(to_replace):
                name = name.replace(to_replace, replacement)

                send_fn_decls = [x.replace(to_replace, replacement) for x in send_fn_decls]
                send_fn_impls = [x.replace(to_replace, replacement) for x in send_fn_impls]
                recv_fn_decls = [x.replace(to_replace, replacement) for x in recv_fn_decls]
                recv_fn_impls = [x.replace(to_replace, replacement) for x in recv_fn_impls]

        cpy[name] = prop

    # Now generate the structs for real
    for name, prop in cpy.items():
        decl, impl = generate_struct(name, prop)
        struct_decls.append(decl)
        struct_impls.append(impl)


    enums_set = []
    for name, entries, add_none in enums_to_generate:
        if any(inner_name for inner_name, _, _ in enums_set if name == inner_name):
            continue
        enums_set.append((name, entries, add_none))


    enums_required = []
    for l_tup in enums_set:
        l_name, l_entries, l_add_none = l_tup
        for r_tup in enums_required:
            r_name, r_entries, r_add_none, _ = r_tup
            if l_entries != r_entries or l_add_none != r_add_none:
                continue

            suffix = common_suffix(l_name, r_name)
            r_tup[0] = suffix
            r_tup[3].append(l_name)
            break
        else:
            enums_required.append([l_name, l_entries, l_add_none, [l_name]])


    for name, entries, add_none, replacements in enums_required:
        decl, impl = generate_enum(name, entries, add_none)
        enum_decls.append(decl)
        enum_impls.append(impl)

    h_callactionenum, cpp_callactionenum = generate_enum("CallAction", [obj.__name__.removesuffix("Request") for obj in flatten(flatten(all_profiles))], add_none=False)
    enum_decls.append(h_callactionenum)
    enum_impls.append(cpp_callactionenum)

    h, cpp = generate_on_call(flatten(flatten(all_profiles)), flatten([x[1] for x in supported_profiles]))
    h2, cpp2 = generate_on_call_response(flatten(flatten(all_profiles)), flatten([x[1] for x in supported_profiles]))

    enums = newline_remove("\n\n".join(enum_decls))
    structs = newline_remove("\n\n".join(reversed(struct_decls + [generate_view(k, v) for k, v in views_to_generate.items()])))
    messages = newline_remove("\n\n".join([h] + send_fn_decls + recv_fn_decls + [h2]))

    for name, _, _, replacements in enums_required:
        for r in replacements:
            enums = enums.replace("{{{}}}".format(r), name)
            structs = structs.replace("{{{}}}".format(r), name)
            messages = messages.replace("{{{}}}".format(r), name)

    for old, new in enum_name_replacements.items():
        enums = enums.replace(old, new)
        structs = structs.replace(old, new)
        messages = messages.replace(old, new)

    os.makedirs('generated', exist_ok=True)
    specialize_template("Messages.h.template", "generated/Messages.h", {
        "{{{enums}}}": '\n'.join([s.rstrip() for s in enums.split('\n')]),
        "{{{structs}}}": '\n'.join([s.rstrip() for s in structs.split('\n')]),
        "{{{messages}}}": '\n'.join([s.rstrip() for s in messages.split('\n')]),
    })

    enum_strings = newline_remove("\n\n".join(enum_impls))
    struct_method_impls = newline_remove("\n\n".join(struct_impls))
    message_impls = newline_remove("\n\n".join(send_fn_impls + recv_fn_impls + [cpp, cpp2]))

    for name, _, _, replacements in enums_required:
        for r in replacements:
            enum_strings = enum_strings.replace("{{{}}}".format(r), name)
            struct_method_impls = struct_method_impls.replace("{{{}}}".format(r), name)
            message_impls = message_impls.replace("{{{}}}".format(r), name)

    for old, new in enum_name_replacements.items():
        enum_strings = enum_strings.replace(old, new)
        struct_method_impls = struct_method_impls.replace(old, new)
        message_impls = message_impls.replace(old, new)

    specialize_template("Messages.cpp.template", "generated/Messages.cpp", {
        "{{{enum_strings}}}": '\n'.join([s.rstrip() for s in enum_strings.split('\n')]),
        "{{{struct_method_impls}}}": '\n'.join([s.rstrip() for s in struct_method_impls.split('\n')]),
        "{{{message_impls}}}": '\n'.join([s.rstrip() for s in message_impls.split('\n')]),
    })
