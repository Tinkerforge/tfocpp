from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class EVSEType(Object, additionalProperties=False):
    """Electric Vehicle Supply Equipment
"""

    id: int = Property(Integer(minimum=0.0, description='EVSE Identifier. This contains a number (&gt; 0) designating an EVSE of the Charging Station.\r\n'), required=True)

    connectorId: Maybe[int] = Property(Integer(minimum=0.0, description='An id to designate a specific connector (on an EVSE) by connector index number.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ChangeAvailabilityRequest(Object, additionalProperties=False):

    evse: Maybe[EVSEType] = Property(EVSEType)

    operationalStatus: str = Property(String(enum=['Inoperative', 'Operative'], description='This contains the type of availability change that the Charging Station should perform.\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
