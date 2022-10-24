from statham.schema.elements import Object, String
from statham.schema.property import Property


class ResetRequest(Object, additionalProperties=False):

    type: str = Property(String(enum=['Hard', 'Soft']), required=True)
