from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class RequestStopTransactionRequest(Object, additionalProperties=False):

    transactionId: str = Property(String(maxLength=36, description='The identifier of the transaction which the Charging Station is requested to stop.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
