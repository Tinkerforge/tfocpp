from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Number, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class SignedMeterValueType(Object, additionalProperties=False):
    """Represent a signed version of the meter value.
"""

    signedMeterData: str = Property(String(maxLength=32768, description='Base64 encoded, contains the signed data from the meter in the format specified in _encodingMethod_, which might contain more then just the meter value. It can contain information like timestamps, reference to a customer etc.\r\n'), required=True)

    signingMethod: Maybe[str] = Property(String(maxLength=50, description='*(2.1)* Method used to create the digital signature. Optional, if already included in _signedMeterData_. Standard values for this are defined in Appendix as SigningMethodEnumStringType.\r\n'))

    encodingMethod: str = Property(String(maxLength=50, description='Format used by the energy meter to encode the meter data. For example: OCMF or EDL.\r\n'), required=True)

    publicKey: Maybe[str] = Property(String(maxLength=2500, description='*(2.1)* Base64 encoded, sending depends on configuration variable _PublicKeyWithSignedMeterValue_.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class UnitOfMeasureType(Object, additionalProperties=False):
    """Represents a UnitOfMeasure with a multiplier
"""

    unit: str = Property(String(default='Wh', maxLength=20, description='Unit of the value. Default = "Wh" if the (default) measurand is an "Energy" type.\r\nThis field SHALL use a value from the list Standardized Units of Measurements in Part 2 Appendices. \r\nIf an applicable unit is available in that list, otherwise a "custom" unit might be used.\r\n'))

    multiplier: int = Property(Integer(default=0, description='Multiplier, this value represents the exponent to base 10. I.e. multiplier 3 means 10 raised to the 3rd power. Default is 0. +\r\nThe _multiplier_ only multiplies the value of the measurand. It does not specify a conversion between units, for example, kW and W.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SampledValueType(Object, additionalProperties=False):
    """Single sampled value in MeterValues. Each value can be accompanied by optional fields.

To save on mobile data usage, default values of all of the optional fields are such that. The value without any additional fields will be interpreted, as a register reading of active import energy in Wh (Watt-hour) units.
"""

    value: float = Property(Number(description='Indicates the measured value.\r\n\r\n'), required=True)

    measurand: str = Property(String(default='Energy.Active.Import.Register', enum=['Current.Export', 'Current.Export.Offered', 'Current.Export.Minimum', 'Current.Import', 'Current.Import.Offered', 'Current.Import.Minimum', 'Current.Offered', 'Display.PresentSOC', 'Display.MinimumSOC', 'Display.TargetSOC', 'Display.MaximumSOC', 'Display.RemainingTimeToMinimumSOC', 'Display.RemainingTimeToTargetSOC', 'Display.RemainingTimeToMaximumSOC', 'Display.ChargingComplete', 'Display.BatteryEnergyCapacity', 'Display.InletHot', 'Energy.Active.Export.Interval', 'Energy.Active.Export.Register', 'Energy.Active.Import.Interval', 'Energy.Active.Import.Register', 'Energy.Active.Import.CableLoss', 'Energy.Active.Import.LocalGeneration.Register', 'Energy.Active.Net', 'Energy.Active.Setpoint.Interval', 'Energy.Apparent.Export', 'Energy.Apparent.Import', 'Energy.Apparent.Net', 'Energy.Reactive.Export.Interval', 'Energy.Reactive.Export.Register', 'Energy.Reactive.Import.Interval', 'Energy.Reactive.Import.Register', 'Energy.Reactive.Net', 'EnergyRequest.Target', 'EnergyRequest.Minimum', 'EnergyRequest.Maximum', 'EnergyRequest.Minimum.V2X', 'EnergyRequest.Maximum.V2X', 'EnergyRequest.Bulk', 'Frequency', 'Power.Active.Export', 'Power.Active.Import', 'Power.Active.Setpoint', 'Power.Active.Residual', 'Power.Export.Minimum', 'Power.Export.Offered', 'Power.Factor', 'Power.Import.Offered', 'Power.Import.Minimum', 'Power.Offered', 'Power.Reactive.Export', 'Power.Reactive.Import', 'SoC', 'Voltage', 'Voltage.Minimum', 'Voltage.Maximum'], description='Type of measurement. Default = "Energy.Active.Import.Register"\r\n'))

    context: str = Property(String(default='Sample.Periodic', enum=['Interruption.Begin', 'Interruption.End', 'Other', 'Sample.Clock', 'Sample.Periodic', 'Transaction.Begin', 'Transaction.End', 'Trigger'], description='Type of detail value: start, end or sample. Default = "Sample.Periodic"\r\n'))

    phase: Maybe[str] = Property(String(enum=['L1', 'L2', 'L3', 'N', 'L1-N', 'L2-N', 'L3-N', 'L1-L2', 'L2-L3', 'L3-L1'], description='Indicates how the measured value is to be interpreted. For instance between L1 and neutral (L1-N) Please note that not all values of phase are applicable to all Measurands. When phase is absent, the measured value is interpreted as an overall value.\r\n'))

    location: str = Property(String(default='Outlet', enum=['Body', 'Cable', 'EV', 'Inlet', 'Outlet', 'Upstream'], description='Indicates where the measured value has been sampled. Default =  "Outlet"\r\n\r\n'))

    signedMeterValue: Maybe[SignedMeterValueType] = Property(SignedMeterValueType)

    unitOfMeasure: Maybe[UnitOfMeasureType] = Property(UnitOfMeasureType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class MeterValueType(Object, additionalProperties=False):
    """Collection of one or more sampled values in MeterValuesRequest and TransactionEvent. All sampled values in a MeterValue are sampled at the same point in time.
"""

    sampledValue: List[SampledValueType] = Property(Array(SampledValueType, additionalItems=False, minItems=1), required=True)

    timestamp: str = Property(String(format='date-time', description='Timestamp for measured value(s).\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class MeterValuesRequest(Object, additionalProperties=False):

    evseId: int = Property(Integer(minimum=0.0, description='This contains a number (&gt;0) designating an EVSE of the Charging Station. ‘0’ (zero) is used to designate the main power meter.\r\n'), required=True)

    meterValue: List[MeterValueType] = Property(Array(MeterValueType, additionalItems=False, minItems=1), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
