from typing import Any, List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class ChargingProfileCriterionType(Object, additionalProperties=False):
    """A ChargingProfileCriterionType is a filter for charging profiles to be selected by a GetChargingProfilesRequest.

"""

    chargingProfilePurpose: Maybe[str] = Property(String(enum=['ChargingStationExternalConstraints', 'ChargingStationMaxProfile', 'TxDefaultProfile', 'TxProfile', 'PriorityCharging', 'LocalGeneration'], description='Defines the purpose of the schedule transferred by this profile\r\n'))

    stackLevel: Maybe[int] = Property(Integer(minimum=0.0, description='Value determining level in hierarchy stack of profiles. Higher values have precedence over lower values. Lowest level is 0.\r\n'))

    chargingProfileId: Maybe[List[int]] = Property(Array(Integer(), additionalItems=False, minItems=1, description='List of all the chargingProfileIds requested. Any ChargingProfile that matches one of these profiles will be reported. If omitted, the Charging Station SHALL not filter on chargingProfileId. This field SHALL NOT contain more ids than set in &lt;&lt;configkey-charging-profile-entries,ChargingProfileEntries.maxLimit&gt;&gt;\r\n\r\n'))

    chargingLimitSource: Maybe[List[str]] = Property(Array(String(maxLength=20), additionalItems=False, minItems=1, maxItems=4, description='For which charging limit sources, charging profiles SHALL be reported. If omitted, the Charging Station SHALL not filter on chargingLimitSource. Values defined in Appendix as ChargingLimitSourceEnumStringType.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class GetChargingProfilesRequest(Object, additionalProperties=False):

    requestId: int = Property(Integer(description='Reference identification that is to be used by the Charging Station in the &lt;&lt;reportchargingprofilesrequest, ReportChargingProfilesRequest&gt;&gt; when provided.\r\n'), required=True)

    evseId: Maybe[int] = Property(Integer(minimum=0.0, description='For which EVSE installed charging profiles SHALL be reported. If 0, only charging profiles installed on the Charging Station itself (the grid connection) SHALL be reported. If omitted, all installed charging profiles SHALL be reported. +\r\nReported charging profiles SHALL match the criteria in field _chargingProfile_.\r\n'))

    chargingProfile: ChargingProfileCriterionType = Property(ChargingProfileCriterionType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
