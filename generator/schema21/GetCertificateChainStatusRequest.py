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


class CertificateStatusRequestInfoType(Object, additionalProperties=False):
    """Data necessary to request the revocation status of a certificate.
"""

    certificateHashData: CertificateHashDataType = Property(CertificateHashDataType, required=True)

    source: str = Property(String(enum=['CRL', 'OCSP'], description='Source of status: OCSP, CRL\r\n'), required=True)

    urls: List[str] = Property(Array(String(maxLength=2000), additionalItems=False, minItems=1, maxItems=5, description='URL(s) of _source_.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class GetCertificateChainStatusRequest(Object, additionalProperties=False):

    certificateStatusRequests: List[CertificateStatusRequestInfoType] = Property(Array(CertificateStatusRequestInfoType, additionalItems=False, minItems=1, maxItems=4), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
