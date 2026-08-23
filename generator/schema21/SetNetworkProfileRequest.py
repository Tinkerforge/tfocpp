from typing import Any

from statham.schema.constants import Maybe
from statham.schema.elements import Boolean, Integer, Object, String
from statham.schema.property import Property


class CustomDataType(Object):
    """This class does not get 'AdditionalProperties = false' in the schema generation, so it can be extended with arbitrary JSON properties to allow adding custom data."""

    vendorId: str = Property(String(maxLength=255), required=True)


class APNType(Object, additionalProperties=False):
    """Collection of configuration data needed to make a data-connection over a cellular network.

NOTE: When asking a GSM modem to dial in, it is possible to specify which mobile operator should be used. This can be done with the mobile country code (MCC) in combination with a mobile network code (MNC). Example: If your preferred network is Vodafone Netherlands, the MCC=204 and the MNC=04 which means the key PreferredNetwork = 20404 Some modems allows to specify a preferred network, which means, if this network is not available, a different network is used. If you specify UseOnlyPreferredNetwork and this network is not available, the modem will not dial in.
"""

    apn: str = Property(String(maxLength=2000, description='The Access Point Name as an URL.\r\n'), required=True)

    apnUserName: Maybe[str] = Property(String(maxLength=50, description='APN username.\r\n'))

    apnPassword: Maybe[str] = Property(String(maxLength=64, description='*(2.1)* APN Password.\r\n'))

    simPin: Maybe[int] = Property(Integer(description='SIM card pin code.\r\n'))

    preferredNetwork: Maybe[str] = Property(String(maxLength=6, description='Preferred network, written as MCC and MNC concatenated. See note.\r\n'))

    useOnlyPreferredNetwork: bool = Property(Boolean(default=False, description='Default: false. Use only the preferred Network, do\r\nnot dial in when not available. See Note.\r\n'))

    apnAuthentication: str = Property(String(enum=['PAP', 'CHAP', 'NONE', 'AUTO'], description='Authentication method.\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class VPNType(Object, additionalProperties=False):
    """VPN Configuration settings
"""

    server: str = Property(String(maxLength=2000, description='VPN Server Address\r\n'), required=True)

    user: str = Property(String(maxLength=50, description='VPN User\r\n'), required=True)

    group: Maybe[str] = Property(String(maxLength=50, description='VPN group.\r\n'))

    password: str = Property(String(maxLength=64, description='*(2.1)* VPN Password.\r\n'), required=True)

    key: str = Property(String(maxLength=255, description='VPN shared secret.\r\n'), required=True)

    type: str = Property(String(enum=['IKEv2', 'IPSec', 'L2TP', 'PPTP'], description='Type of VPN\r\n'), required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class NetworkConnectionProfileType(Object, additionalProperties=False):
    """The NetworkConnectionProfile defines the functional and technical parameters of a communication link.
"""

    apn: Maybe[APNType] = Property(APNType)

    ocppVersion: Maybe[str] = Property(String(enum=['OCPP12', 'OCPP15', 'OCPP16', 'OCPP20', 'OCPP201', 'OCPP21'], description='*(2.1)* This field is ignored, since the OCPP version to use is determined during the websocket handshake. The field is only kept for backwards compatibility with the OCPP 2.0.1 JSON schema.\r\n'))

    ocppInterface: str = Property(String(enum=['Wired0', 'Wired1', 'Wired2', 'Wired3', 'Wireless0', 'Wireless1', 'Wireless2', 'Wireless3', 'Any'], description='Applicable Network Interface. Charging Station is allowed to use a different network interface to connect if the given one does not work.\r\n'), required=True)

    ocppTransport: str = Property(String(enum=['SOAP', 'JSON'], description='Defines the transport protocol (e.g. SOAP or JSON). Note: SOAP is not supported in OCPP 2.x, but is supported by earlier versions of OCPP.\r\n'), required=True)

    messageTimeout: int = Property(Integer(description='Duration in seconds before a message send by the Charging Station via this network connection times-out.\r\nThe best setting depends on the underlying network and response times of the CSMS.\r\nIf you are looking for a some guideline: use 30 seconds as a starting point.\r\n'), required=True)

    ocppCsmsUrl: str = Property(String(maxLength=2000, description='URL of the CSMS(s) that this Charging Station communicates with, without the Charging Station identity part. +\r\nThe SecurityCtrlr.Identity field is appended to _ocppCsmsUrl_ to provide the full websocket URL.\r\n'), required=True)

    securityProfile: int = Property(Integer(minimum=0.0, description='This field specifies the security profile used when connecting to the CSMS with this NetworkConnectionProfile.\r\n'), required=True)

    identity: Maybe[str] = Property(String(maxLength=48, description='*(2.1)* Charging Station identity to be used as the basic authentication username.\r\n'))

    basicAuthPassword: Maybe[str] = Property(String(maxLength=64, description='*(2.1)* BasicAuthPassword to use for security profile 1 or 2.\r\n'))

    vpn: Maybe[VPNType] = Property(VPNType)

    customData: Maybe[CustomDataType] = Property(CustomDataType)


class SetNetworkProfileRequest(Object, additionalProperties=False):

    configurationSlot: int = Property(Integer(description='Slot in which the configuration should be stored.\r\n'), required=True)

    connectionData: NetworkConnectionProfileType = Property(NetworkConnectionProfileType, required=True)

    customData: Maybe[CustomDataType] = Property(CustomDataType)
