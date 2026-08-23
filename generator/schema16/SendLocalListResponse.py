from typing import List

from statham.schema.elements import Object, String
from statham.schema.property import Property


class SendLocalListResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Failed', 'NotSupported', 'VersionMismatch']), required=True)
