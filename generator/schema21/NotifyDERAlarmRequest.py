from statham.schema.constants import Maybe
from statham.schema.elements import Boolean, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class NotifyDERAlarmRequest(Object, additionalProperties=False):

    controlType: str = Property(String(enum=['EnterService', 'FreqDroop', 'FreqWatt', 'FixedPFAbsorb', 'FixedPFInject', 'FixedVar', 'Gradients', 'HFMustTrip', 'HFMayTrip', 'HVMustTrip', 'HVMomCess', 'HVMayTrip', 'LimitMaxDischarge', 'LFMustTrip', 'LVMustTrip', 'LVMomCess', 'LVMayTrip', 'PowerMonitoringMustTrip', 'VoltVar', 'VoltWatt', 'WattPF', 'WattVar'], description='Name of DER control, e.g. LFMustTrip\r\n'), required=True)

    gridEventFault: Maybe[str] = Property(String(enum=['CurrentImbalance', 'LocalEmergency', 'LowInputPower', 'OverCurrent', 'OverFrequency', 'OverVoltage', 'PhaseRotation', 'RemoteEmergency', 'UnderFrequency', 'UnderVoltage', 'VoltageImbalance'], description='Type of grid event that caused this\r\n\r\n'))

    alarmEnded: Maybe[bool] = Property(Boolean(description='True when error condition has ended.\r\nAbsent or false when alarm has started.\r\n\r\n'))

    timestamp: str = Property(String(format='date-time', description='Time of start or end of alarm.\r\n\r\n'), required=True)

    extraInfo: Maybe[str] = Property(String(maxLength=200, description='Optional info provided by EV.\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
