# leashLink handle_c3 firmware

Target: ESP32-C3 handle node.

This project follows the leashLink documents:

- Handle node: ESP32-C3
- Collar node: ESP32-C5
- Local safety loop: tension + collar acceleration -> lock leash -> collar feedback -> cloud event
- Realtime link: ESP-NOW first, BLE for pairing/discovery, MQTT for cloud upload

## Build

```powershell
idf.py set-target esp32c3
idf.py build
idf.py flash monitor
```

## Bring-up order

1. Check serial log and board power.
2. Verify HX711 raw tension changes with load.
3. Verify servo lock/unlock on an unloaded mechanism.
4. Verify ESPNOW collar telemetry with a test sender.
5. Enable Wi-Fi/MQTT credentials and test telemetry/event upload.

## Important safety note

Do not test the lock mechanism on a real pet until the mechanical release, current limiting, and manual override have been validated.

