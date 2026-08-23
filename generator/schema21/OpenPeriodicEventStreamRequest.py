from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class PeriodicEventStreamParamsType(Object, additionalProperties=False):

    interval: Maybe[int] = Property(Integer(minimum=0.0, description='Time in seconds after which stream data is sent.\r\n'))

    values: Maybe[int] = Property(Integer(minimum=0.0, description='Number of items to be sent together in stream.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ConstantStreamDataType(Object, additionalProperties=False):

    id: int = Property(Integer(minimum=0.0, description='Uniquely identifies the stream\r\n'), required=True)

    params: PeriodicEventStreamParamsType = Property(PeriodicEventStreamParamsType, required=True)

    variableMonitoringId: int = Property(Integer(minimum=0.0, description='Id of monitor used to report his event. It can be a preconfigured or hardwired monitor.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class OpenPeriodicEventStreamRequest(Object, additionalProperties=False):

    constantStreamData: ConstantStreamDataType = Property(ConstantStreamDataType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
