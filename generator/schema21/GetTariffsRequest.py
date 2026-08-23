from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class GetTariffsRequest(Object, additionalProperties=False):

    evseId: int = Property(Integer(minimum=0.0, description='EVSE id to get tariff from. When _evseId_ = 0, this gets tariffs from all EVSEs.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
