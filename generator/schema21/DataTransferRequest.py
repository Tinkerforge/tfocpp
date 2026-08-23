from typing import Any

from statham.schema.constants import Maybe
from statham.schema.elements import Element, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class DataTransferRequest(Object, additionalProperties=False):

    messageId: Maybe[str] = Property(String(maxLength=50, description='May be used to indicate a specific message or implementation.\r\n'))

    data: Maybe[Any] = Property(Element(description='Data without specified length or format. This needs to be decided by both parties (Open to implementation).\r\n'))

    vendorId: str = Property(String(maxLength=255, description='This identifies the Vendor specific implementation\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
