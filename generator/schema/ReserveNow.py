from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class ReserveNowRequest(Object, additionalProperties=False):

    connectorId: int = Property(Integer(minimum=0), required=True)

    expiryDate: str = Property(String(format='date-time'), required=True)

    idTag: str = Property(String(maxLength=20), required=True)

    parentIdTag: Maybe[str] = Property(String(maxLength=20))

    reservationId: int = Property(Integer(), required=True)
