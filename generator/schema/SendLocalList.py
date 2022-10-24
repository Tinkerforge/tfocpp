from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
from statham.schema.property import Property


class IdTagInfo(Object, additionalProperties=False):

    expiryDate: Maybe[str] = Property(String(format='date-time'))

    parentIdTag: Maybe[str] = Property(String(maxLength=20))

    status: str = Property(String(enum=['Accepted', 'Blocked', 'Expired', 'Invalid', 'ConcurrentTx']), required=True)


class LocalAuthorizationListItem(Object, additionalProperties=False):

    idTag: str = Property(String(maxLength=20), required=True)

    idTagInfo: Maybe[IdTagInfo] = Property(IdTagInfo)


class SendLocalListRequest(Object, additionalProperties=False):

    listVersion: int = Property(Integer(), required=True)

    localAuthorizationList: Maybe[List[LocalAuthorizationListItem]] = Property(Array(LocalAuthorizationListItem))

    updateType: str = Property(String(enum=['Differential', 'Full']), required=True)
