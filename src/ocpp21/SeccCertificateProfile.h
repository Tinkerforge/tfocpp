// ISO 15118-2 Annex F / ISO 15118-20 AMD1 Tables B.5/B.6.
#pragma once

#include <cstddef>
#include <cstdint>

namespace SeccCertificateProfile {

// Borrowed DER view. take() returns an element's contents and advances this
// view past the element. Neither view owns or modifies the underlying bytes.
struct Der {
    const uint8_t *p = nullptr;
    size_t n = 0;

    bool take(unsigned tag, Der &out);
    bool same(Der other) const;
    bool is(const char *bytes, size_t size) const;
};

// The backend supplies SHA-1 solely for RFC 5280 key identifier derivation.
// The callback writes a 20-byte digest and returns whether hashing succeeded.
using Sha1 = bool (*)(Der input, uint8_t *digest);

// Check one delivered certificate. The backend separately verifies the path
// signatures, validity and pending-key ownership. requested_subject contains
// the contents of the pending CSR's Name SEQUENCE, without its tag/length.
// lower_ca identifies a CA below another delivered CA. The independently
// trusted issuer may be an anchor and is not itself subjected to this profile.
bool check(Der certificate, Der issuer, Der requested_subject, bool leaf, bool lower_ca, bool iso20, Sha1 sha1);

} // namespace SeccCertificateProfile
