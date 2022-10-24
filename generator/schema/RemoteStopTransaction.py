from statham.schema.elements import Integer, Object
from statham.schema.property import Property


class RemoteStopTransactionRequest(Object, additionalProperties=False):

    transactionId: int = Property(Integer(), required=True)
