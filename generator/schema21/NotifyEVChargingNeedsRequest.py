from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Number, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ACChargingParametersType(Object, additionalProperties=False):
    """EV AC charging parameters for ISO 15118-2

"""

    energyAmount: float = Property(Number(description='Amount of energy requested (in Wh). This includes energy required for preconditioning.\r\nRelates to: +\r\n*ISO 15118-2*: AC_EVChargeParameterType: EAmount +\r\n*ISO 15118-20*: Dynamic/Scheduled_SEReqControlModeType: EVTargetEnergyRequest\r\n\r\n'), required=True)

    evMinCurrent: float = Property(Number(description='Minimum current (amps) supported by the electric vehicle (per phase).\r\nRelates to: +\r\n*ISO 15118-2*: AC_EVChargeParameterType: EVMinCurrent\r\n\r\n'), required=True)

    evMaxCurrent: float = Property(Number(description='Maximum current (amps) supported by the electric vehicle (per phase). Includes cable capacity.\r\nRelates to: +\r\n*ISO 15118-2*: AC_EVChargeParameterType: EVMaxCurrent\r\n\r\n'), required=True)

    evMaxVoltage: float = Property(Number(description='Maximum voltage supported by the electric vehicle.\r\nRelates to: +\r\n*ISO 15118-2*: AC_EVChargeParameterType: EVMaxVoltage\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class DCChargingParametersType(Object, additionalProperties=False):
    """EV DC charging parameters for ISO 15118-2
"""

    evMaxCurrent: float = Property(Number(description='Maximum current (in A) supported by the electric vehicle. Includes cable capacity.\r\nRelates to: +\r\n*ISO 15118-2*: DC_EVChargeParameterType:EVMaximumCurrentLimit\r\n'), required=True)

    evMaxVoltage: float = Property(Number(description='Maximum voltage supported by the electric vehicle.\r\nRelates to: +\r\n*ISO 15118-2*: DC_EVChargeParameterType: EVMaximumVoltageLimit\r\n\r\n'), required=True)

    evMaxPower: Maybe[float] = Property(Number(description='Maximum power (in W) supported by the electric vehicle. Required for DC charging.\r\nRelates to: +\r\n*ISO 15118-2*: DC_EVChargeParameterType: EVMaximumPowerLimit\r\n\r\n'))

    evEnergyCapacity: Maybe[float] = Property(Number(description='Capacity of the electric vehicle battery (in Wh).\r\nRelates to: +\r\n*ISO 15118-2*: DC_EVChargeParameterType: EVEnergyCapacity\r\n\r\n'))

    energyAmount: Maybe[float] = Property(Number(description='Amount of energy requested (in Wh). This inludes energy required for preconditioning.\r\nRelates to: +\r\n*ISO 15118-2*: DC_EVChargeParameterType: EVEnergyRequest\r\n\r\n\r\n'))

    stateOfCharge: Maybe[int] = Property(Integer(minimum=0.0, maximum=100.0, description='Energy available in the battery (in percent of the battery capacity)\r\nRelates to: +\r\n*ISO 15118-2*: DC_EVChargeParameterType: DC_EVStatus: EVRESSSOC\r\n\r\n'))

    fullSoC: Maybe[int] = Property(Integer(minimum=0.0, maximum=100.0, description='Percentage of SoC at which the EV considers the battery fully charged. (possible values: 0 - 100)\r\nRelates to: +\r\n*ISO 15118-2*: DC_EVChargeParameterType: FullSOC\r\n\r\n'))

    bulkSoC: Maybe[int] = Property(Integer(minimum=0.0, maximum=100.0, description='Percentage of SoC at which the EV considers a fast charging process to end. (possible values: 0 - 100)\r\nRelates to: +\r\n*ISO 15118-2*: DC_EVChargeParameterType: BulkSOC\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class DERChargingParametersType(Object, additionalProperties=False):
    """*(2.1)* DERChargingParametersType is used in ChargingNeedsType during an ISO 15118-20 session for AC_BPT_DER to report the inverter settings related to DER control that were agreed between EVSE and EV.

Fields starting with "ev" contain values from the EV.
Other fields contain a value that is supported by both EV and EVSE.

DERChargingParametersType type is only relevant in case of an ISO 15118-20 AC_BPT_DER/AC_DER charging session.

NOTE: All these fields have values greater or equal to zero (i.e. are non-negative)

"""

    evSupportedDERControl: Maybe[List[str]] = Property(Array(String(enum=['EnterService', 'FreqDroop', 'FreqWatt', 'FixedPFAbsorb', 'FixedPFInject', 'FixedVar', 'Gradients', 'HFMustTrip', 'HFMayTrip', 'HVMustTrip', 'HVMomCess', 'HVMayTrip', 'LimitMaxDischarge', 'LFMustTrip', 'LVMustTrip', 'LVMomCess', 'LVMayTrip', 'PowerMonitoringMustTrip', 'VoltVar', 'VoltWatt', 'WattPF', 'WattVar']), additionalItems=False, minItems=1, description='DER control functions supported by EV. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType:DERControlFunctions (bitmap)\r\n'))

    evOverExcitedMaxDischargePower: Maybe[float] = Property(Number(description='Rated maximum injected active power by EV, at specified over-excited power factor (overExcitedPowerFactor). +\r\nIt can also be defined as the rated maximum discharge power at the rated minimum injected reactive power value. This means that if the EV is providing reactive power support, and it is requested to discharge at max power (e.g. to satisfy an EMS request), the EV may override the request and discharge up to overExcitedMaximumDischargePower to meet the minimum reactive power requirements. +\r\nCorresponds to the WOvPF attribute in IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVOverExcitedMaximumDischargePower\r\n'))

    evOverExcitedPowerFactor: Maybe[float] = Property(Number(description='EV power factor when injecting (over excited) the minimum reactive power. +\r\nCorresponds to the OvPF attribute in IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVOverExcitedPowerFactor\r\n'))

    evUnderExcitedMaxDischargePower: Maybe[float] = Property(Number(description='Rated maximum injected active power by EV supported at specified under-excited power factor (EVUnderExcitedPowerFactor). +\r\nIt can also be defined as the rated maximum dischargePower at the rated minimum absorbed reactive power value.\r\nThis means that if the EV is providing reactive power support, and it is requested to discharge at max power (e.g. to satisfy an EMS request), the EV may override the request and discharge up to underExcitedMaximumDischargePower to meet the minimum reactive power requirements. +\r\nThis corresponds to the WUnPF attribute in the IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVUnderExcitedMaximumDischargePower\r\n'))

    evUnderExcitedPowerFactor: Maybe[float] = Property(Number(description='EV power factor when injecting (under excited) the minimum reactive power. +\r\nCorresponds to the OvPF attribute in IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVUnderExcitedPowerFactor\r\n'))

    maxApparentPower: Maybe[float] = Property(Number(description='Rated maximum total apparent power, defined by min(EV, EVSE) in va.\r\nCorresponds to the VAMaxRtg in IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumApparentPower\r\n'))

    maxChargeApparentPower: Maybe[float] = Property(Number(description='Rated maximum absorbed apparent power, defined by min(EV, EVSE) in va. +\r\n    This field represents the sum of all phases, unless values are provided for L2 and L3,\r\n    in which case this field represents phase L1. +\r\n    Corresponds to the ChaVAMaxRtg in IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumChargeApparentPower\r\n'))

    maxChargeApparentPower_L2: Maybe[float] = Property(Number(description='Rated maximum absorbed apparent power on phase L2, defined by min(EV, EVSE) in va.\r\nCorresponds to the ChaVAMaxRtg in IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumChargeApparentPower_L2\r\n'))

    maxChargeApparentPower_L3: Maybe[float] = Property(Number(description='Rated maximum absorbed apparent power on phase L3, defined by min(EV, EVSE) in va.\r\nCorresponds to the ChaVAMaxRtg in IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumChargeApparentPower_L3\r\n'))

    maxDischargeApparentPower: Maybe[float] = Property(Number(description='Rated maximum injected apparent power, defined by min(EV, EVSE) in va. +\r\n    This field represents the sum of all phases, unless values are provided for L2 and L3,\r\n    in which case this field represents phase L1. +\r\n    Corresponds to the DisVAMaxRtg in IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumDischargeApparentPower\r\n'))

    maxDischargeApparentPower_L2: Maybe[float] = Property(Number(description='Rated maximum injected apparent power on phase L2, defined by min(EV, EVSE) in va. +\r\n    Corresponds to the DisVAMaxRtg in IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumDischargeApparentPower_L2\r\n'))

    maxDischargeApparentPower_L3: Maybe[float] = Property(Number(description='Rated maximum injected apparent power on phase L3, defined by min(EV, EVSE) in va. +\r\n    Corresponds to the DisVAMaxRtg in IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumDischargeApparentPower_L3\r\n'))

    maxChargeReactivePower: Maybe[float] = Property(Number(description='Rated maximum absorbed reactive power, defined by min(EV, EVSE), in vars. +\r\n    This field represents the sum of all phases, unless values are provided for L2 and L3,\r\n    in which case this field represents phase L1. +\r\nCorresponds to the AvarMax attribute in the IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumChargeReactivePower\r\n'))

    maxChargeReactivePower_L2: Maybe[float] = Property(Number(description='Rated maximum absorbed reactive power, defined by min(EV, EVSE), in vars on phase L2. +\r\nCorresponds to the AvarMax attribute in the IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumChargeReactivePower_L2\r\n'))

    maxChargeReactivePower_L3: Maybe[float] = Property(Number(description='Rated maximum absorbed reactive power, defined by min(EV, EVSE), in vars on phase L3. +\r\nCorresponds to the AvarMax attribute in the IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumChargeReactivePower_L3\r\n'))

    minChargeReactivePower: Maybe[float] = Property(Number(description='Rated minimum absorbed reactive power, defined by max(EV, EVSE), in vars. +\r\n    This field represents the sum of all phases, unless values are provided for L2 and L3,\r\n    in which case this field represents phase L1. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMinimumChargeReactivePower\r\n'))

    minChargeReactivePower_L2: Maybe[float] = Property(Number(description='Rated minimum absorbed reactive power, defined by max(EV, EVSE), in vars on phase L2. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMinimumChargeReactivePower_L2\r\n'))

    minChargeReactivePower_L3: Maybe[float] = Property(Number(description='Rated minimum absorbed reactive power, defined by max(EV, EVSE), in vars on phase L3. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMinimumChargeReactivePower_L3\r\n'))

    maxDischargeReactivePower: Maybe[float] = Property(Number(description='Rated maximum injected reactive power, defined by min(EV, EVSE), in vars. +\r\n    This field represents the sum of all phases, unless values are provided for L2 and L3,\r\n    in which case this field represents phase L1. +\r\nCorresponds to the IvarMax attribute in the IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumDischargeReactivePower\r\n'))

    maxDischargeReactivePower_L2: Maybe[float] = Property(Number(description='Rated maximum injected reactive power, defined by min(EV, EVSE), in vars on phase L2. +\r\nCorresponds to the IvarMax attribute in the IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumDischargeReactivePower_L2\r\n'))

    maxDischargeReactivePower_L3: Maybe[float] = Property(Number(description='Rated maximum injected reactive power, defined by min(EV, EVSE), in vars on phase L3. +\r\nCorresponds to the IvarMax attribute in the IEC 61850. +\r\n    *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumDischargeReactivePower_L3\r\n'))

    minDischargeReactivePower: Maybe[float] = Property(Number(description='Rated minimum injected reactive power, defined by max(EV, EVSE), in vars. +\r\n    This field represents the sum of all phases, unless values are provided for L2 and L3,\r\n    in which case this field represents phase L1. +\r\n        *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMinimumDischargeReactivePower\r\n'))

    minDischargeReactivePower_L2: Maybe[float] = Property(Number(description='Rated minimum injected reactive power, defined by max(EV, EVSE), in var on phase L2. +\r\n        *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMinimumDischargeReactivePower_L2\r\n'))

    minDischargeReactivePower_L3: Maybe[float] = Property(Number(description='Rated minimum injected reactive power, defined by max(EV, EVSE), in var on phase L3. +\r\n        *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMinimumDischargeReactivePower_L3\r\n'))

    nominalVoltage: Maybe[float] = Property(Number(description='Line voltage supported by EVSE and EV.\r\n        *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVNominalVoltage\r\n'))

    nominalVoltageOffset: Maybe[float] = Property(Number(description="The nominal AC voltage (rms) offset between the Charging Station's electrical connection point and the utility’s point of common coupling. +\r\n        *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVNominalVoltageOffset\r\n"))

    maxNominalVoltage: Maybe[float] = Property(Number(description='Maximum AC rms voltage, as defined by min(EV, EVSE)  to operate with. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumNominalVoltage\r\n'))

    minNominalVoltage: Maybe[float] = Property(Number(description='Minimum AC rms voltage, as defined by max(EV, EVSE)  to operate with. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMinimumNominalVoltage\r\n'))

    evInverterManufacturer: Maybe[str] = Property(String(maxLength=50, description='Manufacturer of the EV inverter. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVInverterManufacturer\r\n'))

    evInverterModel: Maybe[str] = Property(String(maxLength=50, description='Model name of the EV inverter. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVInverterModel\r\n'))

    evInverterSerialNumber: Maybe[str] = Property(String(maxLength=50, description='Serial number of the EV inverter. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVInverterSerialNumber\r\n'))

    evInverterSwVersion: Maybe[str] = Property(String(maxLength=50, description='Software version of EV inverter. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVInverterSwVersion\r\n'))

    evInverterHwVersion: Maybe[str] = Property(String(maxLength=50, description='Hardware version of EV inverter. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVInverterHwVersion\r\n'))

    evIslandingDetectionMethod: Maybe[List[str]] = Property(Array(String(enum=['NoAntiIslandingSupport', 'RoCoF', 'UVP_OVP', 'UFP_OFP', 'VoltageVectorShift', 'ZeroCrossingDetection', 'OtherPassive', 'ImpedanceMeasurement', 'ImpedanceAtFrequency', 'SlipModeFrequencyShift', 'SandiaFrequencyShift', 'SandiaVoltageShift', 'FrequencyJump', 'RCLQFactor', 'OtherActive']), additionalItems=False, minItems=1, description='Type of islanding detection method. Only mandatory when islanding detection is required at the site, as set in the ISO 15118 Service Details configuration. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVIslandingDetectionMethod\r\n'))

    evIslandingTripTime: Maybe[float] = Property(Number(description='Time after which EV will trip if an island has been detected. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVIslandingTripTime\r\n'))

    evMaximumLevel1DCInjection: Maybe[float] = Property(Number(description='Maximum injected DC current allowed at level 1 charging. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumLevel1DCInjection\r\n'))

    evDurationLevel1DCInjection: Maybe[float] = Property(Number(description='Maximum allowed duration of DC injection at level 1 charging. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVDurationLevel1DCInjection\r\n'))

    evMaximumLevel2DCInjection: Maybe[float] = Property(Number(description='Maximum injected DC current allowed at level 2 charging. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVMaximumLevel2DCInjection\r\n'))

    evDurationLevel2DCInjection: Maybe[float] = Property(Number(description='Maximum allowed duration of DC injection at level 2 charging. +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVDurationLevel2DCInjection\r\n'))

    evReactiveSusceptance: Maybe[float] = Property(Number(description='\tMeasure of the susceptibility of the circuit to reactance, in Siemens (S). +\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVReactiveSusceptance\r\n\r\n\r\n'))

    evSessionTotalDischargeEnergyAvailable: Maybe[float] = Property(Number(description='Total energy value, in Wh, that EV is allowed to provide during the entire V2G session. The value is independent of the V2X Cycling area. Once this value reaches the value of 0, the EV may block any attempt to discharge in order to protect the battery health.\r\n       *ISO 15118-20*: DER_BPT_AC_CPDReqEnergyTransferModeType: EVSessionTotalDischargeEnergyAvailable\r\n\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class EVPowerScheduleEntryType(Object, additionalProperties=False):
    """*(2.1)* An entry in schedule of the energy amount over time that EV is willing to discharge. A negative value indicates the willingness to discharge under specific conditions, a positive value indicates that the EV currently is not able to offer energy to discharge.
"""

    duration: int = Property(Integer(description='The duration of this entry.\r\n'), required=True)

    power: float = Property(Number(description='Defines maximum amount of power for the duration of this EVPowerScheduleEntry to be discharged from the EV battery through EVSE power outlet. Negative values are used for discharging.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class EVPowerScheduleType(Object, additionalProperties=False):
    """*(2.1)* Schedule of EV energy offer.
"""

    evPowerScheduleEntries: List[EVPowerScheduleEntryType] = Property(Array(EVPowerScheduleEntryType, additionalItems=False, minItems=1, maxItems=1024), required=True)

    timeAnchor: str = Property(String(format='date-time', description='The time that defines the starting point for the EVEnergyOffer.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class EVPriceRuleType(Object, additionalProperties=False):
    """*(2.1)* An entry in price schedule over time for which EV is willing to discharge.
"""

    energyFee: float = Property(Number(description='Cost per kWh.\r\n'), required=True)

    powerRangeStart: float = Property(Number(description='The EnergyFee applies between this value and the value of the PowerRangeStart of the subsequent EVPriceRule. If the power is below this value, the EnergyFee of the previous EVPriceRule applies. Negative values are used for discharging.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class EVAbsolutePriceScheduleEntryType(Object, additionalProperties=False):
    """*(2.1)* An entry in price schedule over time for which EV is willing to discharge.
"""

    duration: int = Property(Integer(description='The amount of seconds of this entry.\r\n'), required=True)

    evPriceRule: List[EVPriceRuleType] = Property(Array(EVPriceRuleType, additionalItems=False, minItems=1, maxItems=8), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class EVAbsolutePriceScheduleType(Object, additionalProperties=False):
    """*(2.1)* Price schedule of EV energy offer.
"""

    timeAnchor: str = Property(String(format='date-time', description='Starting point in time of the EVEnergyOffer.\r\n'), required=True)

    currency: str = Property(String(maxLength=3, description='Currency code according to ISO 4217.\r\n'), required=True)

    evAbsolutePriceScheduleEntries: List[EVAbsolutePriceScheduleEntryType] = Property(Array(EVAbsolutePriceScheduleEntryType, additionalItems=False, minItems=1, maxItems=1024), required=True)

    priceAlgorithm: str = Property(String(maxLength=2000, description='ISO 15118-20 URN of price algorithm: Power, PeakPower, StackedEnergy.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class EVEnergyOfferType(Object, additionalProperties=False):
    """*(2.1)* A schedule of the energy amount over time that EV is willing to discharge. A negative value indicates the willingness to discharge under specific conditions, a positive value indicates that the EV currently is not able to offer energy to discharge. 
"""

    evAbsolutePriceSchedule: Maybe[EVAbsolutePriceScheduleType] = Property(EVAbsolutePriceScheduleType)

    evPowerSchedule: EVPowerScheduleType = Property(EVPowerScheduleType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class V2XChargingParametersType(Object, additionalProperties=False):
    """Charging parameters for ISO 15118-20, also supporting V2X charging/discharging.+
All values are greater or equal to zero, with the exception of EVMinEnergyRequest, EVMaxEnergyRequest, EVTargetEnergyRequest, EVMinV2XEnergyRequest and EVMaxV2XEnergyRequest.
"""

    minChargePower: Maybe[float] = Property(Number(description='Minimum charge power in W, defined by max(EV, EVSE).\r\nThis field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\nRelates to:\r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMinimumChargePower\r\n'))

    minChargePower_L2: Maybe[float] = Property(Number(description='Minimum charge power on phase L2 in W, defined by max(EV, EVSE).\r\nRelates to:\r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMinimumChargePower_L2\r\n'))

    minChargePower_L3: Maybe[float] = Property(Number(description='Minimum charge power on phase L3 in W, defined by max(EV, EVSE).\r\nRelates to:\r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMinimumChargePower_L3\r\n'))

    maxChargePower: Maybe[float] = Property(Number(description='Maximum charge (absorbed) power in W, defined by min(EV, EVSE) at unity power factor. +\r\nThis field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\nIt corresponds to the ChaWMax attribute in the IEC 61850.\r\nIt is usually equivalent to the rated apparent power of the EV when discharging (ChaVAMax) in IEC 61850. +\r\n\r\nRelates to: \r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMaximumChargePower\r\n\r\n'))

    maxChargePower_L2: Maybe[float] = Property(Number(description='Maximum charge power on phase L2 in W, defined by min(EV, EVSE)\r\nRelates to: \r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMaximumChargePower_L2\r\n\r\n\r\n'))

    maxChargePower_L3: Maybe[float] = Property(Number(description='Maximum charge power on phase L3 in W, defined by min(EV, EVSE)\r\nRelates to: \r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMaximumChargePower_L3\r\n\r\n\r\n'))

    minDischargePower: Maybe[float] = Property(Number(description='Minimum discharge (injected) power in W, defined by max(EV, EVSE) at unity power factor. Value &gt;= 0. +\r\nThis field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1. +\r\nIt corresponds to the WMax attribute in the IEC 61850.\r\nIt is usually equivalent to the rated apparent power of the EV when discharging (VAMax attribute in the IEC 61850).\r\n\r\nRelates to:\r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMinimumDischargePower\r\n\r\n'))

    minDischargePower_L2: Maybe[float] = Property(Number(description='Minimum discharge power on phase L2 in W, defined by max(EV, EVSE).  Value &gt;= 0.\r\nRelates to:\r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMinimumDischargePower_L2\r\n\r\n'))

    minDischargePower_L3: Maybe[float] = Property(Number(description='Minimum discharge power on phase L3 in W, defined by max(EV, EVSE).  Value &gt;= 0.\r\nRelates to:\r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMinimumDischargePower_L3\r\n\r\n'))

    maxDischargePower: Maybe[float] = Property(Number(description='Maximum discharge (injected) power in W, defined by min(EV, EVSE) at unity power factor.  Value &gt;= 0.\r\nThis field represents the sum of all phases, unless values are provided for L2 and L3, in which case this field represents phase L1.\r\nRelates to:\r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMaximumDischargePower\r\n\r\n\r\n'))

    maxDischargePower_L2: Maybe[float] = Property(Number(description='Maximum discharge power on phase L2 in W, defined by min(EV, EVSE).  Value &gt;= 0.\r\nRelates to:\r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMaximumDischargePowe_L2\r\n\r\n'))

    maxDischargePower_L3: Maybe[float] = Property(Number(description='Maximum discharge power on phase L3 in W, defined by min(EV, EVSE).  Value &gt;= 0.\r\nRelates to:\r\n*ISO 15118-20*: BPT_AC/DC_CPDReqEnergyTransferModeType: EVMaximumDischargePower_L3\r\n\r\n'))

    minChargeCurrent: Maybe[float] = Property(Number(description='Minimum charge current in A, defined by max(EV, EVSE)\r\nRelates to: \r\n*ISO 15118-20*: BPT_DC_CPDReqEnergyTransferModeType: EVMinimumChargeCurrent\r\n\r\n'))

    maxChargeCurrent: Maybe[float] = Property(Number(description='Maximum charge current in A, defined by min(EV, EVSE)\r\nRelates to: \r\n*ISO 15118-20*: BPT_DC_CPDReqEnergyTransferModeType: EVMaximumChargeCurrent\r\n\r\n\r\n'))

    minDischargeCurrent: Maybe[float] = Property(Number(description='Minimum discharge current in A, defined by max(EV, EVSE).  Value &gt;= 0.\r\nRelates to: \r\n*ISO 15118-20*: BPT_DC_CPDReqEnergyTransferModeType: EVMinimumDischargeCurrent\r\n\r\n\r\n'))

    maxDischargeCurrent: Maybe[float] = Property(Number(description='Maximum discharge current in A, defined by min(EV, EVSE).  Value &gt;= 0.\r\nRelates to: \r\n*ISO 15118-20*: BPT_DC_CPDReqEnergyTransferModeType: EVMaximumDischargeCurrent\r\n\r\n'))

    minVoltage: Maybe[float] = Property(Number(description='Minimum voltage in V, defined by max(EV, EVSE)\r\nRelates to:\r\n*ISO 15118-20*: BPT_DC_CPDReqEnergyTransferModeType: EVMinimumVoltage\r\n\r\n'))

    maxVoltage: Maybe[float] = Property(Number(description='Maximum voltage in V, defined by min(EV, EVSE)\r\nRelates to:\r\n*ISO 15118-20*: BPT_DC_CPDReqEnergyTransferModeType: EVMaximumVoltage\r\n\r\n'))

    evTargetEnergyRequest: Maybe[float] = Property(Number(description='Energy to requested state of charge in Wh\r\nRelates to:\r\n*ISO 15118-20*: Dynamic/Scheduled_SEReqControlModeType: EVTargetEnergyRequest\r\n\r\n'))

    evMinEnergyRequest: Maybe[float] = Property(Number(description='Energy to minimum allowed state of charge in Wh\r\nRelates to:\r\n*ISO 15118-20*: Dynamic/Scheduled_SEReqControlModeType: EVMinimumEnergyRequest\r\n\r\n'))

    evMaxEnergyRequest: Maybe[float] = Property(Number(description='Energy to maximum state of charge in Wh\r\nRelates to:\r\n*ISO 15118-20*: Dynamic/Scheduled_SEReqControlModeType: EVMaximumEnergyRequest\r\n\r\n'))

    evMinV2XEnergyRequest: Maybe[float] = Property(Number(description='Energy (in Wh) to minimum state of charge for cycling (V2X) activity. \r\nPositive value means that current state of charge is below V2X range.\r\nRelates to:\r\n*ISO 15118-20*: Dynamic_SEReqControlModeType: EVMinimumV2XEnergyRequest\r\n\r\n'))

    evMaxV2XEnergyRequest: Maybe[float] = Property(Number(description='Energy (in Wh) to maximum state of charge for cycling (V2X) activity.\r\nNegative value indicates that current state of charge is above V2X range.\r\nRelates to:\r\n*ISO 15118-20*: Dynamic_SEReqControlModeType: EVMaximumV2XEnergyRequest\r\n\r\n\r\n'))

    targetSoC: Maybe[int] = Property(Integer(minimum=0.0, maximum=100.0, description='Target state of charge at departure as percentage.\r\nRelates to:\r\n*ISO 15118-20*: BPT_DC_CPDReqEnergyTransferModeType: TargetSOC\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ChargingNeedsType(Object, additionalProperties=False):

    acChargingParameters: Maybe[ACChargingParametersType] = Property(ACChargingParametersType)

    derChargingParameters: Maybe[DERChargingParametersType] = Property(DERChargingParametersType)

    evEnergyOffer: Maybe[EVEnergyOfferType] = Property(EVEnergyOfferType)

    requestedEnergyTransfer: str = Property(String(enum=['AC_single_phase', 'AC_two_phase', 'AC_three_phase', 'DC', 'AC_BPT', 'AC_BPT_DER', 'AC_DER', 'DC_BPT', 'DC_ACDP', 'DC_ACDP_BPT', 'WPT'], description='Mode of energy transfer requested by the EV.\r\n'), required=True)

    dcChargingParameters: Maybe[DCChargingParametersType] = Property(DCChargingParametersType)

    v2xChargingParameters: Maybe[V2XChargingParametersType] = Property(V2XChargingParametersType)

    availableEnergyTransfer: Maybe[List[str]] = Property(Array(String(enum=['AC_single_phase', 'AC_two_phase', 'AC_three_phase', 'DC', 'AC_BPT', 'AC_BPT_DER', 'AC_DER', 'DC_BPT', 'DC_ACDP', 'DC_ACDP_BPT', 'WPT'], description='Mode of energy transfer requested by the EV.\r\n'), additionalItems=False, minItems=1, description='*(2.1)* Modes of energy transfer that are marked as available by EV.\r\n'))

    controlMode: Maybe[str] = Property(String(enum=['ScheduledControl', 'DynamicControl'], description='*(2.1)* Indicates whether EV wants to operate in Dynamic or Scheduled mode. When absent, Scheduled mode is assumed for backwards compatibility. +\r\n*ISO 15118-20:* +\r\nServiceSelectionReq(SelectedEnergyTransferService)\r\n'))

    mobilityNeedsMode: Maybe[str] = Property(String(enum=['EVCC', 'EVCC_SECC'], description='*(2.1)* Value of EVCC indicates that EV determines min/target SOC and departure time. +\r\nA value of EVCC_SECC indicates that charging station or CSMS may also update min/target SOC and departure time. +\r\n*ISO 15118-20:* +\r\nServiceSelectionReq(SelectedEnergyTransferService)\r\n'))

    departureTime: Maybe[str] = Property(String(format='date-time', description='Estimated departure time of the EV. +\r\n*ISO 15118-2:* AC/DC_EVChargeParameterType: DepartureTime +\r\n*ISO 15118-20:* Dynamic/Scheduled_SEReqControlModeType: DepartureTIme\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class NotifyEVChargingNeedsRequest(Object, additionalProperties=False):

    evseId: int = Property(Integer(minimum=1.0, description='Defines the EVSE and connector to which the EV is connected. EvseId may not be 0.\r\n'), required=True)

    maxScheduleTuples: Maybe[int] = Property(Integer(minimum=0.0, description='Contains the maximum elements the EV supports for: +\r\n- ISO 15118-2: schedule tuples in SASchedule (both Pmax and Tariff). +\r\n- ISO 15118-20: PowerScheduleEntry, PriceRule and PriceLevelScheduleEntries.\r\n'))

    chargingNeeds: ChargingNeedsType = Property(ChargingNeedsType, required=True)

    timestamp: Maybe[str] = Property(String(format='date-time', description='*(2.1)* Time when EV charging needs were received. +\r\nField can be added when charging station was offline when charging needs were received.\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
