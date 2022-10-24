from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class GetDiagnosticsRequest(Object, additionalProperties=False):

    location: str = Property(String(format='uri'), required=True)

    retries: Maybe[int] = Property(Integer())

    retryInterval: Maybe[int] = Property(Integer())

    startTime: Maybe[str] = Property(String(format='date-time'))

    stopTime: Maybe[str] = Property(String(format='date-time'))
