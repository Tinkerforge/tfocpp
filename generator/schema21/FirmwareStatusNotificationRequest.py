from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class StatusInfoType(Object, additionalProperties=False):
    """Element providing more information about the status.
"""

    reasonCode: str = Property(String(maxLength=20, description='A predefined code for the reason why the status is returned in this response. The string is case-insensitive.\r\n'), required=True)

    additionalInfo: Maybe[str] = Property(String(maxLength=1024, description='Additional text to provide detailed information.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class FirmwareStatusNotificationRequest(Object, additionalProperties=False):

    status: str = Property(String(enum=['Downloaded', 'DownloadFailed', 'Downloading', 'DownloadScheduled', 'DownloadPaused', 'Idle', 'InstallationFailed', 'Installing', 'Installed', 'InstallRebooting', 'InstallScheduled', 'InstallVerificationFailed', 'InvalidSignature', 'SignatureVerified'], description='This contains the progress status of the firmware installation.\r\n'), required=True)

    requestId: Maybe[int] = Property(Integer(description='The request id that was provided in the\r\nUpdateFirmwareRequest that started this firmware update.\r\nThis field is mandatory, unless the message was triggered by a TriggerMessageRequest AND there is no firmware update ongoing.\r\n'))

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
