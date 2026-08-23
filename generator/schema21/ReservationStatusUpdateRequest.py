from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ReservationStatusUpdateRequest(Object, additionalProperties=False):

    reservationId: int = Property(Integer(minimum=0.0, description='The ID of the reservation.\r\n'), required=True)

    reservationUpdateStatus: str = Property(String(enum=['Expired', 'Removed', 'NoTransaction'], description='The updated reservation status.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
