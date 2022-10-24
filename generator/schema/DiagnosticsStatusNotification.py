from statham.schema.elements import Object, String
from statham.schema.property import Property


class DiagnosticsStatusNotificationRequest(Object, additionalProperties=False):

    status: str = Property(String(enum=['Idle', 'Uploaded', 'UploadFailed', 'Uploading']), required=True)
