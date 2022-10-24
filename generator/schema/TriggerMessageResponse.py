from statham.schema.elements import Object, String
from statham.schema.property import Property


class TriggerMessageResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected', 'NotImplemented']), required=True)
