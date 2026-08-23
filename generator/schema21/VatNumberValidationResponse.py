from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class AddressType(Object, additionalProperties=False):
    """*(2.1)* A generic address format.
"""

    name: str = Property(String(maxLength=50, description='Name of person/company\r\n'), required=True)

    address1: str = Property(String(maxLength=100, description='Address line 1\r\n'), required=True)

    address2: Maybe[str] = Property(String(maxLength=100, description='Address line 2\r\n'))

    city: str = Property(String(maxLength=100, description='City\r\n'), required=True)

    postalCode: Maybe[str] = Property(String(maxLength=20, description='Postal code\r\n'))

    country: str = Property(String(maxLength=50, description='Country name\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class StatusInfoType(Object, additionalProperties=False):
    """Element providing more information about the status.
"""

    reasonCode: str = Property(String(maxLength=20, description='A predefined code for the reason why the status is returned in this response. The string is case-insensitive.\r\n'), required=True)

    additionalInfo: Maybe[str] = Property(String(maxLength=1024, description='Additional text to provide detailed information.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class VatNumberValidationResponse(Object, additionalProperties=False):

    company: Maybe[AddressType] = Property(AddressType)

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    vatNumber: str = Property(String(maxLength=20, description='VAT number that was requested.\r\n\r\n'), required=True)

    evseId: Maybe[int] = Property(Integer(minimum=0.0, description='EVSE id for which check was requested. \r\n\r\n'))

    status: str = Property(String(enum=['Accepted', 'Rejected'], description='Result of operation.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
