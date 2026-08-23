from statham.schema.constants import Maybe
from statham.schema.elements import Boolean, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class NotifyPriorityChargingRequest(Object, additionalProperties=False):

    transactionId: str = Property(String(maxLength=36, description='The transaction for which priority charging is requested.\r\n'), required=True)

    activated: bool = Property(Boolean(description='True if priority charging was activated. False if it has stopped using the priority charging profile.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
