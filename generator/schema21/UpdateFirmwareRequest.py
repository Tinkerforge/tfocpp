from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class FirmwareType(Object, additionalProperties=False):
    """Represents a copy of the firmware that can be loaded/updated on the Charging Station.
"""

    location: str = Property(String(maxLength=2000, description='URI defining the origin of the firmware.\r\n'), required=True)

    retrieveDateTime: str = Property(String(format='date-time', description='Date and time at which the firmware shall be retrieved.\r\n'), required=True)

    installDateTime: Maybe[str] = Property(String(format='date-time', description='Date and time at which the firmware shall be installed.\r\n'))

    signingCertificate: Maybe[str] = Property(String(maxLength=5500, description='Certificate with which the firmware was signed.\r\nPEM encoded X.509 certificate.\r\n'))

    signature: Maybe[str] = Property(String(maxLength=800, description='Base64 encoded firmware signature.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class UpdateFirmwareRequest(Object, additionalProperties=False):

    retries: Maybe[int] = Property(Integer(minimum=0.0, description='This specifies how many times Charging Station must retry to download the firmware before giving up. If this field is not present, it is left to Charging Station to decide how many times it wants to retry.\r\nIf the value is 0, it means: no retries.\r\n'))

    retryInterval: Maybe[int] = Property(Integer(description='The interval in seconds after which a retry may be attempted. If this field is not present, it is left to Charging Station to decide how long to wait between attempts.\r\n'))

    requestId: int = Property(Integer(description='The Id of this request\r\n'), required=True)

    firmware: FirmwareType = Property(FirmwareType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
