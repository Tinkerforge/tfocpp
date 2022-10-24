from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class DataTransferRequest(Object, additionalProperties=False):

    vendorId: str = Property(String(maxLength=255), required=True)

    messageId: Maybe[str] = Property(String(maxLength=50))

    data: Maybe[str] = Property(String())
