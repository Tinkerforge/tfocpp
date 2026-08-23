from typing import Any

from statham.schema.constants import Maybe
from statham.schema.elements import Element, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class StatusInfoType(Object, additionalProperties=False):
    """Element providing more information about the status.
"""

    reasonCode: str = Property(String(maxLength=20, description='A predefined code for the reason why the status is returned in this response. The string is case-insensitive.\r\n'), required=True)

    additionalInfo: Maybe[str] = Property(String(maxLength=1024, description='Additional text to provide detailed information.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class DataTransferResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected', 'UnknownMessageId', 'UnknownVendorId'], description='This indicates the success or failure of the data transfer.\r\n'), required=True)

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    data: Maybe[Any] = Property(Element(description='Data without specified length or format, in response to request.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
