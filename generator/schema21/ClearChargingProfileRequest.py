from statham.schema.constants import Maybe
from statham.schema.elements import Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ClearChargingProfileType(Object, additionalProperties=False):
    """A ClearChargingProfileType is a filter for charging profiles to be cleared by ClearChargingProfileRequest.

"""

    evseId: Maybe[int] = Property(Integer(minimum=0.0, description='Specifies the id of the EVSE for which to clear charging profiles. An evseId of zero (0) specifies the charging profile for the overall Charging Station. Absence of this parameter means the clearing applies to all charging profiles that match the other criteria in the request.\r\n\r\n'))

    chargingProfilePurpose: Maybe[str] = Property(String(enum=['ChargingStationExternalConstraints', 'ChargingStationMaxProfile', 'TxDefaultProfile', 'TxProfile', 'PriorityCharging', 'LocalGeneration'], description='Specifies to purpose of the charging profiles that will be cleared, if they meet the other criteria in the request.\r\n'))

    stackLevel: Maybe[int] = Property(Integer(minimum=0.0, description='Specifies the stackLevel for which charging profiles will be cleared, if they meet the other criteria in the request.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class ClearChargingProfileRequest(Object, additionalProperties=False):

    chargingProfileId: Maybe[int] = Property(Integer(description='The Id of the charging profile to clear.\r\n'))

    chargingProfileCriteria: Maybe[ClearChargingProfileType] = Property(ClearChargingProfileType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
