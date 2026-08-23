from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class StartTransactionRequest(Object, additionalProperties=False):

    connectorId: int = Property(Integer(minimum=0), required=True)

    idTag: str = Property(String(maxLength=20), required=True)

    meterStart: int = Property(Integer(), required=True)

    reservationId: Maybe[int] = Property(Integer())

    timestamp: str = Property(String(format='date-time'), required=True)
