# Navigation / ST7789VDisplay integration

The production ST7789VDisplay is now the only display implementation.

NavigationDisplayTest.cpp is only a test harness. It supplies spoofed
navigation data to the production display through:

    SetNavigationPlayerPosition(GameCoordinate, heading)

The display remains responsible for:
    - map rendering
    - player-centred camera
    - zoom
    - heading-up rotation
    - player marker

The test harness is responsible only for:
    - creating the spoof navigation source
    - converting GPS to GameCoordinate
    - moving the spoof player
    - feeding position and heading to ST7789VDisplay
