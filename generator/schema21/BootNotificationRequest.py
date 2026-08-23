from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ModemType(Object, additionalProperties=False):
    """Defines parameters required for initiating and maintaining wireless communication with other devices.
"""

    iccid: Maybe[str] = Property(String(maxLength=20, description='This contains the ICCID of the modem’s SIM card.\r\n'))

    imsi: Maybe[str] = Property(String(maxLength=20, description='This contains the IMSI of the modem’s SIM card.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ChargingStationType(Object, additionalProperties=False):
    """The physical system where an Electrical Vehicle (EV) can be charged.
"""

    serialNumber: Maybe[str] = Property(String(maxLength=25, description='Vendor-specific device identifier.\r\n'))

    model: str = Property(String(maxLength=20, description='Defines the model of the device.\r\n'), required=True)

    modem: Maybe[ModemType] = Property(ModemType)

    vendorName: str = Property(String(maxLength=50, description='Identifies the vendor (not necessarily in a unique manner).\r\n'), required=True)

    firmwareVersion: Maybe[str] = Property(String(maxLength=50, description='This contains the firmware version of the Charging Station.\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class BootNotificationRequest(Object, additionalProperties=False):

    chargingStation: ChargingStationType = Property(ChargingStationType, required=True)

    reason: str = Property(String(enum=['ApplicationReset', 'FirmwareUpdate', 'LocalReset', 'PowerUp', 'RemoteReset', 'ScheduledReset', 'Triggered', 'Unknown', 'Watchdog'], description='This contains the reason for sending this message to the CSMS.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
