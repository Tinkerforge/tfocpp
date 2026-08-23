from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class NotifyWebPaymentStartedRequest(Object, additionalProperties=False):

    evseId: int = Property(Integer(minimum=0.0, description='EVSE id for which transaction is requested.\r\n\r\n'), required=True)

    timeout: int = Property(Integer(description='Timeout value in seconds after which no result of web payment process (e.g. QR code scanning) is to be expected anymore.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
