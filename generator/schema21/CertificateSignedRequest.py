from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class CertificateSignedRequest(Object, additionalProperties=False):

    certificateChain: str = Property(String(maxLength=10000, description='The signed PEM encoded X.509 certificate. This SHALL also contain the necessary sub CA certificates, when applicable. The order of the bundle follows the certificate chain, starting from the leaf certificate.\r\n\r\nThe Configuration Variable &lt;&lt;configkey-max-certificate-chain-size,MaxCertificateChainSize&gt;&gt; can be used to limit the maximum size of this field.\r\n'), required=True)

    certificateType: Maybe[str] = Property(String(enum=['ChargingStationCertificate', 'V2GCertificate', 'V2G20Certificate'], description='Indicates the type of the signed certificate that is returned. When omitted the certificate is used for both the 15118 connection (if implemented) and the Charging Station to CSMS connection. This field is required when a typeOfCertificate was included in the &lt;&lt;signcertificaterequest,SignCertificateRequest&gt;&gt; that requested this certificate to be signed AND both the 15118 connection and the Charging Station connection are implemented.\r\n\r\n'))

    requestId: Maybe[int] = Property(Integer(description='*(2.1)* RequestId to correlate this message with the SignCertificateRequest.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
