from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
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


class GetCertificateStatusResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Failed'], description='This indicates whether the charging station was able to retrieve the OCSP certificate status.\r\n'), required=True)

    statusInfo: Maybe[StatusInfoType] = Property(StatusInfoType)

    ocspResult: Maybe[str] = Property(String(maxLength=18000, description='*(2.1)* OCSPResponse class as defined in &lt;&lt;ref-ocpp_security_24, IETF RFC 6960&gt;&gt;. DER encoded (as defined in &lt;&lt;ref-ocpp_security_24, IETF RFC 6960&gt;&gt;), and then base64 encoded. MAY only be omitted when status is not Accepted. +\r\nThe minimum supported length is 18000. If a longer _ocspResult_ is supported, then the supported length must be communicated in variable OCPPCommCtrlr.FieldLength[ "GetCertificateStatusResponse.ocspResult" ].\r\n\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
