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


class CostType(Object, additionalProperties=False):

    costKind: str = Property(String(enum=['CarbonDioxideEmission', 'RelativePricePercentage', 'RenewableGenerationPercentage'], description='The kind of cost referred to in the message element amount\r\n'), required=True)

    amount: int = Property(Integer(description='The estimated or actual cost per kWh\r\n'), required=True)

    amountMultiplier: Maybe[int] = Property(Integer(description='Values: -3..3, The amountMultiplier defines the exponent to base 10 (dec). The final value is determined by: amount * 10 ^ amountMultiplier\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ConsumptionCostType(Object, additionalProperties=False):

    startValue: float = Property(Number(description='The lowest level of consumption that defines the starting point of this consumption block. The block interval extends to the start of the next interval.\r\n'), required=True)

    cost: List[CostType] = Property(Array(CostType, additionalItems=False, minItems=1, maxItems=3), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class LimitAtSoCType(Object, additionalProperties=False):

    soc: int = Property(Integer(minimum=0.0, maximum=100.0, description='The SoC value beyond which the charging rate limit should be applied.\r\n'), required=True)

    limit: float = Property(Number(description='Charging rate limit beyond the SoC value.\r\nThe unit is defined by _chargingSchedule.chargingRateUnit_.\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class PriceLevelScheduleEntryType(Object, additionalProperties=False):
    """Part of ISO 15118-20 price schedule.
"""

    duration: int = Property(Integer(description='The amount of seconds that define the duration of this given PriceLevelScheduleEntry.\r\n'), required=True)

    priceLevel: int = Property(Integer(minimum=0.0, description='Defines the price level of this PriceLevelScheduleEntry (referring to NumberOfPriceLevels). Small values for the PriceLevel represent a cheaper PriceLevelScheduleEntry. Large values for the PriceLevel represent a more expensive PriceLevelScheduleEntry.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class PriceLevelScheduleType(Object, additionalProperties=False):
    """The PriceLevelScheduleType is modeled after the same type that is defined in ISO 15118-20, such that if it is supplied by an EMSP as a signed EXI message, the conversion from EXI to JSON (in OCPP) and back to EXI (for ISO 15118-20) does not change the digest and therefore does not invalidate the signature.
"""

    priceLevelScheduleEntries: List[PriceLevelScheduleEntryType] = Property(Array(PriceLevelScheduleEntryType, additionalItems=False, minItems=1, maxItems=100), required=True)

    timeAnchor: str = Property(String(format='date-time', description='Starting point of this price schedule.\r\n'), required=True)

    priceScheduleId: int = Property(Integer(minimum=0.0, description='Unique ID of this price schedule.\r\n'), required=True)

    priceScheduleDescription: Maybe[str] = Property(String(maxLength=32, description='Description of the price schedule.\r\n'))

    numberOfPriceLevels: int = Property(Integer(minimum=0.0, description='Defines the overall number of distinct price level elements used across all PriceLevelSchedules.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class RationalNumberType(Object, additionalProperties=False):
    """Part of ISO 15118-20 price schedule.

"""

    exponent: int = Property(Integer(description='The exponent to base 10 (dec)\r\n'), required=True)

    value: int = Property(Integer(description='Value which shall be multiplied.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class AdditionalSelectedServicesType(Object, additionalProperties=False):
    """Part of ISO 15118-20 price schedule.

"""

    serviceFee: RationalNumberType = Property(RationalNumberType, required=True)

    serviceName: str = Property(String(maxLength=80, description='Human readable string to identify this service.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class OverstayRuleType(Object, additionalProperties=False):
    """Part of ISO 15118-20 price schedule.

"""

    overstayFee: RationalNumberType = Property(RationalNumberType, required=True)

    overstayRuleDescription: Maybe[str] = Property(String(maxLength=32, description='Human readable string to identify the overstay rule.\r\n'))

    startTime: int = Property(Integer(description='Time in seconds after trigger of the parent Overstay Rules for this particular fee to apply.\r\n'), required=True)

    overstayFeePeriod: int = Property(Integer(description='Time till overstay will be reapplied\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class OverstayRuleListType(Object, additionalProperties=False):
    """Part of ISO 15118-20 price schedule.

"""

    overstayPowerThreshold: Maybe[RationalNumberType] = Property(RationalNumberType)

    overstayRule: List[OverstayRuleType] = Property(Array(OverstayRuleType, additionalItems=False, minItems=1, maxItems=5), required=True)

    overstayTimeThreshold: Maybe[int] = Property(Integer(description='Time till overstay is applied in seconds.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class PriceRuleType(Object, additionalProperties=False):
    """Part of ISO 15118-20 price schedule.

"""

    parkingFeePeriod: Maybe[int] = Property(Integer(description='The duration of the parking fee period (in seconds).\r\nWhen the time enters into a ParkingFeePeriod, the ParkingFee will apply to the session. .\r\n'))

    carbonDioxideEmission: Maybe[int] = Property(Integer(minimum=0.0, description='Number of grams of CO2 per kWh.\r\n'))

    renewableGenerationPercentage: Maybe[int] = Property(Integer(minimum=0.0, maximum=100.0, description='Percentage of the power that is created by renewable resources.\r\n'))

    energyFee: RationalNumberType = Property(RationalNumberType, required=True)

    parkingFee: Maybe[RationalNumberType] = Property(RationalNumberType)

    powerRangeStart: RationalNumberType = Property(RationalNumberType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class PriceRuleStackType(Object, additionalProperties=False):
    """Part of ISO 15118-20 price schedule.
"""

    duration: int = Property(Integer(description='Duration of the stack of price rules.  he amount of seconds that define the duration of the given PriceRule(s).\r\n'), required=True)

    priceRule: List[PriceRuleType] = Property(Array(PriceRuleType, additionalItems=False, minItems=1, maxItems=8), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class RelativeTimeIntervalType(Object, additionalProperties=False):

    start: int = Property(Integer(description='Start of the interval, in seconds from NOW.\r\n'), required=True)

    duration: Maybe[int] = Property(Integer(description='Duration of the interval, in seconds.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SalesTariffEntryType(Object, additionalProperties=False):

    relativeTimeInterval: RelativeTimeIntervalType = Property(RelativeTimeIntervalType, required=True)

    ePriceLevel: Maybe[int] = Property(Integer(minimum=0.0, description='Defines the price level of this SalesTariffEntry (referring to NumEPriceLevels). Small values for the EPriceLevel represent a cheaper TariffEntry. Large values for the EPriceLevel represent a more expensive TariffEntry.\r\n'))

    consumptionCost: Maybe[List[ConsumptionCostType]] = Property(Array(ConsumptionCostType, additionalItems=False, minItems=1, maxItems=3))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SalesTariffType(Object, additionalProperties=False):
    """A SalesTariff provided by a Mobility Operator (EMSP) .
NOTE: This dataType is based on dataTypes from &lt;&lt;ref-ISOIEC15118-2,ISO 15118-2&gt;&gt;.
"""

    id: int = Property(Integer(minimum=0.0, description='SalesTariff identifier used to identify one sales tariff. An SAID remains a unique identifier for one schedule throughout a charging session.\r\n'), required=True)

    salesTariffDescription: Maybe[str] = Property(String(maxLength=32, description='A human readable title/short description of the sales tariff e.g. for HMI display purposes.\r\n'))

    numEPriceLevels: Maybe[int] = Property(Integer(minimum=0.0, description='Defines the overall number of distinct price levels used across all provided SalesTariff elements.\r\n'))

    salesTariffEntry: List[SalesTariffEntryType] = Property(Array(SalesTariffEntryType, additionalItems=False, minItems=1, maxItems=1024), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TaxRuleType(Object, additionalProperties=False):
    """Part of ISO 15118-20 price schedule.

"""

    taxRuleID: int = Property(Integer(minimum=0.0, description='Id for the tax rule.\r\n'), required=True)

    taxRuleName: Maybe[str] = Property(String(maxLength=100, description='Human readable string to identify the tax rule.\r\n'))

    taxIncludedInPrice: Maybe[bool] = Property(Boolean(description='Indicates whether the tax is included in any price or not.\r\n'))

    appliesToEnergyFee: bool = Property(Boolean(description='Indicates whether this tax applies to Energy Fees.\r\n'), required=True)

    appliesToParkingFee: bool = Property(Boolean(description='Indicates whether this tax applies to Parking Fees.\r\n\r\n'), required=True)

    appliesToOverstayFee: bool = Property(Boolean(description='Indicates whether this tax applies to Overstay Fees.\r\n\r\n'), required=True)

    appliesToMinimumMaximumCost: bool = Property(Boolean(description='Indicates whether this tax applies to Minimum/Maximum Cost.\r\n\r\n'), required=True)

    taxRate: RationalNumberType = Property(RationalNumberType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class AbsolutePriceScheduleType(Object, additionalProperties=False):
    """The AbsolutePriceScheduleType is modeled after the same type that is defined in ISO 15118-20, such that if it is supplied by an EMSP as a signed EXI message, the conversion from EXI to JSON (in OCPP) and back to EXI (for ISO 15118-20) does not change the digest and therefore does not invalidate the signature.

image::images/AbsolutePriceSchedule-Simple.png[]

"""

    timeAnchor: str = Property(String(format='date-time', description='Starting point of price schedule.\r\n'), required=True)

    priceScheduleID: int = Property(Integer(minimum=0.0, description='Unique ID of price schedule\r\n'), required=True)

    priceScheduleDescription: Maybe[str] = Property(String(maxLength=160, description='Description of the price schedule.\r\n'))

    currency: str = Property(String(maxLength=3, description='Currency according to ISO 4217.\r\n'), required=True)

    language: str = Property(String(maxLength=8, description='String that indicates what language is used for the human readable strings in the price schedule. Based on ISO 639.\r\n'), required=True)

    priceAlgorithm: str = Property(String(maxLength=2000, description='A string in URN notation which shall uniquely identify an algorithm that defines how to compute an energy fee sum for a specific power profile based on the EnergyFee information from the PriceRule elements.\r\n'), required=True)

    minimumCost: Maybe[RationalNumberType] = Property(RationalNumberType)

    maximumCost: Maybe[RationalNumberType] = Property(RationalNumberType)

    priceRuleStacks: List[PriceRuleStackType] = Property(Array(PriceRuleStackType, additionalItems=False, minItems=1, maxItems=1024), required=True)

    taxRules: Maybe[List[TaxRuleType]] = Property(Array(TaxRuleType, additionalItems=False, minItems=1, maxItems=10))

    overstayRuleList: Maybe[OverstayRuleListType] = Property(OverstayRuleListType)

    additionalSelectedServices: Maybe[List[AdditionalSelectedServicesType]] = Property(Array(AdditionalSelectedServicesType, additionalItems=False, minItems=1, maxItems=5))

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


class ChargingScheduleType(Object, additionalProperties=False):
    """Charging schedule structure defines a list of charging periods, as used in: NotifyEVChargingScheduleRequest and ChargingProfileType. When used in a NotifyEVChargingScheduleRequest only _duration_ and _chargingSchedulePeriod_ are relevant and _chargingRateUnit_ must be 'W'. +
An ISO 15118-20 session may provide either an _absolutePriceSchedule_ or a _priceLevelSchedule_. An ISO 15118-2 session can only provide a_salesTariff_ element. The field _digestValue_ is used when price schedule or sales tariff are signed.

image::images/ChargingSchedule-Simple.png[]


"""

    id: int = Property(Integer(), required=True)

    limitAtSoC: Maybe[LimitAtSoCType] = Property(LimitAtSoCType)

    startSchedule: Maybe[str] = Property(String(format='date-time', description='Starting point of an absolute schedule or recurring schedule.\r\n'))

    duration: Maybe[int] = Property(Integer(description='Duration of the charging schedule in seconds. If the duration is left empty, the last period will continue indefinitely or until end of the transaction in case startSchedule is absent.\r\n'))

    chargingRateUnit: str = Property(String(enum=['W', 'A'], description='The unit of measure in which limits and setpoints are expressed.\r\n'), required=True)

    minChargingRate: Maybe[float] = Property(Number(description='Minimum charging rate supported by the EV. The unit of measure is defined by the chargingRateUnit. This parameter is intended to be used by a local smart charging algorithm to optimize the power allocation for in the case a charging process is inefficient at lower charging rates. \r\n'))

    powerTolerance: Maybe[float] = Property(Number(description='*(2.1)* Power tolerance when following EVPowerProfile.\r\n\r\n'))

    signatureId: Maybe[int] = Property(Integer(minimum=0.0, description='*(2.1)* Id of this element for referencing in a signature.\r\n'))

    digestValue: Maybe[str] = Property(String(maxLength=88, description='*(2.1)* Base64 encoded hash (SHA256 for ISO 15118-2, SHA512 for ISO 15118-20) of the EXI price schedule element. Used in signature.\r\n'))

    useLocalTime: Maybe[bool] = Property(Boolean(description='*(2.1)* Defaults to false. When true, disregard time zone offset in dateTime fields of  _ChargingScheduleType_ and use unqualified local time at Charging Station instead.\r\n This allows the same `Absolute` or `Recurring` charging profile to be used in both summer and winter time.\r\n\r\n'))

    chargingSchedulePeriod: List[ChargingSchedulePeriodType] = Property(Array(ChargingSchedulePeriodType, additionalItems=False, minItems=1, maxItems=1024), required=True)

    randomizedDelay: Maybe[int] = Property(Integer(minimum=0.0, description='*(2.1)* Defaults to 0. When _randomizedDelay_ not equals zero, then the start of each &lt;&lt;cmn_chargingscheduleperiodtype,ChargingSchedulePeriodType&gt;&gt; is delayed by a randomly chosen number of seconds between 0 and _randomizedDelay_.  Only allowed for TxProfile and TxDefaultProfile.\r\n\r\n'))

    salesTariff: Maybe[SalesTariffType] = Property(SalesTariffType)

    absolutePriceSchedule: Maybe[AbsolutePriceScheduleType] = Property(AbsolutePriceScheduleType)

    priceLevelSchedule: Maybe[PriceLevelScheduleType] = Property(PriceLevelScheduleType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ChargingProfileType(Object, additionalProperties=False):
    """A ChargingProfile consists of 1 to 3 ChargingSchedules with a list of ChargingSchedulePeriods, describing the amount of power or current that can be delivered per time interval.

image::images/ChargingProfile-Simple.png[]

"""

    id: int = Property(Integer(description='Id of ChargingProfile. Unique within charging station. Id can have a negative value. This is useful to distinguish charging profiles from an external actor (external constraints) from charging profiles received from CSMS.\r\n'), required=True)

    stackLevel: int = Property(Integer(minimum=0.0, description='Value determining level in hierarchy stack of profiles. Higher values have precedence over lower values. Lowest level is 0.\r\n'), required=True)

    chargingProfilePurpose: str = Property(String(enum=['ChargingStationExternalConstraints', 'ChargingStationMaxProfile', 'TxDefaultProfile', 'TxProfile', 'PriorityCharging', 'LocalGeneration'], description='Defines the purpose of the schedule transferred by this profile\r\n'), required=True)

    chargingProfileKind: str = Property(String(enum=['Absolute', 'Recurring', 'Relative', 'Dynamic'], description='Indicates the kind of schedule.\r\n'), required=True)

    recurrencyKind: Maybe[str] = Property(String(enum=['Daily', 'Weekly'], description='Indicates the start point of a recurrence.\r\n'))

    validFrom: Maybe[str] = Property(String(format='date-time', description='Point in time at which the profile starts to be valid. If absent, the profile is valid as soon as it is received by the Charging Station.\r\n'))

    validTo: Maybe[str] = Property(String(format='date-time', description='Point in time at which the profile stops to be valid. If absent, the profile is valid until it is replaced by another profile.\r\n'))

    transactionId: Maybe[str] = Property(String(maxLength=36, description='SHALL only be included if ChargingProfilePurpose is set to TxProfile in a SetChargingProfileRequest. The transactionId is used to match the profile to a specific transaction.\r\n'))

    maxOfflineDuration: Maybe[int] = Property(Integer(description='*(2.1)* Period in seconds that this charging profile remains valid after the Charging Station has gone offline. After this period the charging profile becomes invalid for as long as it is offline and the Charging Station reverts back to a valid profile with a lower stack level. \r\nIf _invalidAfterOfflineDuration_ is true, then this charging profile will become permanently invalid.\r\nA value of 0 means that the charging profile is immediately invalid while offline. When the field is absent, then  no timeout applies and the charging profile remains valid when offline.\r\n'))

    chargingSchedule: List[ChargingScheduleType] = Property(Array(ChargingScheduleType, additionalItems=False, minItems=1, maxItems=3), required=True)

    invalidAfterOfflineDuration: Maybe[bool] = Property(Boolean(description='*(2.1)* When set to true this charging profile will not be valid anymore after being offline for more than _maxOfflineDuration_. +\r\n    When absent defaults to false.\r\n'))

    dynUpdateInterval: Maybe[int] = Property(Integer(description='*(2.1)*  Interval in seconds after receipt of last update, when to request a profile update by sending a PullDynamicScheduleUpdateRequest message.\r\n    A value of 0 or no value means that no update interval applies. +\r\n    Only relevant in a dynamic charging profile.\r\n\r\n'))

    dynUpdateTime: Maybe[str] = Property(String(format='date-time', description='*(2.1)* Time at which limits or setpoints in this charging profile were last updated by a PullDynamicScheduleUpdateRequest or UpdateDynamicScheduleRequest or by an external actor. +\r\n    Only relevant in a dynamic charging profile.\r\n\r\n'))

    priceScheduleSignature: Maybe[str] = Property(String(maxLength=256, description='*(2.1)* ISO 15118-20 signature for all price schedules in _chargingSchedules_. +\r\nNote: for 256-bit elliptic curves (like secp256k1) the ECDSA signature is 512 bits (64 bytes) and for 521-bit curves (like secp521r1) the signature is 1042 bits. This equals 131 bytes, which can be encoded as base64 in 176 bytes.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SetChargingProfileRequest(Object, additionalProperties=False):

    evseId: int = Property(Integer(minimum=0.0, description='For TxDefaultProfile an evseId=0 applies the profile to each individual evse. For ChargingStationMaxProfile and ChargingStationExternalConstraints an evseId=0 contains an overal limit for the whole Charging Station.\r\n'), required=True)

    chargingProfile: ChargingProfileType = Property(ChargingProfileType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
