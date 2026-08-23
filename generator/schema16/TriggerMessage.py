from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class TriggerMessageRequest(Object, additionalProperties=False):

    requestedMessage: str = Property(String(enum=['BootNotification', 'DiagnosticsStatusNotification', 'FirmwareStatusNotification', 'Heartbeat', 'MeterValues', 'StatusNotification']), required=True)

    connectorId: Maybe[int] = Property(Integer(minimum=0))
