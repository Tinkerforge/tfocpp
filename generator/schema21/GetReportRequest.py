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


class ComponentVariableType(Object, additionalProperties=False):
    """Class to report components, variables and variable attributes and characteristics.
"""

    component: ComponentType = Property(ComponentType, required=True)

    variable: Maybe[VariableType] = Property(VariableType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class GetReportRequest(Object, additionalProperties=False):

    componentVariable: Maybe[List[ComponentVariableType]] = Property(Array(ComponentVariableType, additionalItems=False, minItems=1))

    requestId: int = Property(Integer(description='The Id of the request.\r\n'), required=True)

    componentCriteria: Maybe[List[str]] = Property(Array(String(enum=['Active', 'Available', 'Enabled', 'Problem']), additionalItems=False, minItems=1, maxItems=4, description='This field contains criteria for components for which a report is requested\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
