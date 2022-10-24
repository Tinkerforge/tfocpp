from statham.schema.elements import Object, String
from statham.schema.property import Property


class ChangeAvailabilityResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected', 'Scheduled']), required=True)
