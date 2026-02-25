from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import (
    Array,
    Boolean,
    Integer,
    Number,
    Object,
    String,
)
from statham.schema.property import Property


class LC(Object, additionalProperties=False):

    LN: Maybe[str] = Property(String(maxLength=250))

    LI: Maybe[int] = Property(Integer())

    LR: float = Property(Number(), required=True)

    LU: str = Property(String(enum=['mOhm', 'uOhm']), required=True)


class RDItem(Object, additionalProperties=False):

    TM: str = Property(String(pattern='^[0-9]{4}-[0-1][0-9]-[0-3][0-9]T[0-2][0-9]:[0-5][0-9]:[0-5][0-9],[0-9]{3}[+-][0-2][0-9][0-5][0-9] [UISR]$', minLength=30, maxLength=30), required=True)

    TX: Maybe[str] = Property(String(enum=['B', 'C', 'X', 'E', 'L', 'R', 'A', 'P', 'S', 'T']))

    RV: float = Property(Number(), required=True)

    RI: Maybe[str] = Property(String())

    RU: str = Property(String(enum=['kWh', 'Wh', 'mOhm', 'uOhm']), required=True)

    RT: Maybe[str] = Property(String(enum=['AC', 'DC']))

    CL: Maybe[float] = Property(Number())

    EF: Maybe[str] = Property(String(enum=['', 'E', 't', 'Et', 'tE']))

    ST: str = Property(String(enum=['N', 'G', 'T', 'D', 'R', 'M', 'X', 'I', 'O', 'S', 'E', 'F']), required=True)


class ExtOCMFRequest(Object):

    FV: Maybe[str] = Property(String())

    GI: Maybe[str] = Property(String(maxLength=41))

    GS: Maybe[str] = Property(String(maxLength=25))

    GV: Maybe[str] = Property(String(maxLength=50))

    PG: str = Property(String(), required=True)

    MV: Maybe[str] = Property(String())

    MM: Maybe[str] = Property(String())

    MS: str = Property(String(), required=True)

    MF: Maybe[str] = Property(String())

    IS: bool = Property(Boolean(), required=True)

    IL: Maybe[str] = Property(String(enum=['NONE', 'HEARSAY', 'TRUSTED', 'VERIFIED', 'CERTIFIED', 'SECURE', 'MISMATCH', 'INVALID', 'OUTDATED', 'UNKNOWN']))

    IF: Maybe[List[str]] = Property(Array(String(enum=['RFID_NONE', 'RFID_PLAIN', 'RFID_RELATED', 'RFID_PSK', 'OCPP_NONE', 'OCPP_RS', 'OCPP_AUTH', 'OCPP_RS_TLS', 'OCPP_AUTH_TLS', 'OCPP_CACHE', 'OCPP_WHITELIST', 'OCPP_CERTIFIED', 'ISO15118_NONE', 'ISO15118_PNC', 'PLMN_NONE', 'PLMN_RING', 'PLMN_SMS']), maxItems=4))

    IT: str = Property(String(enum=['NONE', 'DENIED', 'UNDEFINED', 'ISO14443', 'ISO15693', 'EMAID', 'EVCCID', 'EVCOID', 'ISO7812', 'CARD_TXN_NR', 'CENTRAL', 'CENTRAL_1', 'CENTRAL_2', 'LOCAL', 'LOCAL_1', 'LOCAL_2', 'PHONE_NUMBER', 'KEY_CODE']), required=True)

    ID: Maybe[str] = Property(String())

    TT: Maybe[str] = Property(String(maxLength=250))

    CF: Maybe[str] = Property(String(maxLength=25))

    LC: Maybe[LC] = Property(LC)

    CT: Maybe[str] = Property(String(enum=['EVSEID', 'CBIDC']))

    CI: Maybe[str] = Property(String(maxLength=20))

    RD: List[RDItem] = Property(Array(RDItem), required=True)

    WTF_connector_id: Maybe[int] = Property(Integer(minimum=0))

    WTF_unix_time: Maybe[str] = Property(String(format='date-time'))

    WTF_signature_encoding: Maybe[str] = Property(String(enum=['BASE16', 'BASE64']))
