from statham.schema.elements import Integer, Object
from statham.schema.property import Property


class UnlockConnectorRequest(Object, additionalProperties=False):

    connectorId: int = Property(Integer(minimum=0), required=True)
