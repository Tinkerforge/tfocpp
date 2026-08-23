from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class StatusNotificationRequest(Object, additionalProperties=False):

    timestamp: str = Property(String(format='date-time', description='The time for which the status is reported.\r\n'), required=True)

    connectorStatus: str = Property(String(enum=['Available', 'Occupied', 'Reserved', 'Unavailable', 'Faulted'], description='This contains the current status of the Connector.\r\n'), required=True)

    evseId: int = Property(Integer(minimum=0.0, description='The id of the EVSE to which the connector belongs for which the the status is reported.\r\n'), required=True)

    connectorId: int = Property(Integer(minimum=0.0, description='The id of the connector within the EVSE for which the status is reported.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
