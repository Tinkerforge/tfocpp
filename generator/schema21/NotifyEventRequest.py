from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Boolean, Integer, Object, String
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


class VariableType(Object, additionalProperties=False):
    """Reference key to a component-variable.
"""

    name: str = Property(String(maxLength=50, description='Name of the variable. Name should be taken from the list of standardized variable names whenever possible. Case Insensitive. strongly advised to use Camel Case.\r\n'), required=True)

    instance: Maybe[str] = Property(String(maxLength=50, description='Name of instance in case the variable exists as multiple instances. Case Insensitive. strongly advised to use Camel Case.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class EventDataType(Object, additionalProperties=False):
    """Class to report an event notification for a component-variable.
"""

    eventId: int = Property(Integer(minimum=0.0, description='Identifies the event. This field can be referred to as a cause by other events.\r\n\r\n'), required=True)

    timestamp: str = Property(String(format='date-time', description='Timestamp of the moment the report was generated.\r\n'), required=True)

    trigger: str = Property(String(enum=['Alerting', 'Delta', 'Periodic'], description='Type of trigger for this event, e.g. exceeding a threshold value.\r\n\r\n'), required=True)

    cause: Maybe[int] = Property(Integer(minimum=0.0, description='Refers to the Id of an event that is considered to be the cause for this event.\r\n\r\n'))

    actualValue: str = Property(String(maxLength=2500, description='Actual value (_attributeType_ Actual) of the variable.\r\n\r\nThe Configuration Variable &lt;&lt;configkey-reporting-value-size,ReportingValueSize&gt;&gt; can be used to limit GetVariableResult.attributeValue, VariableAttribute.value and EventData.actualValue. The max size of these values will always remain equal. \r\n\r\n'), required=True)

    techCode: Maybe[str] = Property(String(maxLength=50, description='Technical (error) code as reported by component.\r\n'))

    techInfo: Maybe[str] = Property(String(maxLength=500, description='Technical detail information as reported by component.\r\n'))

    cleared: Maybe[bool] = Property(Boolean(description="_Cleared_ is set to true to report the clearing of a monitored situation, i.e. a 'return to normal'. \r\n\r\n"))

    transactionId: Maybe[str] = Property(String(maxLength=36, description='If an event notification is linked to a specific transaction, this field can be used to specify its transactionId.\r\n'))

    component: ComponentType = Property(ComponentType, required=True)

    variableMonitoringId: Maybe[int] = Property(Integer(minimum=0.0, description='Identifies the VariableMonitoring which triggered the event.\r\n'))

    eventNotificationType: str = Property(String(enum=['HardWiredNotification', 'HardWiredMonitor', 'PreconfiguredMonitor', 'CustomMonitor'], description='Specifies the event notification type of the message.\r\n\r\n'), required=True)

    variable: VariableType = Property(VariableType, required=True)

    severity: Maybe[int] = Property(Integer(minimum=0.0, description='*(2.1)* Severity associated with the monitor in _variableMonitoringId_ or with the hardwired notification.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class NotifyEventRequest(Object, additionalProperties=False):

    generatedAt: str = Property(String(format='date-time', description='Timestamp of the moment this message was generated at the Charging Station.\r\n'), required=True)

    tbc: bool = Property(Boolean(default=False, description='“to be continued” indicator. Indicates whether another part of the report follows in an upcoming notifyEventRequest message. Default value when omitted is false. \r\n'))

    seqNo: int = Property(Integer(minimum=0.0, description='Sequence number of this message. First message starts at 0.\r\n'), required=True)

    eventData: List[EventDataType] = Property(Array(EventDataType, additionalItems=False, minItems=1), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
