from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class HeartbeatResponse(Object, additionalProperties=False):

    currentTime: str = Property(String(format='date-time', description='Contains the current time of the CSMS.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
