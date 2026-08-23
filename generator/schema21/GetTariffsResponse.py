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


class TariffAssignmentType(Object, additionalProperties=False):
    """Shows assignment of tariffs to EVSE or IdToken.
"""

    tariffId: str = Property(String(maxLength=60, description='Tariff id.\r\n'), required=True)

    tariffKind: str = Property(String(enum=['DefaultTariff', 'DriverTariff'], description='Kind of tariff (driver/default)\r\n'), required=True)

    validFrom: Maybe[str] = Property(String(format='date-time', description='Date/time when this tariff become active.\r\n'))

    evseIds: Maybe[List[int]] = Property(Array(Integer(minimum=0.0), additionalItems=False, minItems=1))

    idTokens: Maybe[List[str]] = Property(Array(String(maxLength=255), additionalItems=False, minItems=1, description='IdTokens related to tariff\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class GetTariffsResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected', 'NoTariff'], description='Status of operation\r\n'), required=True)

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    tariffAssignments: Maybe[List[TariffAssignmentType]] = Property(Array(TariffAssignmentType, additionalItems=False, minItems=1))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
