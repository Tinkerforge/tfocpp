from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class GetCompositeScheduleRequest(Object, additionalProperties=False):

    duration: int = Property(Integer(description='Length of the requested schedule in seconds.\r\n\r\n'), required=True)

    chargingRateUnit: Maybe[str] = Property(String(enum=['W', 'A'], description='Can be used to force a power or current profile.\r\n\r\n'))

    evseId: int = Property(Integer(minimum=0.0, description='The ID of the EVSE for which the schedule is requested. When evseid=0, the Charging Station will calculate the expected consumption for the grid connection.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
