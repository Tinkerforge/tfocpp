TFOCPP - Tinkerforge OCPP 1.6J Charge Point Implementation
==========================================================

This is the OCPP implementation used in Tinkerforge WARP Chargers. See http://github.com/Tinkerforge/esp32-firmware for the integration of this library in the WARP Charger's firmware.

This library implements OCPP 1.6J and currently supports the Core and Smart Charging profiles. Support for other profiles will follow in the future as needed.

To integrate this library into a charger firmware, you have to implement the platform interface defined in src/ocpp/Platform.h. See src/platforms and the http://github.com/Tinkerforge/esp32-firmware OCPP module for examples.

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
