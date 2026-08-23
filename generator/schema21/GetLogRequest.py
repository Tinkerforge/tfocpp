from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class LogParametersType(Object, additionalProperties=False):
    """Generic class for the configuration of logging entries.
"""

    remoteLocation: str = Property(String(maxLength=2000, description='The URL of the location at the remote system where the log should be stored.\r\n'), required=True)

    oldestTimestamp: Maybe[str] = Property(String(format='date-time', description='This contains the date and time of the oldest logging information to include in the diagnostics.\r\n'))

    latestTimestamp: Maybe[str] = Property(String(format='date-time', description='This contains the date and time of the latest logging information to include in the diagnostics.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class GetLogRequest(Object, additionalProperties=False):

    log: LogParametersType = Property(LogParametersType, required=True)

    logType: str = Property(String(enum=['DiagnosticsLog', 'SecurityLog', 'DataCollectorLog'], description='This contains the type of log file that the Charging Station\r\nshould send.\r\n'), required=True)

    requestId: int = Property(Integer(description='The Id of this request\r\n'), required=True)

    retries: Maybe[int] = Property(Integer(minimum=0.0, description='This specifies how many times the Charging Station must retry to upload the log before giving up. If this field is not present, it is left to Charging Station to decide how many times it wants to retry. If the value is 0, it means: no retries.\r\n'))

    retryInterval: Maybe[int] = Property(Integer(description='The interval in seconds after which a retry may be attempted. If this field is not present, it is left to Charging Station to decide how long to wait between attempts.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
