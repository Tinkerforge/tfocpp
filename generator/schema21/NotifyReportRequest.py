from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import (
    Array,
    Boolean,
    Integer,
    Number,
    Object,
    String,
)
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class EVSEType(Object, additionalProperties=False):
    """Electric Vehicle Supply Equipment
"""

    id: int = Property(Integer(minimum=0.0, description='EVSE Identifier. This contains a number (&gt; 0) designating an EVSE of the Charging Station.\r\n'), required=True)

    connectorId: Maybe[int] = Property(Integer(minimum=0.0, description='An id to designate a specific connector (on an EVSE) by connector index number.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ComponentType(Object, additionalProperties=False):
    """A physical or logical component
"""

    evse: Maybe[EVSEType] = Property(EVSEType)

    name: str = Property(String(maxLength=50, description='Name of the component. Name should be taken from the list of standardized component names whenever possible. Case Insensitive. strongly advised to use Camel Case.\r\n'), required=True)

    instance: Maybe[str] = Property(String(maxLength=50, description='Name of instance in case the component exists as multiple instances. Case Insensitive. strongly advised to use Camel Case.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class VariableAttributeType(Object, additionalProperties=False):
    """Attribute data of a variable.
"""

    type: str = Property(String(default='Actual', enum=['Actual', 'Target', 'MinSet', 'MaxSet'], description='Attribute: Actual, MinSet, MaxSet, etc.\r\nDefaults to Actual if absent.\r\n'))

    value: Maybe[str] = Property(String(maxLength=2500, description="Value of the attribute. May only be omitted when mutability is set to 'WriteOnly'.\r\n\r\nThe Configuration Variable &lt;&lt;configkey-reporting-value-size,ReportingValueSize&gt;&gt; can be used to limit GetVariableResult.attributeValue, VariableAttribute.value and EventData.actualValue. The max size of these values will always remain equal. \r\n"))

    mutability: str = Property(String(default='ReadWrite', enum=['ReadOnly', 'WriteOnly', 'ReadWrite'], description='Defines the mutability of this attribute. Default is ReadWrite when omitted.\r\n'))

    persistent: bool = Property(Boolean(default=False, description='If true, value will be persistent across system reboots or power down. Default when omitted is false.\r\n'))

    constant: bool = Property(Boolean(default=False, description='If true, value that will never be changed by the Charging Station at runtime. Default when omitted is false.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class VariableCharacteristicsType(Object, additionalProperties=False):
    """Fixed read-only parameters of a variable.
"""

    unit: Maybe[str] = Property(String(maxLength=16, description='Unit of the variable. When the transmitted value has a unit, this field SHALL be included.\r\n'))

    dataType: str = Property(String(enum=['string', 'decimal', 'integer', 'dateTime', 'boolean', 'OptionList', 'SequenceList', 'MemberList'], description='Data type of this variable.\r\n'), required=True)

    minLimit: Maybe[float] = Property(Number(description='Minimum possible value of this variable.\r\n'))

    maxLimit: Maybe[float] = Property(Number(description='Maximum possible value of this variable. When the datatype of this Variable is String, OptionList, SequenceList or MemberList, this field defines the maximum length of the (CSV) string.\r\n'))

    maxElements: Maybe[int] = Property(Integer(minimum=1.0, description='*(2.1)* Maximum number of elements from _valuesList_ that are supported as _attributeValue_.\r\n'))

    valuesList: Maybe[str] = Property(String(maxLength=1000, description='Mandatory when _dataType_ = OptionList, MemberList or SequenceList. In that case _valuesList_ specifies the allowed values for the type.\r\n\r\nThe length of this field can be limited by DeviceDataCtrlr.ConfigurationValueSize.\r\n\r\n* OptionList: The (Actual) Variable value must be a single value from the reported (CSV) enumeration list.\r\n\r\n* MemberList: The (Actual) Variable value  may be an (unordered) (sub-)set of the reported (CSV) valid values list.\r\n\r\n* SequenceList: The (Actual) Variable value  may be an ordered (priority, etc)  (sub-)set of the reported (CSV) valid values.\r\n\r\nThis is a comma separated list.\r\n\r\nThe Configuration Variable &lt;&lt;configkey-configuration-value-size,ConfigurationValueSize&gt;&gt; can be used to limit SetVariableData.attributeValue and VariableCharacteristics.valuesList. The max size of these values will always remain equal. \r\n\r\n\r\n'))

    supportsMonitoring: bool = Property(Boolean(description='Flag indicating if this variable supports monitoring. \r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class VariableType(Object, additionalProperties=False):
    """Reference key to a component-variable.
"""

    name: str = Property(String(maxLength=50, description='Name of the variable. Name should be taken from the list of standardized variable names whenever possible. Case Insensitive. strongly advised to use Camel Case.\r\n'), required=True)

    instance: Maybe[str] = Property(String(maxLength=50, description='Name of instance in case the variable exists as multiple instances. Case Insensitive. strongly advised to use Camel Case.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ReportDataType(Object, additionalProperties=False):
    """Class to report components, variables and variable attributes and characteristics.
"""

    component: ComponentType = Property(ComponentType, required=True)

    variable: VariableType = Property(VariableType, required=True)

    variableAttribute: List[VariableAttributeType] = Property(Array(VariableAttributeType, additionalItems=False, minItems=1, maxItems=4), required=True)

    variableCharacteristics: Maybe[VariableCharacteristicsType] = Property(VariableCharacteristicsType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class NotifyReportRequest(Object, additionalProperties=False):

    requestId: int = Property(Integer(description='The id of the GetReportRequest  or GetBaseReportRequest that requested this report\r\n'), required=True)

    generatedAt: str = Property(String(format='date-time', description='Timestamp of the moment this message was generated at the Charging Station.\r\n'), required=True)

    reportData: Maybe[List[ReportDataType]] = Property(Array(ReportDataType, additionalItems=False, minItems=1))

    tbc: bool = Property(Boolean(default=False, description='“to be continued” indicator. Indicates whether another part of the report follows in an upcoming notifyReportRequest message. Default value when omitted is false.\r\n\r\n'))

    seqNo: int = Property(Integer(minimum=0.0, description='Sequence number of this message. First message starts at 0.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
