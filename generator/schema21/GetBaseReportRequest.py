from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class GetBaseReportRequest(Object, additionalProperties=False):

    requestId: int = Property(Integer(description='The Id of the request.\r\n'), required=True)

    reportBase: str = Property(String(enum=['ConfigurationInventory', 'FullInventory', 'SummaryInventory'], description='This field specifies the report base.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
