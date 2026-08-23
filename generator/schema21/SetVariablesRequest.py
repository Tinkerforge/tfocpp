from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class EVSEType(Object, additionalProperties=False):
    """Electric Vehicle Supply Equipment
"""

    id: int = Property(Integer(minimum=0.0, description='EVSE Identifier. This contains a number (&gt; 0) designating an EVSE of the Charging Station.\r\n'), required=True)

    connectorId: Maybe[int] = Property(Integer(minimum=0.0, description='An id to designate a specific connector (on an EVSE) by connector index number.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ComponentType(Object, additionalProperties=False):
    """A physical or logical component
"""

    evse: Maybe[EVSEType] = Property(EVSEType)

    name: str = Property(String(maxLength=50, description='Name of the component. Name should be taken from the list of standardized component names whenever possible. Case Insensitive. strongly advised to use Camel Case.\r\n'), required=True)

    instance: Maybe[str] = Property(String(maxLength=50, description='Name of instance in case the component exists as multiple instances. Case Insensitive. strongly advised to use Camel Case.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class VariableType(Object, additionalProperties=False):
    """Reference key to a component-variable.
"""

    name: str = Property(String(maxLength=50, description='Name of the variable. Name should be taken from the list of standardized variable names whenever possible. Case Insensitive. strongly advised to use Camel Case.\r\n'), required=True)

    instance: Maybe[str] = Property(String(maxLength=50, description='Name of instance in case the variable exists as multiple instances. Case Insensitive. strongly advised to use Camel Case.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SetVariableDataType(Object, additionalProperties=False):

    attributeType: str = Property(String(default='Actual', enum=['Actual', 'Target', 'MinSet', 'MaxSet'], description='Type of attribute: Actual, Target, MinSet, MaxSet. Default is Actual when omitted.\r\n'))

    attributeValue: str = Property(String(maxLength=2500, description='Value to be assigned to attribute of variable.\r\nThis value is allowed to be an empty string ("").\r\n\r\nThe Configuration Variable &lt;&lt;configkey-configuration-value-size,ConfigurationValueSize&gt;&gt; can be used to limit SetVariableData.attributeValue and VariableCharacteristics.valuesList. The max size of these values will always remain equal. \r\n'), required=True)

    component: ComponentType = Property(ComponentType, required=True)

    variable: VariableType = Property(VariableType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SetVariablesRequest(Object, additionalProperties=False):

    setVariableData: List[SetVariableDataType] = Property(Array(SetVariableDataType, additionalItems=False, minItems=1), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
