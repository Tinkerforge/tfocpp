from statham.schema.elements import Object, String
from statham.schema.property import Property


class AuthorizeRequest(Object, additionalProperties=False):

    idTag: str = Property(String(maxLength=20), required=True)
