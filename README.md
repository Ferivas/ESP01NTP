# ESP01NTP

Reloj con sincronización vía NTP en un ESP01 (ESP8266). Envía la hora por puerto serial.

## Protocolo serial

- 9600 baud, 8N1
- Formato: `$SETCLK,HHMMSS[CRLF]` (hora local)
- Ejemplo: hora NTP 15:23:24 con offset -5h → `$SETCLK,102324[CRLF]`

## Operación

- Si no hay credenciales WiFi configuradas, se abre un portal cautivo (AP `ESP01NTP`, portal en `192.168.4.1`).
- El portal permite configurar: red WiFi, password, huso horario (GMT offset en horas), intervalo de actualización NTP (`tactntp` en minutos) y intervalo de transmisión serial (`txclk` en minutos).
- En operación normal: lee hora NTP, mantiene reloj interno, re-sincroniza cada `tactntp`, transmite cada `txclk`.
- La última hora sincronizada se guarda en EEPROM y se restaura al arrancar.

## Configuración por defecto

| Parámetro | Default | Descripción |
|-----------|---------|-------------|
| GMT Offset | -5 horas | Huso horario |
| `tactntp` | 30 min | Intervalo de sincronización NTP |
| `txclk` | 1 min | Intervalo de transmisión serial |

## Persistencia (EEPROM)

| Dirección | Contenido |
|-----------|-----------|
| 0 | Estructura `EepromData` (magic, gmtOffsetSec, tactntpMin, txclkMin, lastSync) |

## Toolchain

- PlatformIO + Arduino framework
- Board: ESP01 (ESP8266, 1 MB flash)
- Dependencia: `tzapu/WiFiManager @ ^2.0.17`

## Comandos

```bash
pio run                # compilar
pio run -t upload      # grabar por puerto serial
pio device monitor     # monitor serie (9600 baud)
```

## Estructura del código

- `src/main.cpp` — programa principal: WiFiManager, NTP, reloj interno, transmisión serial.
- `platformio.ini` — entorno `esp01`, dependencia WiFiManager.

## Montaje en tarjeta perforada
El ESP01 se armo según el esquemático de [SCH](<https://github.com/Ferivas/ESP01NTP/blob/main/sch/SCH_ESP01NTP.pdf>)  y se muestra en la figura siguiente:
<img width="600" alt="Montaje" src="https://github.com/Ferivas/ESP01NTP/blob/main/docs/ESP01NTP.jpg">


## Serial

- `Serial` (GPIO1/TX0): datos del reloj a 9600 baud
- `Serial1` (GPIO2/TX1): debug a 115200 baud
