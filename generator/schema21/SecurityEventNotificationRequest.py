from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class SecurityEventNotificationRequest(Object, additionalProperties=False):

    type: str = Property(String(maxLength=50, description='Type of the security event. This value should be taken from the Security events list.\r\n'), required=True)

    timestamp: str = Property(String(format='date-time', description='Date and time at which the event occurred.\r\n'), required=True)

    techInfo: Maybe[str] = Property(String(maxLength=255, description='Additional information about the occurred security event.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
