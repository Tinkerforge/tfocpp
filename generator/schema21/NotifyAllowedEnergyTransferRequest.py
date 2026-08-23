from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class NotifyAllowedEnergyTransferRequest(Object, additionalProperties=False):

    transactionId: str = Property(String(maxLength=36, description='The transaction for which the allowed energy transfer is allowed.\r\n'), required=True)

    allowedEnergyTransfer: List[str] = Property(Array(String(enum=['AC_single_phase', 'AC_two_phase', 'AC_three_phase', 'DC', 'AC_BPT', 'AC_BPT_DER', 'AC_DER', 'DC_BPT', 'DC_ACDP', 'DC_ACDP_BPT', 'WPT']), additionalItems=False, minItems=1, description='Modes of energy transfer that are accepted by CSMS.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
