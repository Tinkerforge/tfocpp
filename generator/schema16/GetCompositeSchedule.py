from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class GetCompositeScheduleRequest(Object, additionalProperties=False):

    connectorId: int = Property(Integer(), required=True)

    duration: int = Property(Integer(), required=True)

    chargingRateUnit: Maybe[str] = Property(String(enum=['A', 'W']))
