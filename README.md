# CinderWick

Custom ESP32 firmware flasher, written in C. Reimplements the esptool serial protocol. SLIP framing, ROM bootloader handshake,
and flash write commands without depending on esptool itself.

Why? partly spite (esptool and idf.py are just bothersome to work with), mostly because reverse-engineering a serial protocol is just a fun way to spend time.

Currently about to support ROM bootloader-only flashing. Stub loader uploader is planned for follow-up.
