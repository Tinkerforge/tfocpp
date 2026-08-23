from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class GetTransactionStatusRequest(Object, additionalProperties=False):

    transactionId: Maybe[str] = Property(String(maxLength=36, description='The Id of the transaction for which the status is requested.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
