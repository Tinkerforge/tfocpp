from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class AFRRSignalRequest(Object, additionalProperties=False):

    timestamp: str = Property(String(format='date-time', description='Time when signal becomes active.\r\n'), required=True)

    signal: int = Property(Integer(description='Value of signal in _v2xSignalWattCurve_. \r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
