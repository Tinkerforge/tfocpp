from statham.schema.elements import Object, String
from statham.schema.property import Property


class ChangeConfigurationResponse(Object, additionalProperties=False):

    status: str = Property(String(enum=['Accepted', 'Rejected', 'RebootRequired', 'NotSupported']), required=True)
