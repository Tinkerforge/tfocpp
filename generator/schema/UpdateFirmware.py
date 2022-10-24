from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class UpdateFirmwareRequest(Object, additionalProperties=False):

    location: str = Property(String(format='uri'), required=True)

    retries: Maybe[int] = Property(Integer())

    retrieveDate: str = Property(String(format='date-time'), required=True)

    retryInterval: Maybe[int] = Property(Integer())
