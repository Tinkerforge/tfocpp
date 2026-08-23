from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class GetLocalListVersionResponse(Object, additionalProperties=False):

    versionNumber: int = Property(Integer(description='This contains the current version number of the local authorization list in the Charging Station.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
