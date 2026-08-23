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


class VariableMonitoringType(Object, additionalProperties=False):
    """A monitoring setting for a variable.
"""

    id: int = Property(Integer(minimum=0.0, description='Identifies the monitor.\r\n'), required=True)

    transaction: bool = Property(Boolean(description='Monitor only active when a transaction is ongoing on a component relevant to this transaction. \r\n'), required=True)

    value: float = Property(Number(description='Value for threshold or delta monitoring.\r\nFor Periodic or PeriodicClockAligned this is the interval in seconds.\r\n'), required=True)

    type: str = Property(String(enum=['UpperThreshold', 'LowerThreshold', 'Delta', 'Periodic', 'PeriodicClockAligned', 'TargetDelta', 'TargetDeltaRelative'], description='The type of this monitor, e.g. a threshold, delta or periodic monitor. \r\n'), required=True)

    severity: int = Property(Integer(minimum=0.0, description='The severity that will be assigned to an event that is triggered by this monitor. The severity range is 0-9, with 0 as the highest and 9 as the lowest severity level.\r\n\r\nThe severity levels have the following meaning: +\r\n*0-Danger* +\r\nIndicates lives are potentially in danger. Urgent attention is needed and action should be taken immediately. +\r\n*1-Hardware Failure* +\r\nIndicates that the Charging Station is unable to continue regular operations due to Hardware issues. Action is required. +\r\n*2-System Failure* +\r\nIndicates that the Charging Station is unable to continue regular operations due to software or minor hardware issues. Action is required. +\r\n*3-Critical* +\r\nIndicates a critical error. Action is required. +\r\n*4-Error* +\r\nIndicates a non-urgent error. Action is required. +\r\n*5-Alert* +\r\nIndicates an alert event. Default severity for any type of monitoring event.  +\r\n*6-Warning* +\r\nIndicates a warning event. Action may be required. +\r\n*7-Notice* +\r\nIndicates an unusual event. No immediate action is required. +\r\n*8-Informational* +\r\nIndicates a regular operational event. May be used for reporting, measuring throughput, etc. No action is required. +\r\n*9-Debug* +\r\nIndicates information useful to developers for debugging, not useful during operations.\r\n'), required=True)

    eventNotificationType: str = Property(String(enum=['HardWiredNotification', 'HardWiredMonitor', 'PreconfiguredMonitor', 'CustomMonitor'], description='*(2.1)* Type of monitor.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class VariableType(Object, additionalProperties=False):
    """Reference key to a component-variable.
"""

    name: str = Property(String(maxLength=50, description='Name of the variable. Name should be taken from the list of standardized variable names whenever possible. Case Insensitive. strongly advised to use Camel Case.\r\n'), required=True)

    instance: Maybe[str] = Property(String(maxLength=50, description='Name of instance in case the variable exists as multiple instances. Case Insensitive. strongly advised to use Camel Case.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class MonitoringDataType(Object, additionalProperties=False):
    """Class to hold parameters of SetVariableMonitoring request.
"""

    component: ComponentType = Property(ComponentType, required=True)

    variable: VariableType = Property(VariableType, required=True)

    variableMonitoring: List[VariableMonitoringType] = Property(Array(VariableMonitoringType, additionalItems=False, minItems=1), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class NotifyMonitoringReportRequest(Object, additionalProperties=False):

    monitor: Maybe[List[MonitoringDataType]] = Property(Array(MonitoringDataType, additionalItems=False, minItems=1))

    requestId: int = Property(Integer(description='The id of the GetMonitoringRequest that requested this report.\r\n\r\n'), required=True)

    tbc: bool = Property(Boolean(default=False, description='“to be continued” indicator. Indicates whether another part of the monitoringData follows in an upcoming notifyMonitoringReportRequest message. Default value when omitted is false.\r\n'))

    seqNo: int = Property(Integer(minimum=0.0, description='Sequence number of this message. First message starts at 0.\r\n'), required=True)

    generatedAt: str = Property(String(format='date-time', description='Timestamp of the moment this message was generated at the Charging Station.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
