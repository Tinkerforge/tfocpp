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


class StatusInfoType(Object, additionalProperties=False):
    """Element providing more information about the status.
"""

    reasonCode: str = Property(String(maxLength=20, description='A predefined code for the reason why the status is returned in this response. The string is case-insensitive.\r\n'), required=True)

    additionalInfo: Maybe[str] = Property(String(maxLength=1024, description='Additional text to provide detailed information.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class V2XFreqWattPointType(Object, additionalProperties=False):
    """*(2.1)* A point of a frequency-watt curve.
"""

    frequency: float = Property(Number(description='Net frequency in Hz.\r\n'), required=True)

    power: float = Property(Number(description='Power in W to charge (positive) or discharge (negative) at specified frequency.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class V2XSignalWattPointType(Object, additionalProperties=False):
    """*(2.1)* A point of a signal-watt curve.
"""

    signal: int = Property(Integer(description='Signal value from an AFRRSignalRequest.\r\n'), required=True)

    power: float = Property(Number(description='Power in W to charge (positive) or discharge (negative) at specified frequency.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ChargingSchedulePeriodType(Object, additionalProperties=False):
    """Charging schedule period structure defines a time period in a charging schedule. It is used in: CompositeScheduleType and in ChargingScheduleType. When used in a NotifyEVChargingScheduleRequest only _startPeriod_, _limit_, _limit_L2_, _limit_L3_ are relevant.
"""

    startPeriod: int = Property(Integer(description='Start of the period, in seconds from the start of schedule. The value of StartPeriod also defines the stop time of the previous period.\r\n'), required=True)

    limit: Maybe[float] = Property(Number(description='Optional only when not required by the _operationMode_, as in CentralSetpoint, ExternalSetpoint, ExternalLimits, LocalFrequency,  LocalLoadBalancing. +\r\nCharging rate limit during the schedule period, in the applicable _chargingRateUnit_. \r\nThis SHOULD be a non-negative value; a negative value is only supported for backwards compatibility with older systems that use a negative value to specify a discharging limit.\r\nWhen using _chargingRateUnit_ = `W`, this field represents the sum of the power of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\n'))

    limit_L2: Maybe[float] = Property(Number(description='*(2.1)* Charging rate limit on phase L2  in the applicable _chargingRateUnit_. \r\n'))

    limit_L3: Maybe[float] = Property(Number(description='*(2.1)* Charging rate limit on phase L3  in the applicable _chargingRateUnit_. \r\n'))

    numberPhases: Maybe[int] = Property(Integer(minimum=0.0, maximum=3.0, description='The number of phases that can be used for charging. +\r\nFor a DC EVSE this field should be omitted. +\r\nFor an AC EVSE a default value of _numberPhases_ = 3 will be assumed if the field is absent.\r\n'))

    phaseToUse: Maybe[int] = Property(Integer(minimum=0.0, maximum=3.0, description='Values: 1..3, Used if numberPhases=1 and if the EVSE is capable of switching the phase connected to the EV, i.e. ACPhaseSwitchingSupported is defined and true. It’s not allowed unless both conditions above are true. If both conditions are true, and phaseToUse is omitted, the Charging Station / EVSE will make the selection on its own.\r\n\r\n'))

    dischargeLimit: Maybe[float] = Property(Number(maximum=0.0, description='*(2.1)* Limit in _chargingRateUnit_ that the EV is allowed to discharge with. Note, these are negative values in order to be consistent with _setpoint_, which can be positive and negative.  +\r\nFor AC this field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\n'))

    dischargeLimit_L2: Maybe[float] = Property(Number(maximum=0.0, description='*(2.1)* Limit in _chargingRateUnit_ on phase L2 that the EV is allowed to discharge with. \r\n'))

    dischargeLimit_L3: Maybe[float] = Property(Number(maximum=0.0, description='*(2.1)* Limit in _chargingRateUnit_ on phase L3 that the EV is allowed to discharge with. \r\n'))

    setpoint: Maybe[float] = Property(Number(description='*(2.1)* Setpoint in _chargingRateUnit_ that the EV should follow as close as possible. Use negative values for discharging. +\r\nWhen a limit and/or _dischargeLimit_ are given the overshoot when following _setpoint_ must remain within these values.\r\nThis field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\n'))

    setpoint_L2: Maybe[float] = Property(Number(description='*(2.1)* Setpoint in _chargingRateUnit_ that the EV should follow on phase L2 as close as possible.\r\n'))

    setpoint_L3: Maybe[float] = Property(Number(description='*(2.1)* Setpoint in _chargingRateUnit_ that the EV should follow on phase L3 as close as possible. \r\n'))

    setpointReactive: Maybe[float] = Property(Number(description='*(2.1)* Setpoint for reactive power (or current) in _chargingRateUnit_ that the EV should follow as closely as possible. Positive values for inductive, negative for capacitive reactive power or current.  +\r\nThis field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\n'))

    setpointReactive_L2: Maybe[float] = Property(Number(description='*(2.1)* Setpoint for reactive power (or current) in _chargingRateUnit_ that the EV should follow on phase L2 as closely as possible. \r\n'))

    setpointReactive_L3: Maybe[float] = Property(Number(description='*(2.1)* Setpoint for reactive power (or current) in _chargingRateUnit_ that the EV should follow on phase L3 as closely as possible. \r\n'))

    preconditioningRequest: Maybe[bool] = Property(Boolean(description='*(2.1)* If  true, the EV should attempt to keep the BMS preconditioned for this time interval.\r\n'))

    evseSleep: Maybe[bool] = Property(Boolean(description='*(2.1)* If true, the EVSE must turn off power electronics/modules associated with this transaction. Default value when absent is false.\r\n'))

    v2xBaseline: Maybe[float] = Property(Number(description='*(2.1)* Power value that, when present, is used as a baseline on top of which values from _v2xFreqWattCurve_ and _v2xSignalWattCurve_ are added.\r\n\r\n'))

    operationMode: Maybe[str] = Property(String(enum=['Idle', 'ChargingOnly', 'CentralSetpoint', 'ExternalSetpoint', 'ExternalLimits', 'CentralFrequency', 'LocalFrequency', 'LocalLoadBalancing'], description='*(2.1)* Charging operation mode to use during this time interval. When absent defaults to `ChargingOnly`.\r\n'))

    v2xFreqWattCurve: Maybe[List[V2XFreqWattPointType]] = Property(Array(V2XFreqWattPointType, additionalItems=False, minItems=1, maxItems=20))

    v2xSignalWattCurve: Maybe[List[V2XSignalWattPointType]] = Property(Array(V2XSignalWattPointType, additionalItems=False, minItems=1, maxItems=20))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class CompositeScheduleType(Object, additionalProperties=False):

    evseId: int = Property(Integer(minimum=0.0), required=True)

    duration: int = Property(Integer(), required=True)

    scheduleStart: str = Property(String(format='date-time'), required=True)

    chargingRateUnit: str = Property(String(enum=['W', 'A']), required=True)

    chargingSchedulePeriod: List[ChargingSchedulePeriodType] = Property(Array(ChargingSchedulePeriodType, additionalItems=False, minItems=1), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class GetCompositeScheduleResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected'], description='The Charging Station will indicate if it was\r\nable to process the request\r\n'), required=True)

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    schedule: Maybe[CompositeScheduleType] = Property(CompositeScheduleType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
