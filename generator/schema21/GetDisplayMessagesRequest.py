from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class GetDisplayMessagesRequest(Object, additionalProperties=False):

    id: Maybe[List[int]] = Property(Array(Integer(minimum=0.0), additionalItems=False, minItems=1, description='If provided the Charging Station shall return Display Messages of the given ids. This field SHALL NOT contain more ids than set in &lt;&lt;configkey-number-of-display-messages,NumberOfDisplayMessages.maxLimit&gt;&gt;\r\n\r\n'))

    requestId: int = Property(Integer(description='The Id of this request.\r\n'), required=True)

    priority: Maybe[str] = Property(String(enum=['AlwaysFront', 'InFront', 'NormalCycle'], description='If provided the Charging Station shall return Display Messages with the given priority only.\r\n'))

    state: Maybe[str] = Property(String(enum=['Charging', 'Faulted', 'Idle', 'Unavailable', 'Suspended', 'Discharging'], description='If provided the Charging Station shall return Display Messages with the given state only. \r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
