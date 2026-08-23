from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
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


class PublishFirmwareStatusNotificationRequest(Object, additionalProperties=False):

    status: str = Property(String(enum=['Idle', 'DownloadScheduled', 'Downloading', 'Downloaded', 'Published', 'DownloadFailed', 'DownloadPaused', 'InvalidChecksum', 'ChecksumVerified', 'PublishFailed'], description='This contains the progress status of the publishfirmware\r\ninstallation.\r\n'), required=True)

    location: Maybe[List[str]] = Property(Array(String(maxLength=2000), additionalItems=False, minItems=1, description='Required if status is Published. Can be multiple URI’s, if the Local Controller supports e.g. HTTP, HTTPS, and FTP.\r\n'))

    requestId: Maybe[int] = Property(Integer(minimum=0.0, description='The request id that was\r\nprovided in the\r\nPublishFirmwareRequest which\r\ntriggered this action.\r\n'))

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
