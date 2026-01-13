from statham.schema.constants import Maybe
from statham.schema.elements import Object, String
from statham.schema.property import Property


class SignedMeterValueType(Object, additionalProperties=False):

    signedMeterData: str = Property(String(maxLength=2500), required=True)

    signingMethod: str = Property(String(enum=['', 'ECDSA-secp192k1-SHA256', 'ECDSA-secp256k1-SHA256', 'ECDSA-secp192r1-SHA256', 'ECDSA-secp256r1-SHA256', 'ECDSA-brainpool256r1-SHA256', 'ECDSA-secp384r1-SHA256', 'ECDSA-brainpool384r1-SHA256']), required=True)

    encodingMethod: str = Property(String(enum=['OCMF', 'EDL']), required=True)

    publicKey: str = Property(String(maxLength=2500), required=True)


class ExtSMV(Object):

    signedMeterValueType: Maybe[SignedMeterValueType] = Property(SignedMeterValueType)
