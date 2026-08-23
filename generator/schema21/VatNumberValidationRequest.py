from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class VatNumberValidationRequest(Object, additionalProperties=False):

    vatNumber: str = Property(String(maxLength=20, description='VAT number to check.\r\n\r\n'), required=True)

    evseId: Maybe[int] = Property(Integer(minimum=0.0, description='EVSE id for which check is done\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
