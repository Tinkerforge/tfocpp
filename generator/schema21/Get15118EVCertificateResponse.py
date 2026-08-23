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


class Get15118EVCertificateResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Failed'], description='Indicates whether the message was processed properly.\r\n'), required=True)

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    exiResponse: str = Property(String(maxLength=17000, description='*(2/1)* Raw CertificateInstallationRes response for the EV, Base64 encoded. +\r\nExtended to support ISO 15118-20 certificates. The minimum supported length is 17000. If a longer _exiResponse_ is supported, then the supported length must be communicated in variable OCPPCommCtrlr.FieldLength[ "Get15118EVCertificateResponse.exiResponse" ].\r\n\r\n'), required=True)

    remainingContracts: Maybe[int] = Property(Integer(minimum=0.0, description='*(2.1)* Number of contracts that can be retrieved with additional requests.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
