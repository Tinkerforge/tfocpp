from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class DataTransferResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected', 'UnknownMessageId', 'UnknownVendorId']), required=True)

    data: Maybe[str] = Property(String())
