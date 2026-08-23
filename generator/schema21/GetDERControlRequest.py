from statham.schema.constants import Maybe
from statham.schema.elements import Boolean, Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class GetDERControlRequest(Object, additionalProperties=False):

    requestId: int = Property(Integer(description='RequestId to be used in ReportDERControlRequest.\r\n'), required=True)

    isDefault: Maybe[bool] = Property(Boolean(description='True: get a default DER control. False: get a scheduled control.\r\n\r\n'))

    controlType: Maybe[str] = Property(String(enum=['EnterService', 'FreqDroop', 'FreqWatt', 'FixedPFAbsorb', 'FixedPFInject', 'FixedVar', 'Gradients', 'HFMustTrip', 'HFMayTrip', 'HVMustTrip', 'HVMomCess', 'HVMayTrip', 'LimitMaxDischarge', 'LFMustTrip', 'LVMustTrip', 'LVMomCess', 'LVMayTrip', 'PowerMonitoringMustTrip', 'VoltVar', 'VoltWatt', 'WattPF', 'WattVar'], description='Type of control settings to retrieve. Not used when _controlId_ is provided.\r\n\r\n'))

    controlId: Maybe[str] = Property(String(maxLength=36, description='Id of setting to get. When omitted all settings for _controlType_ are retrieved.\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
