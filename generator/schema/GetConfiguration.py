from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Object, String
from statham.schema.property import Property


class GetConfigurationRequest(Object, additionalProperties=False):

    key: Maybe[List[str]] = Property(Array(String(maxLength=50)))
