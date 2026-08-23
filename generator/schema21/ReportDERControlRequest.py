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


class DERCurvePointsType(Object, additionalProperties=False):

    x: float = Property(Number(description='The data value of the X-axis (independent) variable, depending on the curve type.\r\n\r\n\r\n'), required=True)

    y: float = Property(Number(description='The data value of the Y-axis (dependent) variable, depending on the  &lt;&lt;cmn_derunitenumtype&gt;&gt; of the curve. If _y_ is power factor, then a positive value means DER is absorbing reactive power (under-excited), a negative value when DER is injecting reactive power (over-excited).\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class EnterServiceType(Object, additionalProperties=False):

    priority: int = Property(Integer(minimum=0.0, description='Priority of setting (0=highest)\r\n\r\n'), required=True)

    highVoltage: float = Property(Number(description='Enter service voltage high\r\n'), required=True)

    lowVoltage: float = Property(Number(description='Enter service voltage low\r\n\r\n\r\n'), required=True)

    highFreq: float = Property(Number(description='Enter service frequency high\r\n\r\n'), required=True)

    lowFreq: float = Property(Number(description='Enter service frequency low\r\n\r\n\r\n'), required=True)

    delay: Maybe[float] = Property(Number(description='Enter service delay\r\n\r\n\r\n'))

    randomDelay: Maybe[float] = Property(Number(description='Enter service randomized delay\r\n\r\n\r\n'))

    rampRate: Maybe[float] = Property(Number(description='Enter service ramp rate in seconds\r\n\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class EnterServiceGetType(Object, additionalProperties=False):

    enterService: EnterServiceType = Property(EnterServiceType, required=True)

    id: str = Property(String(maxLength=36, description='Id of setting\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class FixedPFType(Object, additionalProperties=False):

    priority: int = Property(Integer(minimum=0.0, description='Priority of setting (0=highest)\r\n'), required=True)

    displacement: float = Property(Number(description='Power factor, cos(phi), as value between 0..1.\r\n'), required=True)

    excitation: bool = Property(Boolean(description='True when absorbing reactive power (under-excited), false when injecting reactive power (over-excited).\r\n'), required=True)

    startTime: Maybe[str] = Property(String(format='date-time', description='Time when this setting becomes active\r\n'))

    duration: Maybe[float] = Property(Number(description='Duration in seconds that this setting is active.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class FixedPFGetType(Object, additionalProperties=False):

    fixedPF: FixedPFType = Property(FixedPFType, required=True)

    id: str = Property(String(maxLength=36, description='Id of setting.\r\n'), required=True)

    isDefault: bool = Property(Boolean(description='True if setting is a default control.\r\n'), required=True)

    isSuperseded: bool = Property(Boolean(description='True if this setting is superseded by a lower priority setting.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class FixedVarType(Object, additionalProperties=False):

    priority: int = Property(Integer(minimum=0.0, description='Priority of setting (0=highest)\r\n'), required=True)

    setpoint: float = Property(Number(description='The value specifies a target var output interpreted as a signed percentage (-100 to 100). \r\n    A negative value refers to charging, whereas a positive one refers to discharging.\r\n    The value type is determined by the unit field.\r\n'), required=True)

    unit: str = Property(String(enum=['Not_Applicable', 'PctMaxW', 'PctMaxVar', 'PctWAvail', 'PctVarAvail', 'PctEffectiveV'], description='Unit of the Y-axis of DER curve\r\n'), required=True)

    startTime: Maybe[str] = Property(String(format='date-time', description='Time when this setting becomes active.\r\n'))

    duration: Maybe[float] = Property(Number(description='Duration in seconds that this setting is active.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class FixedVarGetType(Object, additionalProperties=False):

    fixedVar: FixedVarType = Property(FixedVarType, required=True)

    id: str = Property(String(maxLength=36, description='Id of setting\r\n\r\n'), required=True)

    isDefault: bool = Property(Boolean(description='True if setting is a default control.\r\n'), required=True)

    isSuperseded: bool = Property(Boolean(description='True if this setting is superseded by a lower priority setting\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class FreqDroopType(Object, additionalProperties=False):

    priority: int = Property(Integer(minimum=0.0, description='Priority of setting (0=highest)\r\n\r\n\r\n'), required=True)

    overFreq: float = Property(Number(description='Over-frequency start of droop\r\n\r\n\r\n'), required=True)

    underFreq: float = Property(Number(description='Under-frequency start of droop\r\n\r\n\r\n'), required=True)

    overDroop: float = Property(Number(description='Over-frequency droop per unit, oFDroop\r\n\r\n\r\n'), required=True)

    underDroop: float = Property(Number(description='Under-frequency droop per unit, uFDroop\r\n\r\n'), required=True)

    responseTime: float = Property(Number(description='Open loop response time in seconds\r\n\r\n'), required=True)

    startTime: Maybe[str] = Property(String(format='date-time', description='Time when this setting becomes active\r\n\r\n\r\n'))

    duration: Maybe[float] = Property(Number(description='Duration in seconds that this setting is active\r\n\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class FreqDroopGetType(Object, additionalProperties=False):

    freqDroop: FreqDroopType = Property(FreqDroopType, required=True)

    id: str = Property(String(maxLength=36, description='Id of setting\r\n\r\n'), required=True)

    isDefault: bool = Property(Boolean(description='True if setting is a default control.\r\n'), required=True)

    isSuperseded: bool = Property(Boolean(description='True if this setting is superseded by a higher priority setting (i.e. lower value of _priority_)\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class GradientType(Object, additionalProperties=False):

    priority: int = Property(Integer(minimum=0.0, description='Id of setting\r\n\r\n\r\n'), required=True)

    gradient: float = Property(Number(description='Default ramp rate in seconds (0 if not applicable)\r\n\r\n\r\n'), required=True)

    softGradient: float = Property(Number(description='Soft-start ramp rate in seconds (0 if not applicable)\r\n\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class GradientGetType(Object, additionalProperties=False):

    gradient: GradientType = Property(GradientType, required=True)

    id: str = Property(String(maxLength=36, description='Id of setting\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class HysteresisType(Object, additionalProperties=False):

    hysteresisHigh: Maybe[float] = Property(Number(description='High value for return to normal operation after a grid event, in absolute value. This value adopts the same unit as defined by yUnit\r\n\r\n\r\n'))

    hysteresisLow: Maybe[float] = Property(Number(description='Low value for return to normal operation after a grid event, in absolute value. This value adopts the same unit as defined by yUnit\r\n\r\n\r\n'))

    hysteresisDelay: Maybe[float] = Property(Number(description='Delay in seconds, once grid parameter within HysteresisLow and HysteresisHigh, for the EV to return to normal operation after a grid event.\r\n\r\n\r\n'))

    hysteresisGradient: Maybe[float] = Property(Number(description='Set default rate of change (ramp rate %/s) for the EV to return to normal operation after a grid event\r\n\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ReactivePowerParamsType(Object, additionalProperties=False):

    vRef: Maybe[float] = Property(Number(description='Only for VoltVar curve: The nominal ac voltage (rms) adjustment to the voltage curve points for Volt-Var curves (percentage).\r\n\r\n\r\n'))

    autonomousVRefEnable: Maybe[bool] = Property(Boolean(description='Only for VoltVar: Enable/disable autonomous VRef adjustment\r\n\r\n\r\n'))

    autonomousVRefTimeConstant: Maybe[float] = Property(Number(description='Only for VoltVar: Adjustment range for VRef time constant\r\n\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class VoltageParamsType(Object, additionalProperties=False):

    hv10MinMeanValue: Maybe[float] = Property(Number(description='EN 50549-1 chapter 4.9.3.4\r\n    Voltage threshold for the 10 min time window mean value monitoring.\r\n    The 10 min mean is recalculated up to every 3 s. \r\n    If the present voltage is above this threshold for more than the time defined by _hv10MinMeanValue_, the EV must trip.\r\n    This value is mandatory if _hv10MinMeanTripDelay_ is set.\r\n\r\n\r\n'))

    hv10MinMeanTripDelay: Maybe[float] = Property(Number(description='Time for which the voltage is allowed to stay above the 10 min mean value. \r\n    After this time, the EV must trip.\r\n    This value is mandatory if OverVoltageMeanValue10min is set.\r\n\r\n\r\n'))

    powerDuringCessation: Maybe[str] = Property(String(enum=['Active', 'Reactive'], description='Parameter is only sent, if the EV has to feed-in power or reactive power during fault-ride through (FRT) as defined by HVMomCess curve and LVMomCess curve.\r\n\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class DERCurveType(Object, additionalProperties=False):

    curveData: List[DERCurvePointsType] = Property(Array(DERCurvePointsType, additionalItems=False, minItems=1, maxItems=10), required=True)

    hysteresis: Maybe[HysteresisType] = Property(HysteresisType)

    priority: int = Property(Integer(minimum=0.0, description='Priority of curve (0=highest)\r\n\r\n\r\n'), required=True)

    reactivePowerParams: Maybe[ReactivePowerParamsType] = Property(ReactivePowerParamsType)

    voltageParams: Maybe[VoltageParamsType] = Property(VoltageParamsType)

    yUnit: str = Property(String(enum=['Not_Applicable', 'PctMaxW', 'PctMaxVar', 'PctWAvail', 'PctVarAvail', 'PctEffectiveV'], description='Unit of the Y-axis of DER curve\r\n'), required=True)

    responseTime: Maybe[float] = Property(Number(description='Open loop response time, the time to ramp up to 90% of the new target in response to the change in voltage, in seconds. A value of 0 is used to mean no limit. When not present, the device should follow its default behavior.\r\n\r\n\r\n'))

    startTime: Maybe[str] = Property(String(format='date-time', description='Point in time when this curve will become activated. Only absent when _default_ is true.\r\n\r\n'))

    duration: Maybe[float] = Property(Number(description='Duration in seconds that this curve will be active. Only absent when _default_ is true.\r\n\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class DERCurveGetType(Object, additionalProperties=False):

    curve: DERCurveType = Property(DERCurveType, required=True)

    id: str = Property(String(maxLength=36, description='Id of DER curve\r\n\r\n'), required=True)

    curveType: str = Property(String(enum=['EnterService', 'FreqDroop', 'FreqWatt', 'FixedPFAbsorb', 'FixedPFInject', 'FixedVar', 'Gradients', 'HFMustTrip', 'HFMayTrip', 'HVMustTrip', 'HVMomCess', 'HVMayTrip', 'LimitMaxDischarge', 'LFMustTrip', 'LVMustTrip', 'LVMomCess', 'LVMayTrip', 'PowerMonitoringMustTrip', 'VoltVar', 'VoltWatt', 'WattPF', 'WattVar'], description='Type of DER curve\r\n\r\n'), required=True)

    isDefault: bool = Property(Boolean(description='True if this is a default curve\r\n\r\n'), required=True)

    isSuperseded: bool = Property(Boolean(description='True if this setting is superseded by a higher priority setting (i.e. lower value of _priority_)\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class LimitMaxDischargeType(Object, additionalProperties=False):

    priority: int = Property(Integer(minimum=0.0, description='Priority of setting (0=highest)\r\n\r\n\r\n'), required=True)

    pctMaxDischargePower: Maybe[float] = Property(Number(description='Only for PowerMonitoring. +\r\n    The value specifies a percentage (0 to 100) of the rated maximum discharge power of EV. \r\n    The PowerMonitoring curve becomes active when power exceeds this percentage.\r\n\r\n\r\n'))

    powerMonitoringMustTrip: Maybe[DERCurveType] = Property(DERCurveType)

    startTime: Maybe[str] = Property(String(format='date-time', description='Time when this setting becomes active\r\n\r\n\r\n'))

    duration: Maybe[float] = Property(Number(description='Duration in seconds that this setting is active\r\n\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class LimitMaxDischargeGetType(Object, additionalProperties=False):

    id: str = Property(String(maxLength=36, description='Id of setting\r\n\r\n'), required=True)

    isDefault: bool = Property(Boolean(description='True if setting is a default control.\r\n'), required=True)

    isSuperseded: bool = Property(Boolean(description='True if this setting is superseded by a higher priority setting (i.e. lower value of _priority_)\r\n\r\n'), required=True)

    limitMaxDischarge: LimitMaxDischargeType = Property(LimitMaxDischargeType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ReportDERControlRequest(Object, additionalProperties=False):

    curve: Maybe[List[DERCurveGetType]] = Property(Array(DERCurveGetType, additionalItems=False, minItems=1, maxItems=24))

    enterService: Maybe[List[EnterServiceGetType]] = Property(Array(EnterServiceGetType, additionalItems=False, minItems=1, maxItems=24))

    fixedPFAbsorb: Maybe[List[FixedPFGetType]] = Property(Array(FixedPFGetType, additionalItems=False, minItems=1, maxItems=24))

    fixedPFInject: Maybe[List[FixedPFGetType]] = Property(Array(FixedPFGetType, additionalItems=False, minItems=1, maxItems=24))

    fixedVar: Maybe[List[FixedVarGetType]] = Property(Array(FixedVarGetType, additionalItems=False, minItems=1, maxItems=24))

    freqDroop: Maybe[List[FreqDroopGetType]] = Property(Array(FreqDroopGetType, additionalItems=False, minItems=1, maxItems=24))

    gradient: Maybe[List[GradientGetType]] = Property(Array(GradientGetType, additionalItems=False, minItems=1, maxItems=24))

    limitMaxDischarge: Maybe[List[LimitMaxDischargeGetType]] = Property(Array(LimitMaxDischargeGetType, additionalItems=False, minItems=1, maxItems=24))

    requestId: int = Property(Integer(description='RequestId from GetDERControlRequest.\r\n'), required=True)

    tbc: Maybe[bool] = Property(Boolean(description='To Be Continued. Default value when omitted: false. +\r\nFalse indicates that there are no further messages as part of this report.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
