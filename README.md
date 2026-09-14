# ESP32-C3 Motorrad-Tacho mit GC9A01

## Projektstatus

Minimalistischer Motorrad-Tacho auf Basis eines **ESP32-C3** und eines
runden **240×240 Pixel GC9A01 LCD**.

Aktueller Stand:

-   ESP32-C3
-   GC9A01 LCD, 240×240 Pixel
-   SPI über `esp_lcd`
-   ESP-IDF 5.5.x
-   LittleFS für die austauschbare Hintergrundgrafik
-   RGB565
-   eigener Grafik-/Gauge-Renderer
-   kein LVGL
-   Hintergrund: `assets/tacho.rgb565`
-   dynamischer Zeiger und digitale Geschwindigkeitsanzeige
-   Testbetrieb mit automatisch laufender Geschwindigkeit

## Hardware

### GC9A01 LCD

  Funktion                           ESP32-C3
  ----------- -------------------------------
  MOSI                                 GPIO 7
  SCLK                                 GPIO 6
  CS                                  GPIO 10
  DC                                   GPIO 2
  RESET                                GPIO 3
  MISO                        nicht verwendet
  Backlight     nicht angeschlossen / GPIO -1

SPI-Takt aktuell: **27 MHz**

Display:

-   240 × 240 Pixel
-   16 Bit / Pixel
-   RGB565

## Software-Struktur

``` text
tacho/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── assets/
│   └── tacho.rgb565
└── main/
    ├── CMakeLists.txt
    ├── main.c
    ├── display.c
    ├── display.h
    ├── gfx.c
    ├── gfx.h
    ├── gauge.c
    ├── gauge.h
    ├── storage.c
    └── storage.h
```

### Abhängigkeiten

``` yaml
dependencies:
  espressif/esp_lcd_gc9a01: "2.0.3"
  joltwallet/littlefs: "1.22.3"
```

### Grafik

Der Framebuffer ist 240×240 RGB565:

``` text
240 × 240 × 2 = 115200 Bytes
```

`gfx.c` stellt Pixel, Linien, Kreise, gefüllte Kreise und Bögen bereit.

### Tacho

`gauge.c` zeichnet:

-   äußeren Ring
-   Skala
-   Haupt- und Nebenstriche
-   roten Bereich
-   Zahlen
-   Zeiger
-   Zeiger-Nabe
-   digitale Geschwindigkeit

Aktueller Bereich:

``` text
0 … 180 km/h
```

## Hintergrundgrafik

Datei:

``` text
assets/tacho.rgb565
```

Exakte Größe:

``` text
115200 Bytes
```

Die Datei muss als **RGB565-Rohdaten** vorliegen.

Im Programm:

``` text
/littlefs/tacho.rgb565
```

Das `assets`-Verzeichnis wird beim Build als LittleFS-Image eingebunden.

## Partitionierung

Aktuelles 2-MB-Layout:

``` csv
# Name,     Type, SubType,   Offset,   Size, Flags
nvs,        data, nvs,       0x9000,   0x6000,
phy_init,   data, phy,      0xf000,   0x1000,
factory,    app,  factory, 0x10000,  0x100000,
storage,    data, littlefs, 0x110000, 0xF0000,
```

Wenn der Chip physisch 4 MB besitzt, das Firmware-Image aber einen
2-MB-Header enthält, kann folgende Meldung erscheinen:

``` text
Detected size(4096k) larger than size in binary image header(2048k).
Using size in binary image header.
```

Das ist nicht automatisch ein Hardwarefehler.

## Build

Aktuelle Umgebung:

``` text
ESP-IDF 5.5.x
```

Beispiel:

``` text
ESP-IDF v5.5.5-316-g1a1a5aa6513-dirty
```

Bauen:

``` bash
idf.py build
```

Flashen:

``` bash
idf.py flash
```

Monitor:

``` bash
idf.py monitor
```

Alles zusammen:

``` bash
idf.py build flash monitor
```

## LittleFS

`main/CMakeLists.txt` enthält:

``` cmake
idf_component_register(
    SRCS
        "main.c"
        "display.c"
        "gfx.c"
        "gauge.c"
        "storage.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_lcd
        esp_lcd_gc9a01
        driver
        littlefs
)

littlefs_create_partition_image(
    storage
    ../assets
    FLASH_IN_PROJECT
)
```

## Testbetrieb

Ohne echten Geschwindigkeitssensor erzeugt `main.c` aktuell einen
Testwert.

Die Geschwindigkeit läuft zwischen:

``` text
0 km/h
180 km/h
```

hin und her.

Der vollständige 240×240-Framebuffer wird anschließend mit

``` c
esp_lcd_panel_draw_bitmap(...)
```

an das Display übertragen.

## Zeigerspuren vermeiden

Da der Hintergrund statisch ist und Zeiger sowie digitale Anzeige
dynamisch sind, müssen die Bereiche der vorherigen Darstellung vor dem
Neuzeichnen aus dem Hintergrund wiederhergestellt werden.

Sonst entstehen:

-   Zeigerspuren
-   sich aufbauende Flächen
-   ein immer dicker werdender Zeiger

Der aktuelle Ansatz stellt deshalb nur die benötigten dynamischen
Bereiche aus der LittleFS-Hintergrunddatei wieder her, statt pro Frame
den kompletten Hintergrund neu zu laden.

## Technische Grenzen

Der vollständige RGB565-Framebuffer benötigt ca. **115,2 KB RAM**.

Ein zweiter kompletter Hintergrundpuffer wird deshalb vermieden.

Bei 240×240 RGB565 müssen pro vollständigem Frame 115200 Bytes
übertragen werden. Bei 27 MHz liegt die theoretische reine
SPI-Übertragungszeit bereits bei ungefähr 34 ms pro Frame. Ein echtes
dauerhaftes 60-FPS-Rendering mit vollständigem Frame-Transfer ist daher
nicht ohne weitere Optimierung zu erwarten.

## Nächste Schritte

1.  echter Geschwindigkeitseingang
2.  Drehzahlsignal
3.  Drehzahlskala bzw. kombinierte Anzeige
4.  saubere DMA-Synchronisation
5.  Optimierung des Background-Restore
6.  konfigurierbare Skala und Redline
7.  Backlight-Steuerung
8.  weitere austauschbare Zifferblätter
