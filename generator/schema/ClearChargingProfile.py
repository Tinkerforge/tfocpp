from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class ClearChargingProfileRequest(Object, additionalProperties=False):

    id: Maybe[int] = Property(Integer())

    connectorId: Maybe[int] = Property(Integer())

    chargingProfilePurpose: Maybe[str] = Property(String(enum=['ChargePointMaxProfile', 'TxDefaultProfile', 'TxProfile']))

    stackLevel: Maybe[int] = Property(Integer())
