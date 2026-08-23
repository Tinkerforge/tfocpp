from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class IdTagInfo(Object, additionalProperties=False):

    expiryDate: Maybe[str] = Property(String(format='date-time'))

    parentIdTag: Maybe[str] = Property(String(maxLength=20))

    status: str = Property(String(enum=['Accepted', 'Blocked', 'Expired', 'Invalid', 'ConcurrentTx']), required=True)


class StartTransactionResponse(Object, additionalProperties=False):

    idTagInfo: IdTagInfo = Property(IdTagInfo, required=True)

    transactionId: int = Property(Integer(), required=True)
