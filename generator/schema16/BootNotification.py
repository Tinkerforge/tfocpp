from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class BootNotificationRequest(Object, additionalProperties=False):

    chargePointVendor: str = Property(String(maxLength=20), required=True)

    chargePointModel: str = Property(String(maxLength=20), required=True)

    chargePointSerialNumber: Maybe[str] = Property(String(maxLength=25))

    chargeBoxSerialNumber: Maybe[str] = Property(String(maxLength=25))

    firmwareVersion: Maybe[str] = Property(String(maxLength=50))

    iccid: Maybe[str] = Property(String(maxLength=20))

    imsi: Maybe[str] = Property(String(maxLength=20))

    meterType: Maybe[str] = Property(String(maxLength=25))

    meterSerialNumber: Maybe[str] = Property(String(maxLength=25))
