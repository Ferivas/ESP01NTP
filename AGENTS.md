# ESP01NTP

Reloj con sincronización vía NTP en un ESP01 (ESP8266). Envía la hora por puerto serial.

## Protocolo serial

- 9600 baud, 8N1
- Formato: `$SETCLK,HHMMSS[CRLF]` (hora local)
- Ejemplo: hora NTP 15:23:24 con offset -5h → `$SETCLK,102324[CRLF]`

## Operación

- Si no hay credenciales WiFi configurar portal cautivo (red, password, huso, `tactntp`, `txclk` en fracciones de minuto).
- Operación normal: leer hora NTP, mantener reloj interno, re-sincronizar cada `tactntp`, transmitir cada `txclk`.

## Toolchain

- PlatformIO + Arduino framework (mismo patrón que `~/RELOJNTP`)
- Board: ESP01 (ESP8266, 1 MB flash) — recursos muy limitados

## Serial

- `Serial` (GPIO1/TX0): datos del reloj a 9600 baud
- `Serial1` (GPIO2/TX1): debug a 115200 baud
