from statham.schema.elements import Object, String
from statham.schema.property import Property


class UnlockConnectorResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Unlocked', 'UnlockFailed', 'NotSupported']), required=True)
