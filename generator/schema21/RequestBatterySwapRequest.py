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


class RequestBatterySwapRequest(Object, additionalProperties=False):

    idToken: IdTokenType = Property(IdTokenType, required=True)

    requestId: int = Property(Integer(description='Request id to match with BatterySwapRequest.\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
