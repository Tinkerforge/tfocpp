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


class AdditionalInfoType(Object, additionalProperties=False):
    """Contains a case insensitive identifier to use for the authorization and the type of authorization to support multiple forms of identifiers.
"""

    additionalIdToken: str = Property(String(maxLength=255, description='*(2.1)* This field specifies the additional IdToken.\r\n'), required=True)

    type: str = Property(String(maxLength=50, description='_additionalInfo_ can be used to send extra information to CSMS in addition to the regular authorization with _IdToken_. _AdditionalInfo_ contains one or more custom _types_, which need to be agreed upon by all parties involved. When the _type_ is not supported, the CSMS/Charging Station MAY ignore the _additionalInfo_.\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class CostDimensionType(Object, additionalProperties=False):
    """Volume consumed of cost dimension.
"""

    type: str = Property(String(enum=['Energy', 'MaxCurrent', 'MinCurrent', 'MaxPower', 'MinPower', 'IdleTIme', 'ChargingTime'], description='Type of cost dimension: energy, power, time, etc.\r\n\r\n'), required=True)

    volume: float = Property(Number(description='Volume of the dimension consumed, measured according to the dimension type.\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ChargingPeriodType(Object, additionalProperties=False):
    """A ChargingPeriodType consists of a start time, and a list of possible values that influence this period, for example: amount of energy charged this period, maximum current during this period etc.

"""

    dimensions: Maybe[List[CostDimensionType]] = Property(Array(CostDimensionType, additionalItems=False, minItems=1))

    tariffId: Maybe[str] = Property(String(maxLength=60, description='Unique identifier of the Tariff that was used to calculate cost. If not provided, then cost was calculated by some other means.\r\n\r\n'))

    startPeriod: str = Property(String(format='date-time', description='Start timestamp of charging period. A period ends when the next period starts. The last period ends when the session ends.\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class EVSEType(Object, additionalProperties=False):
    """Electric Vehicle Supply Equipment
"""

    id: int = Property(Integer(minimum=0.0, description='EVSE Identifier. This contains a number (&gt; 0) designating an EVSE of the Charging Station.\r\n'), required=True)

    connectorId: Maybe[int] = Property(Integer(minimum=0.0, description='An id to designate a specific connector (on an EVSE) by connector index number.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class IdTokenType(Object, additionalProperties=False):
    """Contains a case insensitive identifier to use for the authorization and the type of authorization to support multiple forms of identifiers.
"""

    additionalInfo: Maybe[List[AdditionalInfoType]] = Property(Array(AdditionalInfoType, additionalItems=False, minItems=1))

    idToken: str = Property(String(maxLength=255, description='*(2.1)* IdToken is case insensitive. Might hold the hidden id of an RFID tag, but can for example also contain a UUID.\r\n'), required=True)

    type: str = Property(String(maxLength=20, description='*(2.1)* Enumeration of possible idToken types. Values defined in Appendix as IdTokenEnumStringType.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SignedMeterValueType(Object, additionalProperties=False):
    """Represent a signed version of the meter value.
"""

    signedMeterData: str = Property(String(maxLength=32768, description='Base64 encoded, contains the signed data from the meter in the format specified in _encodingMethod_, which might contain more then just the meter value. It can contain information like timestamps, reference to a customer etc.\r\n'), required=True)

    signingMethod: Maybe[str] = Property(String(maxLength=50, description='*(2.1)* Method used to create the digital signature. Optional, if already included in _signedMeterData_. Standard values for this are defined in Appendix as SigningMethodEnumStringType.\r\n'))

    encodingMethod: str = Property(String(maxLength=50, description='Format used by the energy meter to encode the meter data. For example: OCMF or EDL.\r\n'), required=True)

    publicKey: Maybe[str] = Property(String(maxLength=2500, description='*(2.1)* Base64 encoded, sending depends on configuration variable _PublicKeyWithSignedMeterValue_.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TaxRateType(Object, additionalProperties=False):
    """Tax percentage
"""

    type: str = Property(String(maxLength=20, description='Type of this tax, e.g.  "Federal ",  "State", for information on receipt.\r\n'), required=True)

    tax: float = Property(Number(description='Tax percentage\r\n'), required=True)

    stack: Maybe[int] = Property(Integer(minimum=0.0, description='Stack level for this type of tax. Default value, when absent, is 0. +\r\n_stack_ = 0: tax on net price; +\r\n_stack_ = 1: tax added on top of _stack_ 0; +\r\n_stack_ = 2: tax added on top of _stack_ 1, etc. \r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class PriceType(Object, additionalProperties=False):
    """Price with and without tax. At least one of _exclTax_, _inclTax_ must be present.
"""

    exclTax: Maybe[float] = Property(Number(description='Price/cost excluding tax. Can be absent if _inclTax_ is present.\r\n'))

    inclTax: Maybe[float] = Property(Number(description='Price/cost including tax. Can be absent if _exclTax_ is present.\r\n'))

    taxRates: Maybe[List[TaxRateType]] = Property(Array(TaxRateType, additionalItems=False, minItems=1, maxItems=5))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TotalPriceType(Object, additionalProperties=False):
    """Total cost with and without tax. Contains the total of energy, charging time, idle time, fixed and reservation costs including and/or excluding tax.
"""

    exclTax: Maybe[float] = Property(Number(description='Price/cost excluding tax. Can be absent if _inclTax_ is present.\r\n'))

    inclTax: Maybe[float] = Property(Number(description='Price/cost including tax. Can be absent if _exclTax_ is present.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TotalCostType(Object, additionalProperties=False):
    """This contains the cost calculated during a transaction. It is used both for running cost and final cost of the transaction.
"""

    currency: str = Property(String(maxLength=3, description='Currency of the costs in ISO 4217 Code.\r\n\r\n'), required=True)

    typeOfCost: str = Property(String(enum=['NormalCost', 'MinCost', 'MaxCost'], description='Type of cost: normal or the minimum or maximum cost.\r\n'), required=True)

    fixed: Maybe[PriceType] = Property(PriceType)

    energy: Maybe[PriceType] = Property(PriceType)

    chargingTime: Maybe[PriceType] = Property(PriceType)

    idleTime: Maybe[PriceType] = Property(PriceType)

    reservationTime: Maybe[PriceType] = Property(PriceType)

    reservationFixed: Maybe[PriceType] = Property(PriceType)

    total: TotalPriceType = Property(TotalPriceType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TotalUsageType(Object, additionalProperties=False):
    """This contains the calculated usage of energy, charging time and idle time during a transaction.
"""

    energy: float = Property(Number(), required=True)

    chargingTime: int = Property(Integer(description='Total duration of the charging session (including the duration of charging and not charging), in seconds.\r\n\r\n\r\n'), required=True)

    idleTime: int = Property(Integer(description='Total duration of the charging session where the EV was not charging (no energy was transferred between EVSE and EV), in seconds.\r\n\r\n\r\n'), required=True)

    reservationTime: Maybe[int] = Property(Integer(description='Total time of reservation in seconds.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class CostDetailsType(Object, additionalProperties=False):
    """CostDetailsType contains the cost as calculated by Charging Station based on provided TariffType.

NOTE: Reservation is not shown as a _chargingPeriod_, because it took place outside of the transaction.

"""

    chargingPeriods: Maybe[List[ChargingPeriodType]] = Property(Array(ChargingPeriodType, additionalItems=False, minItems=1))

    totalCost: TotalCostType = Property(TotalCostType, required=True)

    totalUsage: TotalUsageType = Property(TotalUsageType, required=True)

    failureToCalculate: Maybe[bool] = Property(Boolean(description='If set to true, then Charging Station has failed to calculate the cost.\r\n\r\n'))

    failureReason: Maybe[str] = Property(String(maxLength=500, description='Optional human-readable reason text in case of failure to calculate.\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TransactionLimitType(Object, additionalProperties=False):
    """Cost, energy, time or SoC limit for a transaction.
"""

    maxCost: Maybe[float] = Property(Number(description='Maximum allowed cost of transaction in currency of tariff.\r\n'))

    maxEnergy: Maybe[float] = Property(Number(description='Maximum allowed energy in Wh to charge in transaction.\r\n'))

    maxTime: Maybe[int] = Property(Integer(description='Maximum duration of transaction in seconds from start to end.\r\n'))

    maxSoC: Maybe[int] = Property(Integer(minimum=0.0, maximum=100.0, description='Maximum State of Charge of EV in percentage.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TransactionType(Object, additionalProperties=False):

    transactionId: str = Property(String(maxLength=36, description='This contains the Id of the transaction.\r\n'), required=True)

    chargingState: Maybe[str] = Property(String(enum=['EVConnected', 'Charging', 'SuspendedEV', 'SuspendedEVSE', 'Idle'], description='Current charging state, is required when state\r\nhas changed. Omitted when there is no communication between EVSE and EV, because no cable is plugged in.\r\n'))

    timeSpentCharging: Maybe[int] = Property(Integer(description='Contains the total time that energy flowed from EVSE to EV during the transaction (in seconds). Note that timeSpentCharging is smaller or equal to the duration of the transaction.\r\n'))

    stoppedReason: Maybe[str] = Property(String(enum=['DeAuthorized', 'EmergencyStop', 'EnergyLimitReached', 'EVDisconnected', 'GroundFault', 'ImmediateReset', 'MasterPass', 'Local', 'LocalOutOfCredit', 'Other', 'OvercurrentFault', 'PowerLoss', 'PowerQuality', 'Reboot', 'Remote', 'SOCLimitReached', 'StoppedByEV', 'TimeLimitReached', 'Timeout', 'ReqEnergyTransferRejected'], description='The _stoppedReason_ is the reason/event that initiated the process of stopping the transaction. It will normally be the user stopping authorization via card (Local or MasterPass) or app (Remote), but it can also be CSMS revoking authorization (DeAuthorized), or disconnecting the EV when TxStopPoint = EVConnected (EVDisconnected). Most other reasons are related to technical faults or energy limitations. +\r\nMAY only be omitted when _stoppedReason_ is "Local"\r\n\r\n\r\n'))

    remoteStartId: Maybe[int] = Property(Integer(description='The ID given to remote start request (&lt;&lt;requeststarttransactionrequest, RequestStartTransactionRequest&gt;&gt;. This enables to CSMS to match the started transaction to the given start request.\r\n'))

    operationMode: Maybe[str] = Property(String(enum=['Idle', 'ChargingOnly', 'CentralSetpoint', 'ExternalSetpoint', 'ExternalLimits', 'CentralFrequency', 'LocalFrequency', 'LocalLoadBalancing'], description='*(2.1)* The _operationMode_ that is currently in effect for the transaction.\r\n'))

    tariffId: Maybe[str] = Property(String(maxLength=60, description='*(2.1)* Id of tariff in use for transaction\r\n'))

    transactionLimit: Maybe[TransactionLimitType] = Property(TransactionLimitType)

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


class TransactionEventRequest(Object, additionalProperties=False):

    costDetails: Maybe[CostDetailsType] = Property(CostDetailsType)

    eventType: str = Property(String(enum=['Ended', 'Started', 'Updated'], description='This contains the type of this event.\r\nThe first TransactionEvent of a transaction SHALL contain: "Started" The last TransactionEvent of a transaction SHALL contain: "Ended" All others SHALL contain: "Updated"\r\n'), required=True)

    meterValue: Maybe[List[MeterValueType]] = Property(Array(MeterValueType, additionalItems=False, minItems=1))

    timestamp: str = Property(String(format='date-time', description='The date and time at which this transaction event occurred.\r\n'), required=True)

    triggerReason: str = Property(String(enum=['AbnormalCondition', 'Authorized', 'CablePluggedIn', 'ChargingRateChanged', 'ChargingStateChanged', 'CostLimitReached', 'Deauthorized', 'EnergyLimitReached', 'EVCommunicationLost', 'EVConnectTimeout', 'EVDeparted', 'EVDetected', 'LimitSet', 'MeterValueClock', 'MeterValuePeriodic', 'OperationModeChanged', 'RemoteStart', 'RemoteStop', 'ResetCommand', 'RunningCost', 'SignedDataReceived', 'SoCLimitReached', 'StopAuthorized', 'TariffChanged', 'TariffNotAccepted', 'TimeLimitReached', 'Trigger', 'TxResumed', 'UnlockCommand'], description='Reason the Charging Station sends this message to the CSMS\r\n'), required=True)

    seqNo: int = Property(Integer(minimum=0.0, description='Incremental sequence number, helps with determining if all messages of a transaction have been received.\r\n'), required=True)

    offline: bool = Property(Boolean(default=False, description='Indication that this transaction event happened when the Charging Station was offline. Default = false, meaning: the event occurred when the Charging Station was online.\r\n'))

    numberOfPhasesUsed: Maybe[int] = Property(Integer(minimum=0.0, maximum=3.0, description='If the Charging Station is able to report the number of phases used, then it SHALL provide it.\r\nWhen omitted the CSMS may be able to determine the number of phases used as follows: +\r\n1: The numberPhases in the currently used ChargingSchedule. +\r\n2: The number of phases provided via device management.\r\n'))

    cableMaxCurrent: Maybe[int] = Property(Integer(description='The maximum current of the connected cable in Ampere (A).\r\n'))

    reservationId: Maybe[int] = Property(Integer(minimum=0.0, description='This contains the Id of the reservation that terminates as a result of this transaction.\r\n'))

    preconditioningStatus: Maybe[str] = Property(String(enum=['Unknown', 'Ready', 'NotReady', 'Preconditioning'], description='*(2.1)* The current preconditioning status of the BMS in the EV. Default value is Unknown.\r\n'))

    evseSleep: Maybe[bool] = Property(Boolean(description='*(2.1)* True when EVSE electronics are in sleep mode for this transaction. Default value (when absent) is false.\r\n\r\n'))

    transactionInfo: TransactionType = Property(TransactionType, required=True)

    evse: Maybe[EVSEType] = Property(EVSEType)

    idToken: Maybe[IdTokenType] = Property(IdTokenType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
