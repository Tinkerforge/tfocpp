from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
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


class AuthorizationData(Object, additionalProperties=False):
    """Contains the identifier to use for authorization.
"""

    idToken: IdTokenType = Property(IdTokenType, required=True)

    idTokenInfo: Maybe[IdTokenInfoType] = Property(IdTokenInfoType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SendLocalListRequest(Object, additionalProperties=False):

    localAuthorizationList: Maybe[List[AuthorizationData]] = Property(Array(AuthorizationData, additionalItems=False, minItems=1))

    versionNumber: int = Property(Integer(description='In case of a full update this is the version number of the full list. In case of a differential update it is the version number of the list after the update has been applied.\r\n'), required=True)

    updateType: str = Property(String(enum=['Differential', 'Full'], description='This contains the type of update (full or differential) of this request.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
