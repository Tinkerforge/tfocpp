from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class Get15118EVCertificateRequest(Object, additionalProperties=False):

    iso15118SchemaVersion: str = Property(String(maxLength=50, description='Schema version currently used for the 15118 session between EV and Charging Station. Needed for parsing of the EXI stream by the CSMS.\r\n\r\n'), required=True)

    action: str = Property(String(enum=['Install', 'Update'], description='Defines whether certificate needs to be installed or updated.\r\n'), required=True)

    exiRequest: str = Property(String(maxLength=11000, description='*(2.1)* Raw CertificateInstallationReq request from EV, Base64 encoded. +\r\nExtended to support ISO 15118-20 certificates. The minimum supported length is 11000. If a longer _exiRequest_ is supported, then the supported length must be communicated in variable OCPPCommCtrlr.FieldLength[ "Get15118EVCertificateRequest.exiRequest" ].\r\n'), required=True)

    maximumContractCertificateChains: Maybe[int] = Property(Integer(minimum=0.0, description='*(2.1)* Absent during ISO 15118-2 session. Required during ISO 15118-20 session. +\r\nMaximum number of contracts that EV wants to install.\r\n'))

    prioritizedEMAIDs: Maybe[List[str]] = Property(Array(String(maxLength=255), additionalItems=False, minItems=1, maxItems=8, description='*(2.1)*  Absent during ISO 15118-2 session. Optional during ISO 15118-20 session. List of EMAIDs for which contract certificates must be requested first, in case there are more certificates than allowed by _maximumContractCertificateChains_.\r\n'))

    customData: Maybe[CustomDataType] = Property(CustomDataType)
