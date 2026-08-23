from statham.schema.constants import Maybe
from statham.schema.elements import Number, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class CostUpdatedRequest(Object, additionalProperties=False):

    totalCost: float = Property(Number(description='Current total cost, based on the information known by the CSMS, of the transaction including taxes. In the currency configured with the configuration Variable: [&lt;&lt;configkey-currency, Currency&gt;&gt;]\r\n\r\n'), required=True)

    transactionId: str = Property(String(maxLength=36, description='Transaction Id of the transaction the current cost are asked for.\r\n\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
