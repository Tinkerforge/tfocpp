from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class StatusNotificationRequest(Object, additionalProperties=False):

    connectorId: int = Property(Integer(minimum=0), required=True)

    errorCode: str = Property(String(enum=['ConnectorLockFailure', 'EVCommunicationError', 'GroundFailure', 'HighTemperature', 'InternalError', 'LocalListConflict', 'NoError', 'OtherError', 'OverCurrentFailure', 'PowerMeterFailure', 'PowerSwitchFailure', 'ReaderFailure', 'ResetFailure', 'UnderVoltage', 'OverVoltage', 'WeakSignal']), required=True)

    info: Maybe[str] = Property(String(maxLength=50))

    status: str = Property(String(enum=['Available', 'Preparing', 'Charging', 'SuspendedEV', 'SuspendedEVSE', 'Finishing', 'Reserved', 'Unavailable', 'Faulted']), required=True)

    timestamp: Maybe[str] = Property(String(format='date-time'))

    vendorId: Maybe[str] = Property(String(maxLength=255))

    vendorErrorCode: Maybe[str] = Property(String(maxLength=50))
