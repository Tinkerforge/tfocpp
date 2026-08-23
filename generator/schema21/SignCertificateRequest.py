from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
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


class SignCertificateRequest(Object, additionalProperties=False):

    csr: str = Property(String(maxLength=5500, description='The Charging Station SHALL send the public key in form of a Certificate Signing Request (CSR) as described in RFC 2986 [22] and then PEM encoded, using the &lt;&lt;signcertificaterequest,SignCertificateRequest&gt;&gt; message.\r\n'), required=True)

    certificateType: Maybe[str] = Property(String(enum=['ChargingStationCertificate', 'V2GCertificate', 'V2G20Certificate'], description='Indicates the type of certificate that is to be signed. When omitted the certificate is to be used for both the 15118 connection (if implemented) and the Charging Station to CSMS connection.\r\n\r\n'))

    hashRootCertificate: Maybe[CertificateHashDataType] = Property(CertificateHashDataType)

    requestId: Maybe[int] = Property(Integer(description='*(2.1)* RequestId to match this message with the CertificateSignedRequest.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
