from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class ChangeAvailabilityRequest(Object, additionalProperties=False):

    connectorId: int = Property(Integer(minimum=0), required=True)

    type: str = Property(String(enum=['Inoperative', 'Operative']), required=True)
