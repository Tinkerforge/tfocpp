from statham.schema.elements import Object, String
from statham.schema.property import Property


class RemoteStopTransactionResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected']), required=True)
