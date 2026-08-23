from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ClearedChargingLimitRequest(Object, additionalProperties=False):

    chargingLimitSource: str = Property(String(maxLength=20, description='Source of the charging limit. Allowed values defined in Appendix as ChargingLimitSourceEnumStringType.\r\n'), required=True)

    evseId: Maybe[int] = Property(Integer(minimum=0.0, description='EVSE Identifier.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
