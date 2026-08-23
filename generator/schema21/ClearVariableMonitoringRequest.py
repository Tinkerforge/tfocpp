from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ClearVariableMonitoringRequest(Object, additionalProperties=False):

    id: List[int] = Property(Array(Integer(minimum=0.0), additionalItems=False, minItems=1, description='List of the monitors to be cleared, identified by there Id.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
