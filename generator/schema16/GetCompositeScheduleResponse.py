from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Number, Object, String
from statham.schema.property import Property


class ChargingSchedulePeriodItem(Object, additionalProperties=False):

    startPeriod: int = Property(Integer(), required=True)

    limit: float = Property(Number(multipleOf=0.1), required=True)

    numberPhases: Maybe[int] = Property(Integer())


class ChargingSchedule(Object, additionalProperties=False):

    duration: Maybe[int] = Property(Integer())

    startSchedule: Maybe[str] = Property(String(format='date-time'))

    chargingRateUnit: str = Property(String(enum=['A', 'W']), required=True)

    chargingSchedulePeriod: List[ChargingSchedulePeriodItem] = Property(Array(ChargingSchedulePeriodItem), required=True)

    minChargingRate: Maybe[float] = Property(Number(multipleOf=0.1))


class GetCompositeScheduleResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected']), required=True)

    connectorId: Maybe[int] = Property(Integer())

    scheduleStart: Maybe[str] = Property(String(format='date-time'))

    chargingSchedule: Maybe[ChargingSchedule] = Property(ChargingSchedule)
