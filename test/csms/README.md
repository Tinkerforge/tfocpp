# CSMS integration tests

Integration tests that run the `ocpp21_linux` host (and once `ocpp16_linux`)
against a real CSMS and assert on both the host log and the CSMS REST API.
The first supported CSMS is evtivity, other CSMS can follow.

## Setup

1. Start the evtivity stack (docker compose, with demo seed so that RFID
   tokens exist) and make sure the OCPP, TLS and API ports are reachable.
2. Build the host binaries: `make ocpp21_linux ocpp16_linux`
3. Optional: `cp .env.example .env` and adjust. Without a `.env` the
   defaults match a local evtivity on the standard ports.
4. Install [uv](https://docs.astral.sh/uv/). `openssl` and `docker` must be
   in the PATH.

## Run

```
./run.sh            # everything
./run.sh -k security -v
```

The tests create their own stations (prefix `tfocpp-test-`) via the REST API
and delete them afterwards (evtivity implements delete as a soft delete, the
blocked stations stay visible in the operator UI but can not connect). A
pricing group `tfocpp-test-free` with a free tariff is created once and
assigned to every test station: evtivity's demo payment gate randomly
simulates payment failures and remote stops those sessions, the free tariff
bypasses it. Tests marked `docker` need local docker access to
the CSMS containers (certificate extraction, mTLS client cert registration,
offline simulation) and are skipped without it. TLS test certificates are
extracted from the running ocpp container unless provided via the env, the
copies in the evtivity checkout can be older than the ones baked into the
image.

The certificate management tests act as the PKI: a local test CA
(`testca.py`) signs the CSRs from SignCertificate requests and the signed
chain is delivered via the CertificateSigned command. The V2G flows need
evtivity's Plug and Charge feature flag, the suite enables it via the
settings API (the OCPP container caches the flag for up to a minute, the
test retries). The OCSP tests (`test_ocsp.py`) run against a minimal
scripted CSMS (`minicsms.py`) instead of evtivity, which forwards a JSON
body instead of a DER encoded OCSP request to responders. The OCSP
responses are generated locally with `openssl ocsp`.

## Notes

* The full suite takes a few minutes, dominated by reconnect intervals and
  the offline continuation test.
* Each test runs its host in a fresh temporary working directory, so
  persisted state (`<name>.sec21`) does not leak between tests.
* The 1.6 boot test uses the hardcoded simulator identity `warp2-X8D` and
  leaves that station in place if it already existed.
