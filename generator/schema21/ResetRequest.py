from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ResetRequest(Object, additionalProperties=False):

    type: str = Property(String(enum=['Immediate', 'OnIdle', 'ImmediateAndResume'], description='This contains the type of reset that the Charging Station or EVSE should perform.\r\n'), required=True)

    evseId: Maybe[int] = Property(Integer(minimum=0.0, description='This contains the ID of a specific EVSE that needs to be reset, instead of the entire Charging Station.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
