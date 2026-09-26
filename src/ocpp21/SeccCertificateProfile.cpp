#include "SeccCertificateProfile.h"

#include <cstring>

namespace SeccCertificateProfile {

namespace {

constexpr unsigned Boolean = 0x01;
constexpr unsigned Integer = 0x02;
constexpr unsigned BitString = 0x03;
constexpr unsigned OctetString = 0x04;
constexpr unsigned ObjectIdentifier = 0x06;
constexpr unsigned UtcTime = 0x17;
constexpr unsigned GeneralizedTime = 0x18;
constexpr unsigned Sequence = 0x30;
constexpr unsigned Set = 0x31;
constexpr unsigned KeyIdentifier = 0x80;
constexpr unsigned Uri = 0x86;
constexpr unsigned ExplicitVersion = 0xa0;
constexpr unsigned ExplicitExtensions = 0xa3;

constexpr unsigned SubjectKeyIdentifier = 14;
constexpr unsigned KeyUsage = 15;
constexpr unsigned BasicConstraints = 19;
constexpr unsigned CrlDistributionPoints = 31;
constexpr unsigned AuthorityKeyIdentifier = 35;
constexpr unsigned ExtendedKeyUsage = 37;

constexpr unsigned CommonName = 3;
constexpr unsigned CountryName = 6;
constexpr unsigned OrganizationName = 10;

// OID contents, without the ASN.1 tag and length.
constexpr char DomainComponentOid[] = "\x09\x92\x26\x89\x93\xf2\x2c\x64\x01\x19";
constexpr char EcPublicKeyOid[] = "\x2a\x86\x48\xce\x3d\x02\x01";
constexpr char EcdsaSha256Oid[] = "\x2a\x86\x48\xce\x3d\x04\x03\x02";
constexpr char EcdsaSha512Oid[] = "\x2a\x86\x48\xce\x3d\x04\x03\x04";
constexpr char Ed448Oid[] = "\x2b\x65\x71";
constexpr char Secp256r1Oid[] = "\x2a\x86\x48\xce\x3d\x03\x01\x07";
constexpr char Secp521r1Oid[] = "\x2b\x81\x04\x00\x23";
constexpr char AuthorityInfoAccessOid[] = "\x2b\x06\x01\x05\x05\x07\x01\x01";
constexpr char OcspOid[] = "\x2b\x06\x01\x05\x05\x07\x30\x01";
constexpr char ServerAuthOid[] = "\x2b\x06\x01\x05\x05\x07\x03\x01";

template<size_t N>
Der oid_view(const char (&oid)[N])
{
    return {reinterpret_cast<const uint8_t *>(oid), N - 1};
}

struct Certificate {
    Der serial;
    Der signature;
    Der issuer;
    Der subject;
    Der spki;
    Der extensions;
};

bool valid_serial(Der serial)
{
    if (serial.n == 0 || serial.n > 20 || (serial.p[0] & 0x80) != 0) {
        return false;
    }

    // A leading zero is necessary only to keep the integer positive.
    if (serial.n > 1 && serial.p[0] == 0 && (serial.p[1] & 0x80) == 0) {
        return false;
    }

    for (size_t i = 0; i < serial.n; ++i) {
        if (serial.p[i] != 0) {
            return true;
        }
    }
    return false;
}

bool valid_time_encoding(Der &validity)
{
    if (validity.n == 0) {
        return false;
    }

    const unsigned tag = validity.p[0];
    Der date;
    if ((tag != UtcTime && tag != GeneralizedTime) || !validity.take(tag, date)) {
        return false;
    }

    const size_t expected_length = tag == UtcTime ? 13 : 15;
    if (date.n != expected_length || date.p[date.n - 1] != 'Z') {
        return false;
    }
    for (size_t i = 0; i + 1 < date.n; ++i) {
        if (date.p[i] < '0' || date.p[i] > '9') {
            return false;
        }
    }

    // RFC 5280 requires UTCTime for 1950..2049. AMD1 removed the
    // original profile's GeneralizedTime-only wording.
    if (tag == GeneralizedTime) {
        unsigned year = 0;
        for (unsigned i = 0; i < 4; ++i) {
            year = year * 10 + date.p[i] - '0';
        }
        if (year >= 1950 && year < 2050) {
            return false;
        }
    }
    return true;
}

bool parse_certificate(Der input, Certificate &cert)
{
    Der outer;
    Der tbs;
    Der signature_algorithm;
    Der signature_bits;
    if (!input.take(Sequence, outer) || input.n != 0 ||
        !outer.take(Sequence, tbs) ||
        !outer.take(Sequence, signature_algorithm) ||
        !outer.take(BitString, signature_bits) || outer.n != 0) {
        return false;
    }

    Der version;
    Der version_number;
    if (!tbs.take(ExplicitVersion, version) ||
        !version.take(Integer, version_number) || version.n != 0 ||
        !version_number.is("\x02", 1)) {
        return false;
    }

    Der validity;
    if (!tbs.take(Integer, cert.serial) ||
        !tbs.take(Sequence, cert.signature) ||
        !signature_algorithm.same(cert.signature) ||
        !tbs.take(Sequence, cert.issuer) ||
        !tbs.take(Sequence, validity) ||
        !tbs.take(Sequence, cert.subject) ||
        !tbs.take(Sequence, cert.spki)) {
        return false;
    }

    if (!valid_serial(cert.serial) ||
        !valid_time_encoding(validity) || !valid_time_encoding(validity) || validity.n != 0) {
        return false;
    }

    Der extensions_wrapper;
    return tbs.take(ExplicitExtensions, extensions_wrapper) && tbs.n == 0 &&
           extensions_wrapper.take(Sequence, cert.extensions) && extensions_wrapper.n == 0;
}

bool parse_issuer(Der input, Certificate &issuer)
{
    // Extract only the issuer-binding data. In particular, do not apply the
    // delivered-certificate profile to an independently trusted anchor.
    Der outer;
    Der tbs;
    Der ignored;
    if (!input.take(Sequence, outer) || !outer.take(Sequence, tbs)) {
        return false;
    }
    if (tbs.n != 0 && tbs.p[0] == ExplicitVersion && !tbs.take(ExplicitVersion, ignored)) {
        return false;
    }

    if (!tbs.take(Integer, ignored) ||        // serialNumber
        !tbs.take(Sequence, ignored) ||       // signature
        !tbs.take(Sequence, ignored) ||       // issuer
        !tbs.take(Sequence, ignored) ||       // validity
        !tbs.take(Sequence, issuer.subject) ||
        !tbs.take(Sequence, issuer.spki)) {
        return false;
    }

    if (tbs.n != 0) {
        Der extensions_wrapper;
        if (!tbs.take(ExplicitExtensions, extensions_wrapper) ||
            !extensions_wrapper.take(Sequence, issuer.extensions) ||
            extensions_wrapper.n != 0 || tbs.n != 0) {
            return false;
        }
    }
    return true;
}

// Absence returns an empty value. Malformed encodings and duplicate matches
// return false; callers decide whether the requested extension is mandatory.
bool find_extension(Der extensions, Der wanted_oid, Der &value, bool &critical)
{
    bool found = false;
    value = {};
    critical = false;
    while (extensions.n != 0) {
        Der entry;
        Der oid;
        Der bytes;
        if (!extensions.take(Sequence, entry) || !entry.take(ObjectIdentifier, oid)) {
            return false;
        }

        bool entry_critical = false;
        if (entry.n != 0 && entry.p[0] == Boolean) {
            Der flag;
            // DER omits the default FALSE; explicit TRUE must be 0xff.
            if (!entry.take(Boolean, flag) || !flag.is("\xff", 1)) {
                return false;
            }
            entry_critical = true;
        }
        if (!entry.take(OctetString, bytes) || entry.n != 0) {
            return false;
        }

        if (oid.same(wanted_oid)) {
            if (found) {
                return false;
            }
            found = true;
            value = bytes;
            critical = entry_critical;
        }
    }
    return true;
}

bool find_extension(Der extensions, unsigned id, Der &value, bool &critical)
{
    const uint8_t oid[] = {0x55, 0x1d, static_cast<uint8_t>(id)};
    return find_extension(extensions, {oid, sizeof(oid)}, value, critical);
}

bool find_name_field(Der name, Der wanted_oid, Der &value)
{
    bool found = false;
    value = {};
    while (name.n != 0) {
        Der rdn;
        if (!name.take(Set, rdn)) {
            return false;
        }
        while (rdn.n != 0) {
            Der attribute;
            Der oid;
            Der text;
            if (!rdn.take(Sequence, attribute) ||
                !attribute.take(ObjectIdentifier, oid) || attribute.n == 0) {
                return false;
            }
            const unsigned string_tag = attribute.p[0];
            if (!attribute.take(string_tag, text) || attribute.n != 0) {
                return false;
            }
            if (oid.same(wanted_oid)) {
                if (found || text.n == 0 || memchr(text.p, 0, text.n) != nullptr) {
                    return false;
                }
                found = true;
                value = text;
            }
        }
    }
    return true;
}

bool find_name_field(Der name, unsigned id, Der &value)
{
    const uint8_t oid[] = {0x55, 0x04, static_cast<uint8_t>(id)};
    return find_name_field(name, {oid, sizeof(oid)}, value);
}

bool check_subject(Der subject, Der requested_subject, bool leaf, bool iso20)
{
    Der common_name;
    Der organization;
    Der country;
    Der role;
    if (!find_name_field(subject, CommonName, common_name) || common_name.n == 0 ||
        !find_name_field(subject, OrganizationName, organization) || organization.n == 0 ||
        !find_name_field(subject, CountryName, country) ||
        !find_name_field(subject, oid_view(DomainComponentOid), role)) {
        return false;
    }

    if (country.n != 0) {
        if ((country.n != 2) || (country.p[0] < 'A') || (country.p[0] > 'Z') || (country.p[1] < 'A') || (country.p[1] > 'Z')) {
            return false;
        }
    }
    if (leaf) {
        Der expected_common_name;
        if ((country.n == 0) || !find_name_field(requested_subject, CommonName, expected_common_name) || !common_name.same(expected_common_name)) {
            return false;
        }
    }

    if (leaf || iso20) {
        const char *required_role = iso20 ? "CSO" : "CPO";
        if (role.n < 3 || memcmp(role.p + role.n - 3, required_role, 3) != 0) {
            return false;
        }
        // ISO-20 permits a CA-supplied prefix; ISO-2 requires exactly CPO.
        if (!iso20 && role.n != 3) {
            return false;
        }
    }
    return true;
}

bool check_algorithms(const Certificate &cert, bool iso20, bool &ed448, Der &public_key)
{
    Der signature = cert.signature;
    Der signature_oid;
    Der spki = cert.spki;
    Der key_algorithm;
    Der key_oid;
    if (!signature.take(ObjectIdentifier, signature_oid) || signature.n != 0 ||
        !spki.take(Sequence, key_algorithm) || !key_algorithm.take(ObjectIdentifier, key_oid) ||
        !spki.take(BitString, public_key) || spki.n != 0 ||
        public_key.n == 0 || public_key.p[0] != 0) {
        return false;
    }

    // The key identifier hashes exclude the BIT STRING's unused-bits octet.
    ++public_key.p;
    --public_key.n;
    ed448 = key_oid.same(oid_view(Ed448Oid));

    bool signature_ok;
    if (iso20) {
        signature_ok = signature_oid.same(oid_view(EcdsaSha512Oid)) || signature_oid.same(oid_view(Ed448Oid));
    } else {
        signature_ok = signature_oid.same(oid_view(EcdsaSha256Oid));
    }
    if (!signature_ok) {
        return false;
    }

    if (ed448) {
        return iso20 && key_algorithm.n == 0 && public_key.n == 57;
    }

    Der curve_oid;
    if (!key_oid.same(oid_view(EcPublicKeyOid)) ||!key_algorithm.take(ObjectIdentifier, curve_oid) || key_algorithm.n != 0) {
        return false;
    }
    return curve_oid.same(iso20 ? oid_view(Secp521r1Oid) : oid_view(Secp256r1Oid));
}

bool check_basic_constraints(Der extensions, bool leaf, bool lower_ca)
{
    Der value;
    Der constraints;
    bool critical;
    if (!find_extension(extensions, BasicConstraints, value, critical) || !critical || !value.take(Sequence, constraints) || value.n != 0) {
        return false;
    }
    if (leaf) {
        // cA defaults to FALSE and pathLenConstraint is absent.
        return constraints.n == 0;
    }

    Der ca;
    Der path_length;
    if (!constraints.take(Boolean, ca) || !ca.is("\xff", 1) || !constraints.take(Integer, path_length) || constraints.n != 0 || path_length.n != 1 || path_length.p[0] > 1) {
        return false;
    }
    return !lower_ca || path_length.p[0] == 0;
}

bool check_key_usage(Der extensions, bool leaf, bool ed448, bool iso20)
{
    Der value;
    Der bits;
    bool critical;
    if (!find_extension(extensions, KeyUsage, value, critical) || !critical || !value.take(BitString, bits) || value.n != 0 || bits.n < 2 || bits.n > 3 || bits.p[0] > 7) {
        return false;
    }

    const unsigned unused_bits_mask = (1u << bits.p[0]) - 1;
    if ((bits.p[bits.n - 1] & unused_bits_mask) != 0) {
        return false;
    }

    // These masks use the DER bit order, not the ASN.1 bit numbers.
    constexpr unsigned DigitalSignature = 0x80;
    constexpr unsigned ContentCommitment = 0x40;
    constexpr unsigned KeyEncipherment = 0x20;
    constexpr unsigned DataEncipherment = 0x10;
    constexpr unsigned KeyAgreement = 0x08;
    constexpr unsigned KeyCertSign = 0x04;
    constexpr unsigned CrlSign = 0x02;

    unsigned usage = bits.p[1];
    if (bits.n == 3) {
        usage |= unsigned(bits.p[2]) << 8;
    }
    unsigned required = leaf ? DigitalSignature : KeyCertSign;
    if (leaf && !ed448) {
        required |= KeyAgreement;
    }
    unsigned allowed = DigitalSignature | ContentCommitment | KeyEncipherment | KeyAgreement;
    if (!leaf) {
        allowed |= KeyCertSign;
    }
    if (!iso20) {
        allowed |= DataEncipherment;
        if (!leaf) {
            allowed |= CrlSign;
        }
    }

    return (usage & required) == required && (usage & ~allowed) == 0;
}

bool check_extended_key_usage(Der extensions, bool leaf, bool iso20)
{
    Der value;
    bool critical;
    if (!find_extension(extensions, ExtendedKeyUsage, value, critical)) {
        return false;
    }

    if (iso20) {
        if ((!leaf && value.n != 0) || (leaf && (value.n == 0 || !critical))) {
            return false;
        }
    }

    if (!leaf || value.n == 0) {
        return true;
    }

    Der purposes;
    if (!value.take(Sequence, purposes) || value.n != 0) {
        return false;
    }

    bool server_auth = false;
    while (purposes.n != 0) {
        Der purpose;
        if (!purposes.take(ObjectIdentifier, purpose)) {
            return false;
        }
        const bool server = purpose.same(oid_view(ServerAuthOid));
        if (iso20 && (!server || server_auth)) {
            return false;
        }
        server_auth |= server;
    }

    return server_auth;
}

bool check_subject_key_identifier(Der extensions, Der public_key, bool iso20, Sha1 sha1)
{
    Der value;
    bool critical;
    if (!find_extension(extensions, SubjectKeyIdentifier, value, critical) || critical ||
        (iso20 && value.n == 0)) {
        return false;
    }
    if (!iso20 || value.n == 0) {
        return true;
    }

    Der identifier;
    uint8_t digest[20];
    if (!value.take(OctetString, identifier) || value.n != 0 || !sha1(public_key, digest)) {
        return false;
    }

    // RFC 5280 method 1: full SHA-1. Method 2: low 60 bits, prefixed by 0100.
    uint8_t short_identifier[8];
    memcpy(short_identifier, digest + 12, sizeof(short_identifier));
    short_identifier[0] = (short_identifier[0] & 0x0f) | 0x40;
    return identifier.same({digest, sizeof(digest)}) ||
           identifier.same({short_identifier, sizeof(short_identifier)});
}

bool check_authority_key_identifier(Der extensions, Der issuer_extensions, bool iso20)
{
    Der value;
    bool critical;
    if (!find_extension(extensions, AuthorityKeyIdentifier, value, critical) || critical || (iso20 && value.n == 0)) {
        return false;
    }
    if (!iso20 || value.n == 0) {
        return true;
    }

    Der authority;
    Der identifier;
    if (!value.take(Sequence, authority) || value.n != 0 || !authority.take(KeyIdentifier, identifier) || authority.n != 0) {
        return false;
    }

    Der issuer_value;
    Der issuer_identifier;
    if (!find_extension(issuer_extensions, SubjectKeyIdentifier, issuer_value, critical) || !issuer_value.take(OctetString, issuer_identifier) || issuer_value.n != 0) {
        return false;
    }
    return identifier.same(issuer_identifier);
}

bool usable_ocsp_uri(Der uri)
{
    if (uri.n > 255) {
        return false;
    }

    size_t prefix_length;
    if (uri.n > 7 && memcmp(uri.p, "http://", 7) == 0) {
        prefix_length = 7;
    } else if (uri.n > 8 && memcmp(uri.p, "https://", 8) == 0) {
        prefix_length = 8;
    } else {
        return false;
    }

    const uint8_t authority_start = uri.p[prefix_length];
    if (authority_start == '/' || authority_start == ':' || authority_start == '?') {
        return false;
    }
    for (size_t i = 0; i < uri.n; ++i) {
        const uint8_t c = uri.p[i];
        if (c <= 0x20 || c >= 0x7f || c == '#' || c == '@' || c == '\\') {
            return false;
        }
    }
    return true;
}

bool check_ocsp_source(Der extensions)
{
    Der value;
    bool critical;
    if (!find_extension(extensions, oid_view(AuthorityInfoAccessOid), value, critical) || critical) {
        return false;
    }

    Der descriptions;
    Der description;
    Der method;
    Der uri;
    if (!value.take(Sequence, descriptions) || value.n != 0 ||
        !descriptions.take(Sequence, description) || descriptions.n != 0 ||
        !description.take(ObjectIdentifier, method) || !method.same(oid_view(OcspOid)) ||
        !description.take(Uri, uri) || description.n != 0) {
        return false;
    }
    return usable_ocsp_uri(uri);
}

bool check_revocation_sources(Der extensions, bool iso20)
{
    if (!iso20) {
        return true;
    }

    // The ISO-20 SECC profile requires OCSP and excludes CRLDistributionPoints.
    Der crl;
    bool critical;
    return check_ocsp_source(extensions) && find_extension(extensions, CrlDistributionPoints, crl, critical) && crl.n == 0;
}

} // namespace

bool Der::take(unsigned tag, Der &out)
{
    if (n < 2 || p[0] != tag) {
        return false;
    }

    size_t header_length = 2;
    size_t content_length = p[1];
    if ((content_length & 0x80) != 0) {
        const size_t length_octets = content_length & 0x7f;
        if (length_octets == 0 || length_octets > sizeof(size_t) ||
            n < 2 + length_octets || p[2] == 0) {
            return false;
        }
        content_length = 0;
        for (size_t i = 0; i < length_octets; ++i) {
            content_length = (content_length << 8) | p[2 + i];
        }
        header_length += length_octets;
        if (content_length < 128) {
            return false;
        }
    }
    if (content_length > n - header_length) {
        return false;
    }

    out = {p + header_length, content_length};
    p += header_length + content_length;
    n -= header_length + content_length;
    return true;
}

bool Der::same(Der other) const
{
    return n == other.n && (n == 0 || memcmp(p, other.p, n) == 0);
}

bool Der::is(const char *bytes, size_t size) const
{
    return same({reinterpret_cast<const uint8_t *>(bytes), size});
}

bool check(Der raw, Der issuer_raw, Der requested_subject,
           bool leaf, bool lower_ca, bool iso20, Sha1 sha1)
{
    Certificate cert;
    Certificate issuer;
    if (!parse_certificate(raw, cert) || !parse_issuer(issuer_raw, issuer)) {
        return false;
    }
    if (!cert.issuer.same(issuer.subject) || !check_subject(cert.subject, requested_subject, leaf, iso20)) {
        return false;
    }

    bool ed448;
    Der public_key;
    if (!check_algorithms(cert, iso20, ed448, public_key)) {
        return false;
    }

    return check_basic_constraints(cert.extensions, leaf, lower_ca) &&
           check_key_usage(cert.extensions, leaf, ed448, iso20) &&
           check_extended_key_usage(cert.extensions, leaf, iso20) &&
           check_subject_key_identifier(cert.extensions, public_key, iso20, sha1) &&
           check_authority_key_identifier(cert.extensions, issuer.extensions, iso20) &&
           check_revocation_sources(cert.extensions, iso20);
}

} // namespace SeccCertificateProfile
