from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class NotifyPriorityChargingResponse(Object, additionalProperties=False):
    """This response message has an empty body.
"""

    customData: Maybe[CustomDataType] = Property(CustomDataType)
