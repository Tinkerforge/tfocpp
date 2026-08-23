from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ClearTariffsRequest(Object, additionalProperties=False):

    tariffIds: Maybe[List[str]] = Property(Array(String(maxLength=60), additionalItems=False, minItems=1, description='List of tariff Ids to clear. When absent clears all tariffs at _evseId_.\r\n\r\n'))

    evseId: Maybe[int] = Property(Integer(minimum=0.0, description='When present only clear tariffs matching _tariffIds_ at EVSE _evseId_.\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
