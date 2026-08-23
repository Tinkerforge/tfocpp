from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
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


class TriggerMessageRequest(Object, additionalProperties=False):

    evse: Maybe[EVSEType] = Property(EVSEType)

    requestedMessage: str = Property(String(enum=['BootNotification', 'LogStatusNotification', 'FirmwareStatusNotification', 'Heartbeat', 'MeterValues', 'SignChargingStationCertificate', 'SignV2GCertificate', 'SignV2G20Certificate', 'StatusNotification', 'TransactionEvent', 'SignCombinedCertificate', 'PublishFirmwareStatusNotification', 'CustomTrigger'], description='Type of message to be triggered.\r\n'), required=True)

    customTrigger: Maybe[str] = Property(String(maxLength=50, description='*(2.1)* When _requestedMessage_ = `CustomTrigger` this will trigger sending the corresponding message in field _customTrigger_, if supported by Charging Station.\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
