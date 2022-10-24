from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Boolean, Object, String
from statham.schema.property import Property


class ConfigurationKeyItem(Object, additionalProperties=False):

    key: str = Property(String(maxLength=50), required=True)

    readonly: bool = Property(Boolean(), required=True)

    value: Maybe[str] = Property(String(maxLength=500))


class GetConfigurationResponse(Object, additionalProperties=False):

    configurationKey: Maybe[List[ConfigurationKeyItem]] = Property(Array(ConfigurationKeyItem))

    unknownKey: Maybe[List[str]] = Property(Array(String(maxLength=50)))
