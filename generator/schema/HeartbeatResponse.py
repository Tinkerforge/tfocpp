from statham.schema.elements import Object, String
from statham.schema.property import Property


class HeartbeatResponse(Object, additionalProperties=False):

    currentTime: str = Property(String(format='date-time'), required=True)
