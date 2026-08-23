from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class PublishFirmwareRequest(Object, additionalProperties=False):

    location: str = Property(String(maxLength=2000, description='This contains a string containing a URI pointing to a\r\nlocation from which to retrieve the firmware.\r\n'), required=True)

    retries: Maybe[int] = Property(Integer(minimum=0.0, description='This specifies how many times Charging Station must retry\r\nto download the firmware before giving up. If this field is not\r\npresent, it is left to Charging Station to decide how many times it wants to retry.\r\nIf the value is 0, it means: no retries.\r\n'))

    checksum: str = Property(String(maxLength=32, description='The MD5 checksum over the entire firmware file as a hexadecimal string of length 32. \r\n'), required=True)

    requestId: int = Property(Integer(minimum=0.0, description='The Id of the request.\r\n'), required=True)

    retryInterval: Maybe[int] = Property(Integer(minimum=0.0, description='The interval in seconds\r\nafter which a retry may be\r\nattempted. If this field is not\r\npresent, it is left to Charging\r\nStation to decide how long to wait\r\nbetween attempts.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
