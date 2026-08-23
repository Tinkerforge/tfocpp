from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class CertificateHashDataType(Object, additionalProperties=False):

    hashAlgorithm: str = Property(String(enum=['SHA256', 'SHA384', 'SHA512'], description='Used algorithms for the hashes provided.\r\n'), required=True)

    issuerNameHash: str = Property(String(maxLength=128, description='The hash of the issuer’s distinguished\r\nname (DN), that must be calculated over the DER\r\nencoding of the issuer’s name field in the certificate\r\nbeing checked.\r\n\r\n'), required=True)

    issuerKeyHash: str = Property(String(maxLength=128, description='The hash of the DER encoded public key:\r\nthe value (excluding tag and length) of the subject\r\npublic key field in the issuer’s certificate.\r\n'), required=True)

    serialNumber: str = Property(String(maxLength=40, description='The string representation of the\r\nhexadecimal value of the serial number without the\r\nprefix "0x" and without leading zeroes.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class CertificateHashDataChainType(Object, additionalProperties=False):

    certificateHashData: CertificateHashDataType = Property(CertificateHashDataType, required=True)

    certificateType: str = Property(String(enum=['V2GRootCertificate', 'MORootCertificate', 'CSMSRootCertificate', 'V2GCertificateChain', 'ManufacturerRootCertificate', 'OEMRootCertificate'], description='Indicates the type of the requested certificate(s).\r\n'), required=True)

    childCertificateHashData: Maybe[List[CertificateHashDataType]] = Property(Array(CertificateHashDataType, additionalItems=False, minItems=1, maxItems=4))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class StatusInfoType(Object, additionalProperties=False):
    """Element providing more information about the status.
"""

    reasonCode: str = Property(String(maxLength=20, description='A predefined code for the reason why the status is returned in this response. The string is case-insensitive.\r\n'), required=True)

    additionalInfo: Maybe[str] = Property(String(maxLength=1024, description='Additional text to provide detailed information.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class GetInstalledCertificateIdsResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'NotFound'], description='Charging Station indicates if it can process the request.\r\n'), required=True)

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    certificateHashDataChain: Maybe[List[CertificateHashDataChainType]] = Property(Array(CertificateHashDataChainType, additionalItems=False, minItems=1))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
