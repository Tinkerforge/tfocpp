from statham.schema.constants import Maybe
from statham.schema.elements import Boolean, Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class NotifyCustomerInformationRequest(Object, additionalProperties=False):

    data: str = Property(String(maxLength=512, description='(Part of) the requested data. No format specified in which the data is returned. Should be human readable.\r\n'), required=True)

    tbc: bool = Property(Boolean(default=False, description='“to be continued” indicator. Indicates whether another part of the monitoringData follows in an upcoming notifyMonitoringReportRequest message. Default value when omitted is false.\r\n'))

    seqNo: int = Property(Integer(minimum=0.0, description='Sequence number of this message. First message starts at 0.\r\n'), required=True)

    generatedAt: str = Property(String(format='date-time', description=' Timestamp of the moment this message was generated at the Charging Station.\r\n'), required=True)

    requestId: int = Property(Integer(minimum=0.0, description='The Id of the request.\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
