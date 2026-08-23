from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Number, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class StreamDataElementType(Object, additionalProperties=False):

    t: float = Property(Number(description='Offset relative to _basetime_ of this message. _basetime_ + _t_ is timestamp of recorded value.\r\n'), required=True)

    v: str = Property(String(maxLength=2500), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class NotifyPeriodicEventStream(Object, additionalProperties=False):

    data: List[StreamDataElementType] = Property(Array(StreamDataElementType, additionalItems=False, minItems=1), required=True)

    id: int = Property(Integer(minimum=0.0, description='Id of stream.\r\n'), required=True)

    pending: int = Property(Integer(minimum=0.0, description='Number of data elements still pending to be sent.\r\n'), required=True)

    basetime: str = Property(String(format='date-time', description='Base timestamp to add to time offset of values.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
