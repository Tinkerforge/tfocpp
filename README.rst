TFOCPP - Tinkerforge OCPP 1.6J Charge Point Implementation
==========================================================

This is still in beta state. Do not use yet!

Repository Content
------------------

generator/:
 * Python script that generates the OCPP message (de-)serializer

lib/:
 * Dependencies that are required to build TFOCPP. Currently:
 * ArduinoJson: https://github.com/bblanchon/ArduinoJson
 * mongoose (Used for the Linux and test platforms): https://github.com/cesanta/mongoose

spec/:
  * OCPP 1.6J Spec, Errata and (patched!) schemas.

src/:
  * lib/: Further dependencies used for the implementation and the ESP32 platform (which is why they are here)
  * ocpp/: Main OCPP implementation
  * platforms: Platforms to run this on Linux, in the tests and in a ESP32 firmware

test/:
  * Tests for spec conformance
