from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Object, String
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


class ClearTariffsResultType(Object, additionalProperties=False):

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    tariffId: Maybe[str] = Property(String(maxLength=60, description='Id of tariff for which _status_ is reported. If no tariffs were found, then this field is absent, and _status_ will be `NoTariff`.\r\n'))

    status: str = Property(String(enum=['Accepted', 'Rejected', 'NoTariff']), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ClearTariffsResponse(Object, additionalProperties=False):

    clearTariffsResult: List[ClearTariffsResultType] = Property(Array(ClearTariffsResultType, additionalItems=False, minItems=1), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
