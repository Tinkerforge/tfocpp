from typing import List

from statham.schema.elements import Integer, Object
from statham.schema.property import Property


class GetLocalListVersionResponse(Object, additionalProperties=False):

    listVersion: int = Property(Integer(), required=True)
