from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Boolean, Integer, Object, String
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


class CertificateHashDataType(Object, additionalProperties=False):

    hashAlgorithm: str = Property(String(enum=['SHA256', 'SHA384', 'SHA512'], description='Used algorithms for the hashes provided.\r\n'), required=True)

    issuerNameHash: str = Property(String(maxLength=128, description='The hash of the issuer’s distinguished\r\nname (DN), that must be calculated over the DER\r\nencoding of the issuer’s name field in the certificate\r\nbeing checked.\r\n\r\n'), required=True)

    issuerKeyHash: str = Property(String(maxLength=128, description='The hash of the DER encoded public key:\r\nthe value (excluding tag and length) of the subject\r\npublic key field in the issuer’s certificate.\r\n'), required=True)

    serialNumber: str = Property(String(maxLength=40, description='The string representation of the\r\nhexadecimal value of the serial number without the\r\nprefix "0x" and without leading zeroes.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class IdTokenType(Object, additionalProperties=False):
    """Contains a case insensitive identifier to use for the authorization and the type of authorization to support multiple forms of identifiers.
"""

    additionalInfo: Maybe[List[AdditionalInfoType]] = Property(Array(AdditionalInfoType, additionalItems=False, minItems=1))

    idToken: str = Property(String(maxLength=255, description='*(2.1)* IdToken is case insensitive. Might hold the hidden id of an RFID tag, but can for example also contain a UUID.\r\n'), required=True)

    type: str = Property(String(maxLength=20, description='*(2.1)* Enumeration of possible idToken types. Values defined in Appendix as IdTokenEnumStringType.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class CustomerInformationRequest(Object, additionalProperties=False):

    customerCertificate: Maybe[CertificateHashDataType] = Property(CertificateHashDataType)

    idToken: Maybe[IdTokenType] = Property(IdTokenType)

    requestId: int = Property(Integer(minimum=0.0, description='The Id of the request.\r\n\r\n'), required=True)

    report: bool = Property(Boolean(description='Flag indicating whether the Charging Station should return NotifyCustomerInformationRequest messages containing information about the customer referred to.\r\n'), required=True)

    clear: bool = Property(Boolean(description='Flag indicating whether the Charging Station should clear all information about the customer referred to.\r\n'), required=True)

    customerIdentifier: Maybe[str] = Property(String(maxLength=64, description='A (e.g. vendor specific) identifier of the customer this request refers to. This field contains a custom identifier other than IdToken and Certificate.\r\nOne of the possible identifiers (customerIdentifier, customerIdToken or customerCertificate) should be in the request message.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
