from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class StatusInfoType(Object, additionalProperties=False):
    """Element providing more information about the status.
"""

    reasonCode: str = Property(String(maxLength=20, description='A predefined code for the reason why the status is returned in this response. The string is case-insensitive.\r\n'), required=True)

    additionalInfo: Maybe[str] = Property(String(maxLength=1024, description='Additional text to provide detailed information.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SetChargingProfileResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected'], description='Returns whether the Charging Station has been able to process the message successfully. This does not guarantee the schedule will be followed to the letter. There might be other constraints the Charging Station may need to take into account.\r\n'), required=True)

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
