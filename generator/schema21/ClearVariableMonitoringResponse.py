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


class ClearMonitoringResultType(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected', 'NotFound'], description='Result of the clear request for this monitor, identified by its Id.\r\n\r\n'), required=True)

    id: int = Property(Integer(minimum=0.0, description='Id of the monitor of which a clear was requested.\r\n\r\n'), required=True)

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ClearVariableMonitoringResponse(Object, additionalProperties=False):

    clearMonitoringResult: List[ClearMonitoringResultType] = Property(Array(ClearMonitoringResultType, additionalItems=False, minItems=1), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
