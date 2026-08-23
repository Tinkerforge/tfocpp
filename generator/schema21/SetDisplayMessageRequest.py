from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class EVSEType(Object, additionalProperties=False):
    """Electric Vehicle Supply Equipment
"""

    id: int = Property(Integer(minimum=0.0, description='EVSE Identifier. This contains a number (&gt; 0) designating an EVSE of the Charging Station.\r\n'), required=True)

    connectorId: Maybe[int] = Property(Integer(minimum=0.0, description='An id to designate a specific connector (on an EVSE) by connector index number.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ComponentType(Object, additionalProperties=False):
    """A physical or logical component
"""

    evse: Maybe[EVSEType] = Property(EVSEType)

    name: str = Property(String(maxLength=50, description='Name of the component. Name should be taken from the list of standardized component names whenever possible. Case Insensitive. strongly advised to use Camel Case.\r\n'), required=True)

    instance: Maybe[str] = Property(String(maxLength=50, description='Name of instance in case the component exists as multiple instances. Case Insensitive. strongly advised to use Camel Case.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class MessageContentType(Object, additionalProperties=False):
    """Contains message details, for a message to be displayed on a Charging Station.

"""

    format: str = Property(String(enum=['ASCII', 'HTML', 'URI', 'UTF8', 'QRCODE'], description='Format of the message.\r\n'), required=True)

    language: Maybe[str] = Property(String(maxLength=8, description='Message language identifier. Contains a language code as defined in &lt;&lt;ref-RFC5646,[RFC5646]&gt;&gt;.\r\n'))

    content: str = Property(String(maxLength=1024, description='*(2.1)* Required. Message contents. +\r\nMaximum length supported by Charging Station is given in OCPPCommCtrlr.FieldLength["MessageContentType.content"].\r\n    Maximum length defaults to 1024.\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class MessageInfoType(Object, additionalProperties=False):
    """Contains message details, for a message to be displayed on a Charging Station.
"""

    display: Maybe[ComponentType] = Property(ComponentType)

    id: int = Property(Integer(minimum=0.0, description='Unique id within an exchange context. It is defined within the OCPP context as a positive Integer value (greater or equal to zero).\r\n'), required=True)

    priority: str = Property(String(enum=['AlwaysFront', 'InFront', 'NormalCycle'], description='With what priority should this message be shown\r\n'), required=True)

    state: Maybe[str] = Property(String(enum=['Charging', 'Faulted', 'Idle', 'Unavailable', 'Suspended', 'Discharging'], description='During what state should this message be shown. When omitted this message should be shown in any state of the Charging Station.\r\n'))

    startDateTime: Maybe[str] = Property(String(format='date-time', description='From what date-time should this message be shown. If omitted: directly.\r\n'))

    endDateTime: Maybe[str] = Property(String(format='date-time', description='Until what date-time should this message be shown, after this date/time this message SHALL be removed.\r\n'))

    transactionId: Maybe[str] = Property(String(maxLength=36, description='During which transaction shall this message be shown.\r\nMessage SHALL be removed by the Charging Station after transaction has\r\nended.\r\n'))

    message: MessageContentType = Property(MessageContentType, required=True)

    messageExtra: Maybe[List[MessageContentType]] = Property(Array(MessageContentType, additionalItems=False, minItems=1, maxItems=4))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SetDisplayMessageRequest(Object, additionalProperties=False):

    message: MessageInfoType = Property(MessageInfoType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
