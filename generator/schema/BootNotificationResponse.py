from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class BootNotificationResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Pending', 'Rejected']), required=True)

    currentTime: str = Property(String(format='date-time'), required=True)

    interval: int = Property(Integer(), required=True)
