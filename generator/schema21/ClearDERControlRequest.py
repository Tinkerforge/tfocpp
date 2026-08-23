from statham.schema.constants import Maybe
from statham.schema.elements import Boolean, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ClearDERControlRequest(Object, additionalProperties=False):

    isDefault: bool = Property(Boolean(description='True: clearing default DER controls. False: clearing scheduled controls.\r\n\r\n'), required=True)

    controlType: Maybe[str] = Property(String(enum=['EnterService', 'FreqDroop', 'FreqWatt', 'FixedPFAbsorb', 'FixedPFInject', 'FixedVar', 'Gradients', 'HFMustTrip', 'HFMayTrip', 'HVMustTrip', 'HVMomCess', 'HVMayTrip', 'LimitMaxDischarge', 'LFMustTrip', 'LVMustTrip', 'LVMomCess', 'LVMayTrip', 'PowerMonitoringMustTrip', 'VoltVar', 'VoltWatt', 'WattPF', 'WattVar'], description='Name of control settings to clear. Not used when _controlId_ is provided.\r\n\r\n'))

    controlId: Maybe[str] = Property(String(maxLength=36, description='Id of control setting to clear. When omitted all settings for _controlType_ are cleared.\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
