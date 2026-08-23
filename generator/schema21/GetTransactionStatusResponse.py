from statham.schema.constants import Maybe
from statham.schema.elements import Boolean, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class GetTransactionStatusResponse(Object, additionalProperties=False):

    ongoingIndicator: Maybe[bool] = Property(Boolean(description='Whether the transaction is still ongoing.\r\n'))

    messagesInQueue: bool = Property(Boolean(description='Whether there are still message to be delivered.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
