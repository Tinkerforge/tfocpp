from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class GetDiagnosticsResponse(Object, additionalProperties=False):

    fileName: Maybe[str] = Property(String(maxLength=255))
