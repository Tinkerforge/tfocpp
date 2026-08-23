from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Number, Object, String
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


class IdTokenType(Object, additionalProperties=False):
    """Contains a case insensitive identifier to use for the authorization and the type of authorization to support multiple forms of identifiers.
"""

    additionalInfo: Maybe[List[AdditionalInfoType]] = Property(Array(AdditionalInfoType, additionalItems=False, minItems=1))

    idToken: str = Property(String(maxLength=255, description='*(2.1)* IdToken is case insensitive. Might hold the hidden id of an RFID tag, but can for example also contain a UUID.\r\n'), required=True)

    type: str = Property(String(maxLength=20, description='*(2.1)* Enumeration of possible idToken types. Values defined in Appendix as IdTokenEnumStringType.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class MessageContentType(Object, additionalProperties=False):
    """Contains message details, for a message to be displayed on a Charging Station.

"""

    format: str = Property(String(enum=['ASCII', 'HTML', 'URI', 'UTF8', 'QRCODE'], description='Format of the message.\r\n'), required=True)

    language: Maybe[str] = Property(String(maxLength=8, description='Message language identifier. Contains a language code as defined in &lt;&lt;ref-RFC5646,[RFC5646]&gt;&gt;.\r\n'))

    content: str = Property(String(maxLength=1024, description='*(2.1)* Required. Message contents. +\r\nMaximum length supported by Charging Station is given in OCPPCommCtrlr.FieldLength["MessageContentType.content"].\r\n    Maximum length defaults to 1024.\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class IdTokenInfoType(Object, additionalProperties=False):
    """Contains status information about an identifier.
It is advised to not stop charging for a token that expires during charging, as ExpiryDate is only used for caching purposes. If ExpiryDate is not given, the status has no end date.
"""

    status: str = Property(String(enum=['Accepted', 'Blocked', 'ConcurrentTx', 'Expired', 'Invalid', 'NoCredit', 'NotAllowedTypeEVSE', 'NotAtThisLocation', 'NotAtThisTime', 'Unknown'], description='Current status of the ID Token.\r\n'), required=True)

    cacheExpiryDateTime: Maybe[str] = Property(String(format='date-time', description='Date and Time after which the token must be considered invalid.\r\n'))

    chargingPriority: Maybe[int] = Property(Integer(description='Priority from a business point of view. Default priority is 0, The range is from -9 to 9. Higher values indicate a higher priority. The chargingPriority in &lt;&lt;transactioneventresponse,TransactionEventResponse&gt;&gt; overrules this one. \r\n'))

    groupIdToken: Maybe[IdTokenType] = Property(IdTokenType)

    language1: Maybe[str] = Property(String(maxLength=8, description='Preferred user interface language of identifier user. Contains a language code as defined in &lt;&lt;ref-RFC5646,[RFC5646]&gt;&gt;.\r\n\r\n'))

    language2: Maybe[str] = Property(String(maxLength=8, description='Second preferred user interface language of identifier user. Don’t use when language1 is omitted, has to be different from language1. Contains a language code as defined in &lt;&lt;ref-RFC5646,[RFC5646]&gt;&gt;.\r\n'))

    evseId: Maybe[List[int]] = Property(Array(Integer(minimum=0.0), additionalItems=False, minItems=1, description='Only used when the IdToken is only valid for one or more specific EVSEs, not for the entire Charging Station.\r\n\r\n'))

    personalMessage: Maybe[MessageContentType] = Property(MessageContentType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TariffConditionsFixedType(Object, additionalProperties=False):
    """These conditions describe if a FixedPrice applies at start of the transaction.

When more than one restriction is set, they are to be treated as a logical AND. All need to be valid before this price is active.

NOTE: _startTimeOfDay_ and _endTimeOfDay_ are in local time, because it is the time in the tariff as it is shown to the EV driver at the Charging Station.
A Charging Station will convert this to the internal time zone that it uses (which is recommended to be UTC, see section Generic chapter 3.1) when performing cost calculation.

"""

    startTimeOfDay: Maybe[str] = Property(String(description='Start time of day in local time. +\r\nFormat as per RFC 3339: time-hour ":" time-minute  +\r\nMust be in 24h format with leading zeros. Hour/Minute separator: ":"\r\nRegex: ([0-1][0-9]\\|2[0-3]):[0-5][0-9]\r\n'))

    endTimeOfDay: Maybe[str] = Property(String(description='End time of day in local time. Same syntax as _startTimeOfDay_. +\r\n    If end time &lt; start time then the period wraps around to the next day. +\r\n    To stop at end of the day use: 00:00.\r\n'))

    dayOfWeek: Maybe[List[str]] = Property(Array(String(enum=['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday']), additionalItems=False, minItems=1, maxItems=7, description='Day(s) of the week this is tariff applies.\r\n'))

    validFromDate: Maybe[str] = Property(String(description='Start date in local time, for example: 2015-12-24.\r\nValid from this day (inclusive). +\r\nFormat as per RFC 3339: full-date  + \r\n\r\nRegex: ([12][0-9]{3})-(0[1-9]\\|1[0-2])-(0[1-9]\\|[12][0-9]\\|3[01])\r\n'))

    validToDate: Maybe[str] = Property(String(description='End date in local time, for example: 2015-12-27.\r\n    Valid until this day (exclusive). Same syntax as _validFromDate_.\r\n'))

    evseKind: Maybe[str] = Property(String(enum=['AC', 'DC'], description='Type of EVSE (AC, DC) this tariff applies to.\r\n'))

    paymentBrand: Maybe[str] = Property(String(maxLength=20, description='For which payment brand this (adhoc) tariff applies. Can be used to add a surcharge for certain payment brands.\r\n    Based on value of _additionalIdToken_ from _idToken.additionalInfo.type_ = "PaymentBrand".\r\n'))

    paymentRecognition: Maybe[str] = Property(String(maxLength=20, description='Type of adhoc payment, e.g. CC, Debit.\r\n    Based on value of _additionalIdToken_ from _idToken.additionalInfo.type_ = "PaymentRecognition".\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TariffConditionsType(Object, additionalProperties=False):
    """These conditions describe if and when a TariffEnergyType or TariffTimeType applies during a transaction.

When more than one restriction is set, they are to be treated as a logical AND. All need to be valid before this price is active.

For reverse energy flow (discharging) negative values of energy, power and current are used.

NOTE: _minXXX_ (where XXX = Kwh/A/Kw) must be read as "closest to zero", and _maxXXX_ as "furthest from zero". For example, a *charging* power range from 10 kW to 50 kWh is given by _minPower_ = 10000 and _maxPower_ = 50000, and a *discharging* power range from -10 kW to -50 kW is given by _minPower_ = -10 and _maxPower_ = -50.

NOTE: _startTimeOfDay_ and _endTimeOfDay_ are in local time, because it is the time in the tariff as it is shown to the EV driver at the Charging Station.
A Charging Station will convert this to the internal time zone that it uses (which is recommended to be UTC, see section Generic chapter 3.1) when performing cost calculation.

"""

    startTimeOfDay: Maybe[str] = Property(String(description='Start time of day in local time. +\r\nFormat as per RFC 3339: time-hour ":" time-minute  +\r\nMust be in 24h format with leading zeros. Hour/Minute separator: ":"\r\nRegex: ([0-1][0-9]\\|2[0-3]):[0-5][0-9]\r\n'))

    endTimeOfDay: Maybe[str] = Property(String(description='End time of day in local time. Same syntax as _startTimeOfDay_. +\r\n    If end time &lt; start time then the period wraps around to the next day. +\r\n    To stop at end of the day use: 00:00.\r\n'))

    dayOfWeek: Maybe[List[str]] = Property(Array(String(enum=['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday']), additionalItems=False, minItems=1, maxItems=7, description='Day(s) of the week this is tariff applies.\r\n'))

    validFromDate: Maybe[str] = Property(String(description='Start date in local time, for example: 2015-12-24.\r\nValid from this day (inclusive). +\r\nFormat as per RFC 3339: full-date  + \r\n\r\nRegex: ([12][0-9]{3})-(0[1-9]\\|1[0-2])-(0[1-9]\\|[12][0-9]\\|3[01])\r\n'))

    validToDate: Maybe[str] = Property(String(description='End date in local time, for example: 2015-12-27.\r\n    Valid until this day (exclusive). Same syntax as _validFromDate_.\r\n'))

    evseKind: Maybe[str] = Property(String(enum=['AC', 'DC'], description='Type of EVSE (AC, DC) this tariff applies to.\r\n'))

    minEnergy: Maybe[float] = Property(Number(description='Minimum consumed energy in Wh, for example 20000 Wh.\r\n    Valid from this amount of energy (inclusive) being used.\r\n'))

    maxEnergy: Maybe[float] = Property(Number(description='Maximum consumed energy in Wh, for example 50000 Wh.\r\n    Valid until this amount of energy (exclusive) being used.\r\n'))

    minCurrent: Maybe[float] = Property(Number(description='Sum of the minimum current (in Amperes) over all phases, for example 5 A.\r\n    When the EV is charging with more than, or equal to, the defined amount of current, this price is/becomes active. If the charging current is or becomes lower, this price is not or no longer valid and becomes inactive. +\r\n    This is NOT about the minimum current over the entire transaction.\r\n'))

    maxCurrent: Maybe[float] = Property(Number(description='Sum of the maximum current (in Amperes) over all phases, for example 20 A.\r\n      When the EV is charging with less than the defined amount of current, this price becomes/is active. If the charging current is or becomes higher, this price is not or no longer valid and becomes inactive.\r\n      This is NOT about the maximum current over the entire transaction.\r\n'))

    minPower: Maybe[float] = Property(Number(description='Minimum power in W, for example 5000 W.\r\n      When the EV is charging with more than, or equal to, the defined amount of power, this price is/becomes active.\r\n      If the charging power is or becomes lower, this price is not or no longer valid and becomes inactive.\r\n      This is NOT about the minimum power over the entire transaction.\r\n'))

    maxPower: Maybe[float] = Property(Number(description='Maximum power in W, for example 20000 W.\r\n      When the EV is charging with less than the defined amount of power, this price becomes/is active.\r\n      If the charging power is or becomes higher, this price is not or no longer valid and becomes inactive.\r\n      This is NOT about the maximum power over the entire transaction.\r\n'))

    minTime: Maybe[int] = Property(Integer(description='Minimum duration in seconds the transaction (charging &amp; idle) MUST last (inclusive).\r\n      When the duration of a transaction is longer than the defined value, this price is or becomes active.\r\n      Before that moment, this price is not yet active.\r\n'))

    maxTime: Maybe[int] = Property(Integer(description='Maximum duration in seconds the transaction (charging &amp; idle) MUST last (exclusive).\r\n      When the duration of a transaction is shorter than the defined value, this price is or becomes active.\r\n      After that moment, this price is no longer active.\r\n'))

    minChargingTime: Maybe[int] = Property(Integer(description='Minimum duration in seconds the charging MUST last (inclusive).\r\n      When the duration of a charging is longer than the defined value, this price is or becomes active.\r\n      Before that moment, this price is not yet active.\r\n'))

    maxChargingTime: Maybe[int] = Property(Integer(description='Maximum duration in seconds the charging MUST last (exclusive).\r\n      When the duration of a charging is shorter than the defined value, this price is or becomes active.\r\n      After that moment, this price is no longer active.\r\n'))

    minIdleTime: Maybe[int] = Property(Integer(description='Minimum duration in seconds the idle period (i.e. not charging) MUST last (inclusive).\r\n      When the duration of the idle time is longer than the defined value, this price is or becomes active.\r\n      Before that moment, this price is not yet active.\r\n'))

    maxIdleTime: Maybe[int] = Property(Integer(description='Maximum duration in seconds the idle period (i.e. not charging) MUST last (exclusive).\r\n      When the duration of idle time is shorter than the defined value, this price is or becomes active.\r\n      After that moment, this price is no longer active.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TariffEnergyPriceType(Object, additionalProperties=False):
    """Tariff with optional conditions for an energy price.
"""

    priceKwh: float = Property(Number(description='Price per kWh (excl. tax) for this element.\r\n'), required=True)

    conditions: Maybe[TariffConditionsType] = Property(TariffConditionsType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TariffFixedPriceType(Object, additionalProperties=False):
    """Tariff with optional conditions for a fixed price.
"""

    conditions: Maybe[TariffConditionsFixedType] = Property(TariffConditionsFixedType)

    priceFixed: float = Property(Number(description='Fixed price  for this element e.g. a start fee.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TariffTimePriceType(Object, additionalProperties=False):
    """Tariff with optional conditions for a time duration price.
"""

    priceMinute: float = Property(Number(description='Price per minute (excl. tax) for this element.\r\n'), required=True)

    conditions: Maybe[TariffConditionsType] = Property(TariffConditionsType)

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


class TariffEnergyType(Object, additionalProperties=False):
    """Price elements and tax for energy
"""

    prices: List[TariffEnergyPriceType] = Property(Array(TariffEnergyPriceType, additionalItems=False, minItems=1), required=True)

    taxRates: Maybe[List[TaxRateType]] = Property(Array(TaxRateType, additionalItems=False, minItems=1, maxItems=5))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TariffFixedType(Object, additionalProperties=False):

    prices: List[TariffFixedPriceType] = Property(Array(TariffFixedPriceType, additionalItems=False, minItems=1), required=True)

    taxRates: Maybe[List[TaxRateType]] = Property(Array(TaxRateType, additionalItems=False, minItems=1, maxItems=5))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TariffTimeType(Object, additionalProperties=False):
    """Price elements and tax for time

"""

    prices: List[TariffTimePriceType] = Property(Array(TariffTimePriceType, additionalItems=False, minItems=1), required=True)

    taxRates: Maybe[List[TaxRateType]] = Property(Array(TaxRateType, additionalItems=False, minItems=1, maxItems=5))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class TariffType(Object, additionalProperties=False):
    """A tariff is described by fields with prices for:
energy,
charging time,
idle time,
fixed fee,
reservation time,
reservation fixed fee. +
Each of these fields may have (optional) conditions that specify when a price is applicable. +
The _description_ contains a human-readable explanation of the tariff to be shown to the user. +
The other fields are parameters that define the tariff. These are used by the charging station to calculate the price.
"""

    tariffId: str = Property(String(maxLength=60, description='Unique id of tariff\r\n'), required=True)

    description: Maybe[List[MessageContentType]] = Property(Array(MessageContentType, additionalItems=False, minItems=1, maxItems=10))

    currency: str = Property(String(maxLength=3, description='Currency code according to ISO 4217\r\n'), required=True)

    energy: Maybe[TariffEnergyType] = Property(TariffEnergyType)

    validFrom: Maybe[str] = Property(String(format='date-time', description='Time when this tariff becomes active. When absent, it is immediately active.\r\n'))

    chargingTime: Maybe[TariffTimeType] = Property(TariffTimeType)

    idleTime: Maybe[TariffTimeType] = Property(TariffTimeType)

    fixedFee: Maybe[TariffFixedType] = Property(TariffFixedType)

    reservationTime: Maybe[TariffTimeType] = Property(TariffTimeType)

    reservationFixed: Maybe[TariffFixedType] = Property(TariffFixedType)

    minCost: Maybe[PriceType] = Property(PriceType)

    maxCost: Maybe[PriceType] = Property(PriceType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class AuthorizeResponse(Object, additionalProperties=False):

    idTokenInfo: IdTokenInfoType = Property(IdTokenInfoType, required=True)

    certificateStatus: Maybe[str] = Property(String(enum=['Accepted', 'SignatureError', 'CertificateExpired', 'CertificateRevoked', 'NoCertificateAvailable', 'CertChainError', 'ContractCancelled'], description="Certificate status information. \r\n- if all certificates are valid: return 'Accepted'.\r\n- if one of the certificates was revoked, return 'CertificateRevoked'.\r\n"))

    allowedEnergyTransfer: Maybe[List[str]] = Property(Array(String(enum=['AC_single_phase', 'AC_two_phase', 'AC_three_phase', 'DC', 'AC_BPT', 'AC_BPT_DER', 'AC_DER', 'DC_BPT', 'DC_ACDP', 'DC_ACDP_BPT', 'WPT']), additionalItems=False, minItems=1, description='*(2.1)* List of allowed energy transfer modes the EV can choose from. If omitted this defaults to charging only.\r\n\r\n\r\n'))

    tariff: Maybe[TariffType] = Property(TariffType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
