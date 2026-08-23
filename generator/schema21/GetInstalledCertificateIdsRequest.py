from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class GetInstalledCertificateIdsRequest(Object, additionalProperties=False):

    certificateType: Maybe[List[str]] = Property(Array(String(enum=['V2GRootCertificate', 'MORootCertificate', 'CSMSRootCertificate', 'V2GCertificateChain', 'ManufacturerRootCertificate', 'OEMRootCertificate']), additionalItems=False, minItems=1, description='Indicates the type of certificates requested. When omitted, all certificate types are requested.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
