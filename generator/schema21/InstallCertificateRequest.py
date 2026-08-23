from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class InstallCertificateRequest(Object, additionalProperties=False):

    certificateType: str = Property(String(enum=['V2GRootCertificate', 'MORootCertificate', 'ManufacturerRootCertificate', 'CSMSRootCertificate', 'OEMRootCertificate'], description='Indicates the certificate type that is sent.\r\n'), required=True)

    certificate: str = Property(String(maxLength=10000, description='A PEM encoded X.509 certificate.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
