from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class UnlockConnectorRequest(Object, additionalProperties=False):

    evseId: int = Property(Integer(minimum=0.0, description='This contains the identifier of the EVSE for which a connector needs to be unlocked.\r\n'), required=True)

    connectorId: int = Property(Integer(minimum=0.0, description='This contains the identifier of the connector that needs to be unlocked.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
