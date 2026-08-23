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


class CsChargingProfiles(Object, additionalProperties=False):

    chargingProfileId: int = Property(Integer(), required=True)

    transactionId: Maybe[int] = Property(Integer())

    stackLevel: int = Property(Integer(minimum=0), required=True)

    chargingProfilePurpose: str = Property(String(enum=['ChargePointMaxProfile', 'TxDefaultProfile', 'TxProfile']), required=True)

    chargingProfileKind: str = Property(String(enum=['Absolute', 'Recurring', 'Relative']), required=True)

    recurrencyKind: Maybe[str] = Property(String(enum=['Daily', 'Weekly']))

    validFrom: Maybe[str] = Property(String(format='date-time'))

    validTo: Maybe[str] = Property(String(format='date-time'))

    chargingSchedule: ChargingSchedule = Property(ChargingSchedule, required=True)


class SetChargingProfileRequest(Object, additionalProperties=False):

    connectorId: int = Property(Integer(), required=True)

    csChargingProfiles: CsChargingProfiles = Property(CsChargingProfiles, required=True)
