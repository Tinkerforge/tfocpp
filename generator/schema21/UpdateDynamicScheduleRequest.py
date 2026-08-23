from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Number, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ChargingScheduleUpdateType(Object, additionalProperties=False):
    """Updates to a ChargingSchedulePeriodType for dynamic charging profiles.

"""

    limit: Maybe[float] = Property(Number(description='Optional only when not required by the _operationMode_, as in CentralSetpoint, ExternalSetpoint, ExternalLimits, LocalFrequency,  LocalLoadBalancing. +\r\nCharging rate limit during the schedule period, in the applicable _chargingRateUnit_. \r\nThis SHOULD be a non-negative value; a negative value is only supported for backwards compatibility with older systems that use a negative value to specify a discharging limit.\r\nFor AC this field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\n'))

    limit_L2: Maybe[float] = Property(Number(description='*(2.1)* Charging rate limit on phase L2  in the applicable _chargingRateUnit_. \r\n'))

    limit_L3: Maybe[float] = Property(Number(description='*(2.1)* Charging rate limit on phase L3  in the applicable _chargingRateUnit_. \r\n'))

    dischargeLimit: Maybe[float] = Property(Number(maximum=0.0, description='*(2.1)* Limit in _chargingRateUnit_ that the EV is allowed to discharge with. Note, these are negative values in order to be consistent with _setpoint_, which can be positive and negative.  +\r\nFor AC this field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\n'))

    dischargeLimit_L2: Maybe[float] = Property(Number(maximum=0.0, description='*(2.1)* Limit in _chargingRateUnit_ on phase L2 that the EV is allowed to discharge with. \r\n'))

    dischargeLimit_L3: Maybe[float] = Property(Number(maximum=0.0, description='*(2.1)* Limit in _chargingRateUnit_ on phase L3 that the EV is allowed to discharge with. \r\n'))

    setpoint: Maybe[float] = Property(Number(description='*(2.1)* Setpoint in _chargingRateUnit_ that the EV should follow as close as possible. Use negative values for discharging. +\r\nWhen a limit and/or _dischargeLimit_ are given the overshoot when following _setpoint_ must remain within these values.\r\nThis field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\n'))

    setpoint_L2: Maybe[float] = Property(Number(description='*(2.1)* Setpoint in _chargingRateUnit_ that the EV should follow on phase L2 as close as possible.\r\n'))

    setpoint_L3: Maybe[float] = Property(Number(description='*(2.1)* Setpoint in _chargingRateUnit_ that the EV should follow on phase L3 as close as possible. \r\n'))

    setpointReactive: Maybe[float] = Property(Number(description='*(2.1)* Setpoint for reactive power (or current) in _chargingRateUnit_ that the EV should follow as closely as possible. Positive values for inductive, negative for capacitive reactive power or current.  +\r\nThis field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\n'))

    setpointReactive_L2: Maybe[float] = Property(Number(description='*(2.1)* Setpoint for reactive power (or current) in _chargingRateUnit_ that the EV should follow on phase L2 as closely as possible. \r\n'))

    setpointReactive_L3: Maybe[float] = Property(Number(description='*(2.1)* Setpoint for reactive power (or current) in _chargingRateUnit_ that the EV should follow on phase L3 as closely as possible. \r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class UpdateDynamicScheduleRequest(Object, additionalProperties=False):
    """Id of dynamic charging profile to update.
"""

    chargingProfileId: int = Property(Integer(description='Id of charging profile to update.\r\n'), required=True)

    scheduleUpdate: ChargingScheduleUpdateType = Property(ChargingScheduleUpdateType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
