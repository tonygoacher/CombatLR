# CombatLR GPS Simulator

Windows/WPF .NET 8 application for feeding simulated NMEA GPS data to an ESP32.

## Use

1. Open `CombatLR_GpsSimulator.sln` or the `.csproj` in Visual Studio 2022.
2. Build/run.
3. `map.bmp` and `map.json` are included as the supplied CombatLR map.
4. Select the ESP32's USB/TTL serial COM port and baud rate.
5. Click **Connect**, then **Start GPS**.
6. Click or drag the green GPS marker on the map.
7. The **NMEA output** list shows every sentence written to the serial port.

The simulator emits `$GPRMC` and `$GPGGA` at 5 Hz.

Use a 3.3 V TTL serial interface between the PC and ESP32; do not connect RS-232 voltage levels directly to the ESP32.
