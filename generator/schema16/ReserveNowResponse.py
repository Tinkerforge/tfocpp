from statham.schema.elements import Object, String
from statham.schema.property import Property


class ReserveNowResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Faulted', 'Occupied', 'Rejected', 'Unavailable']), required=True)
