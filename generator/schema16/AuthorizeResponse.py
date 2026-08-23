from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class IdTagInfo(Object, additionalProperties=False):

    expiryDate: Maybe[str] = Property(String(format='date-time'))

    parentIdTag: Maybe[str] = Property(String(maxLength=20))

    status: str = Property(String(enum=['Accepted', 'Blocked', 'Expired', 'Invalid', 'ConcurrentTx']), required=True)


class AuthorizeResponse(Object, additionalProperties=False):

    idTagInfo: IdTagInfo = Property(IdTagInfo, required=True)
