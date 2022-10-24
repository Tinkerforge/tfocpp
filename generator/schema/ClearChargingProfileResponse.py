from statham.schema.elements import Object, String
from statham.schema.property import Property


class ClearChargingProfileResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Unknown']), required=True)
