from statham.schema.elements import Integer, Object
from statham.schema.property import Property


class CancelReservationRequest(Object, additionalProperties=False):

    reservationId: int = Property(Integer(), required=True)
