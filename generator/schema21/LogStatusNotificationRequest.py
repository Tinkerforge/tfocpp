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


class LogStatusNotificationRequest(Object, additionalProperties=False):

    status: str = Property(String(enum=['BadMessage', 'Idle', 'NotSupportedOperation', 'PermissionDenied', 'Uploaded', 'UploadFailure', 'Uploading', 'AcceptedCanceled'], description='This contains the status of the log upload.\r\n'), required=True)

    requestId: Maybe[int] = Property(Integer(description='The request id that was provided in GetLogRequest that started this log upload. This field is mandatory,\r\nunless the message was triggered by a TriggerMessageRequest AND there is no log upload ongoing.\r\n'))

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
