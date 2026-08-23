from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Boolean, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class NotifyDERStartStopRequest(Object, additionalProperties=False):

    controlId: str = Property(String(maxLength=36, description='Id of the started or stopped DER control.\r\nCorresponds to the _controlId_ of the SetDERControlRequest.\r\n\r\n'), required=True)

    started: bool = Property(Boolean(description='True if DER control has started. False if it has ended.\r\n\r\n'), required=True)

    timestamp: str = Property(String(format='date-time', description='Time of start or end of event.\r\n\r\n'), required=True)

    supersededIds: Maybe[List[str]] = Property(Array(String(maxLength=36), additionalItems=False, minItems=1, maxItems=24, description='List of controlIds that are superseded as a result of this control starting.\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
