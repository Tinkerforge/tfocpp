from statham.schema.elements import Object, String
from statham.schema.property import Property


class ChangeConfigurationRequest(Object, additionalProperties=False):

    key: str = Property(String(maxLength=50), required=True)

    value: str = Property(String(maxLength=500), required=True)
