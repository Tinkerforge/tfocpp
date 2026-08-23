from statham.schema.elements import Object, String
from statham.schema.property import Property


class FirmwareStatusNotificationRequest(Object, additionalProperties=False):

    status: str = Property(String(enum=['Downloaded', 'DownloadFailed', 'Downloading', 'Idle', 'InstallationFailed', 'Installing', 'Installed']), required=True)
