# DragonRise USB gamepad `0x0079:0x0006` — OPL driver notes

Technical notes for the USB HID gamepad integrated into OPL through the
`hidpad` layer. This document is the reference for the `0x0079:0x0006`
profile; it separates what was empirically observed from the device and what
is a requirement imposed by OPL's implementation.

Implementation mapping:

* profile table, decoder and DS2 translator: `include/hidpad.h`
* pademu driver (games): `modules/pademu/ds34usb.c` / `ds34usb.h`
* menu IOP driver (controller settings): `modules/ds34usb/iop/ds34usb.c`
* host-side test harness: `labs/hidpadtest/`
* USB capture helper (host): `pc/hid_dump_joystick.py`

---

## A. Identidad

| Campo         | Valor                  |
|---------------|------------------------|
| VID           | `0x0079`               |
| PID           | `0x0006`               |
| Dispositivo   | DragonRise, gamepad USB genérico compatible (8 botones + stick L/R + D-pad) |

In OPL both drivers claim the device through the profile `hid_pad_find()`
lookup, never through a hardcoded VID/PID test.

## B. USB / HID

### Observado / empírico

* interface class HID (`0x03`); the device enumerates with a working HID
  interface
* subclass / protocol: no descriptor dump was recorded; the single HID
  interface matches the flat subclass/protocol `0x00` / `0x00` layout
  (presumible, no capturado explícitamente)
* endpoint: interrupt **IN**, dirección `0x81`, `wMaxPacketSize = 8`
* endpoint OUT: **ausente** en el dispositivo (el joystick opera solo con el
  INT-IN)
* reporte HID de `8` bytes (ver sección C)

### Impuesto por nuestra implementación

* selección exclusivamente por perfil: solo se reclaman VID/PID con entrada en
  `hid_pad_devices[]` (`include/hidpad.h`); un HID sin perfil no se reclama
* el joystick debe exponer interface HID (`bInterfaceClass == USB_CLASS_HID`)
* debe existir un endpoint INTERRUPT IN
* `wMaxPacketSize >= profile.report_len` (8); si fuera menor, el joystick se
  rechaza como no válido
* transferencias limitadas al buffer dedicado `joy_buf[8]` (sin overflow);
  reportes de menos de `report_len` bytes se descartan
* la ausencia de endpoint OUT se permite **solo** para el tipo `JOYSTICK`
  (DS3/DS4 y guitarras siguen exigiéndola)

## C. Reporte de 8 bytes

Layout del reporte HID de entrada (`byte`/`descripción`):

| Byte | Campo                          | Rango / valores                          |
|------|--------------------------------|------------------------------------------|
| 0    | `lx` izquierdo X               | 0..255, centro 128                       |
| 1    | `ly` izquierdo Y               | 0..255, centro 128                       |
| 2    | reservado / ignorado           | —                                        |
| 3    | `rx` derecho X                 | 0..255, centro 128                       |
| 4    | `ry` derecho Y                 | 0..255, centro 128                       |
| 5    | nibble bajo: hat               | 0..7 = 8 direcciones; 8..15 = released   |
| 5    | nibble alto: botones de cara   | ver tabla                                |
| 6    | shoulders / system             | ver tabla                                |
| 7    | reservado / ignorado           | —                                        |

### Byte 5 — nibble alto (face buttons)

| Máscara | Botón    |
|---------|----------|
| `0x10`  | triangle |
| `0x20`  | circle   |
| `0x40`  | cross    |
| `0x80`  | square   |

### Byte 6 — shoulders y system buttons

| Máscara | Botón   |
|---------|---------|
| `0x01`  | L1      |
| `0x02`  | R1      |
| `0x04`  | L2      |
| `0x08`  | R2      |
| `0x10`  | Select  |
| `0x20`  | Start   |
| `0x40`  | L3      |
| `0x80`  | R3      |

### Sticks, hat e índices

* axes raw `0..255`; centro observado `128`
* hat `0..7` = las ocho direcciones (N, NE, E, SE, S, SW, W, NW)
* hat `8..15` = released; el decoder nunca indexa `ds2_dpad_hat[]` con
  `HIDP_HAT_RELEASED`

## D. Semántica de salida (modelo DS2)

`translate_pad_hid()` convierte `struct hid_pad_report` en `ds2report`:

* sticks: passthrough sin transformación (`LeftStickX/Y`, `RightStickX/Y`)
* D-pad: el hat se mapea a `DS2ButtonUp/Right/Down/Left`; en `nButtonState`
  (active-low) pulsado = 0; presión digital D-pad: pulsado → `0`, liberado →
  `255`
* face/shoulders: bits de `nButtonState` active-low; presión digital emulada
  pulsado → `255`, liberado → `0`
* Select / Start / L3 / R3: solo bits de `nButtonState` (sin canal de presión
  DS2)
* `nButtonState` conserva la semántica active-low existente

No existe emulación de botones con presión sensible real: los valores de
presión son digitales fijos (0/255). El joystick informa la misma presión para
cualquier presión física.

## E. Limitaciones actuales

* solo existe el perfil `0x0079:0x0006` en `hid_pad_devices[]`
* no hay parser HID genérico ni se interpreta el HID Report Descriptor
* no hay autodetección de layouts desconocidos
* el rumble se activa a través de `HIDP_CAP_RUMBLE` (ver sección G); la
  intensidad se controla solo con `lrum` (motor izquierdo, fuerte) y `rrum`
  (motor derecho, débil)
* Analog OFF no fue resuelto en esta fase (ver apéndice: comportamiento
  observado del dispositivo, no implementado)
* cualquier otro dispositivo requiere un perfil explícito con su decoder

## F. Estado de validación

| Item                      | Estado                                                                 |
|---------------------------|------------------------------------------------------------------------|
| tests host-side (`labs/hidpadtest/`) | Ejecutados: gcc (MinGW), 469/469 checks OK                   |
| compilación PS2SDK        | Pendiente (no hay toolchain instalada)                                |
| prueba con hardware real (entrada)  | Completa (perfil, ejes, hat, botones)                    |
| prueba con hardware real (rumble)   | Windows: validado físicamente (sección G). PS2: fallo diagnosticado (prefijo `0x00` + `wLength=8`) y corregido; pendiente re-probar build corregido |

Las capturas crudas del dispositivo reproducidas en el apéndice fueron
registradas durante el desarrollo y no se han reproducido físicamente en este
cierre.

---

## G. Protocolo de rumble (DragonRise SET_REPORT)

El joystick `0x0079:0x0006` NO tiene endpoint OUT; el rumble se envía por el
endpoint de control (SET_REPORT, `bmRequestType=0x21`, `bRequest=0x09`,
`wValue=0x0200`, `wIndex=0`, `wLength=7`). El transporte es el mismo que
`UsbControlTransfer` de la pila USB de OPL usando un reporte de tipo OUTPUT
(`HID_USB_SET_REPORT_OUTPUT`).

> **Causa raíz del fallo en PS2 (corregido).** El firmware del dispositivo
> espera los **7 bytes de protocolo SIN el byte de report ID `0x00`** delante.
> Windows lo enmascara: el minidriver HID elimina el byte de report ID
> implícito `0x00` del buffer de `HidD_SetOutputReport` antes de enviarlo al
> bus, así que el buffer de 8 bytes `00 51 00 <rrum> 00 <lrum> 00 00` llegaba
> al dispositivo como `51 00 <rrum> 00 <lrum> 00 00` (7 bytes) y el motor
> vibraba. La pila USB de la PS2 **no** elimina ese byte: enviaba los 8 bytes
> completos, `byte[0]=0x00` corrompía el comando y el rumble no actuaba.
> La corrección elimina el prefijo `0x00` y reduce `wLength` de 8 a 7 en todos
> los envíos (UPDATE, COMMIT y STOP).

### G.1 Actualización de motores (UPDATE)

```
51 00 <rrum> 00 <lrum> 00 00
```

Los bytes relevantes:

* byte 0: `0x51` (comando de actualización de motores)
* byte 1: `0x00` (reservado / separador)
* byte 2: `rrum` — intensidad del motor derecho/weak/`rrum`
* byte 3: `0x00` (reservado / separador)
* byte 4: `lrum` — intensidad del motor izquierdo/strong/`lrum`

### G.2 Commit (COMMIT)

```
FA FE 00 00 00 00 00
```

* byte 0: `0xFA` (comando de commit)
* byte 1: `0xFE` (constante del comando)

### G.3 Stop (STOP)

```
F3 00 00 00 00 00 00
```

* byte 0: `0xF3` (comando de parada de motores)

### G.4 Invariante del transporte

UN SET_REPORT de UPDATE no actúa el motor por sí solo; es necesario un COMMIT
(intensidades + commit). El orden verificado físicamente es:

```
UPDATE (51 00 ... ) → COMMIT (FA FE ...)
```

Enviar UPDATE sin COMMIT no produce vibración (`FA FE` es necesario — confirmado
físicamente). El COMMIT se envía a la vez que el STOP en la liberación del
dispositivo.

### G.5 Rango de intensidad y quirk `0x0A → 0x0B`

* el dispositivo acepta valores `0x00..0x0B`; `0x0B` es máximo
* quirk `0x0A → 0x0B`: heredado de drivers previos como compatibilidad
  defensiva. NO reproducido como requisito demostrado en esta validación
  física; no se observó bloqueo ni diferencia funcional entre `0x0A` y `0x0B`
* la API interna acepta el rango completo 0..255; el driver no aplica máscara
  (el dispositivo acepta los valores, la intensidad percibida satura antes)

### G.6 Cómo se integra en OPL

* `include/hidpad.h`: encoders `hid_pad_encode_rumble_0079_0006()` /
  `hid_pad_encode_commit_0079_0006()` / `hid_pad_encode_stop_0079_0006()` y
  `HIDP_CAP_RUMBLE` activado en el perfil
* `modules/pademu/ds34usb.c`: `LEDRumble()` (game path) envía UPDATE + COMMIT;
  `usb_release()` envía STOP antes de cerrar endpoints
* `modules/ds34usb/iop/ds34usb.c`: misma integración en el path del menú
* el camino sin `HIDP_CAP_RUMBLE` ejecuta `PollSema → SignalSema → return 0`
  (evita el timeout de 200 ms de `TransferWait`)

### G.7 Validación física

| Prueba                          | Resultado                                      |
|---------------------------------|------------------------------------------------|
| identificación HID              | VID `0x0079`, PID `0x0006`, versión `0x0107`   |
| caps HID (`HidP_GetCaps`)       | Input=9, Output=8, Feature=0, ValueCaps=1, FeatureValueCaps=0, status `0x00110000` |
| SET_REPORT aceptado por la pila | sí (`HidD_SetOutputReport`); el conteo Output=8 incluye el report ID implícito `0x00` → al bus van 7 bytes |
| endpoint OUT (interrupt)        | **ausente**: `hidapi.write()` devuelve -1; `WriteFile` de pywinusb hace timeout |
| control APIs (`HidD_SetOutputReport`/`GetInputReport`) | devuelven FALSE con error 0 en este dispositivo sin report ID (quirk de Windows; no usable como evidencia) |
| motor weak (`rrum`) solo        | sin vibración perceptible                      |
| motor strong solo               | vibración en el lado izquierdo                  |
| ambos motores                   | vibración inicial breve en ambos lados, persiste la izquierda |
| COMMIT sin UPDATE               | ningún motor vibra                             |
| quirk `0x0A` / `0x0B`            | sin bloqueo; sin diferencia funcional           |
| STOP                            | confirmado; motores se detienen                |
| prueba en PS2 real (pre-fix)    | entrada completa; **rumble no vibraba** con `wLength=8` y prefijo `0x00` (vibración habilitada en Pad Emulation) |
| prueba en PS2 real (post-fix)   | pendiente — verificar con el build corregido (`wLength=7`, sin prefijo) |

---

## Apéndice A — Capturas empíricas originales (analog ON / OFF)

Cada tupla es un reporte HID de 8 bytes. Se han eliminado las repeticiones
idénticas.

### Analog ON — reposo

```
[128, 128, 0, 128, 128, 15, 0, 0]
```

### Analog ON — botones de cara

```
TRIANGULO:  [128, 128, 0, 128, 128, 31, 0, 0]
CIRCULO:    [128, 128, 0, 128, 128, 47, 0, 0]
EQUIS:      [128, 128, 0, 128, 128, 79, 0, 0]
CUADRADO:   [128, 128, 0, 128, 128, 143, 0, 0]
```

### Analog ON — gatillos

```
R1: [128, 128, 0, 128, 128, 15, 2, 0]
R2: [128, 128, 0, 128, 128, 15, 8, 0]
L1: [128, 128, 0, 128, 128, 15, 1, 0]
L2: [128, 128, 0, 128, 128, 15, 4, 0]
```

### Analog ON — cruzeta

```
ARRIBA:    [128, 128, 0, 128, 128, 0, 0, 0]
ABAJO:     [128, 128, 0, 128, 128, 4, 0, 0]
IZQUIERDA: [128, 128, 0, 128, 128, 6, 0, 0]
DERECHA:   [128, 128, 0, 128, 128, 2, 0, 0]
```

### Analog ON — Select / Start / Analog toggle / L3

```
SELECT:  [128, 128, 0, 128, 128, 15, 16, 0]
START:   [128, 128, 0, 128, 128, 15, 32, 0]
L3:      [128, 128, 0, 128, 128, 15, 64, 0]
ANALOG ON:  [128, 128, 0, 128, 128, 15, 0, 0]
ANALOG OFF: [127, 127, 0, 128, 128, 15, 0, 0]
```

### Analog ON — analógico izquierdo

```
ARRIBA:  [128, 0, 0, 128, 128, 15, 0, 0]
ABAJO:   [128, 255, 0, 128, 128, 15, 0, 0]
DERECHA: [255, 128, 0, 128, 128, 15, 0, 0]
IZQUIERDA: [0, 128, 0, 128, 128, 15, 0, 0]
```

### Analog ON — analógico derecho

```
ARRIBA:  [128, 128, 0, 128, 0, 15, 0, 0]
ABAJO:   [128, 128, 0, 128, 255, 15, 0, 0]
DERECHA: [128, 128, 0, 255, 128, 15, 0, 0]
IZQUIERDA: [128, 128, 0, 0, 128, 15, 0, 0]
R3:      [128, 128, 0, 128, 128, 15, 128, 0]
```

### Analog OFF — reposo y sticks

Con el analog en OFF el reposo se registra como `127/127` (no `128/128`) en el
stick izquierdo. Además, **el analógico derecho se comporta como botones de
cara** (comportamiento observado del dispositivo, NO implementado):

```
REPOSO: [127, 127, 0, 128, 128, 15, 0, 0]
D ANALOG DER. ARRIBA:  [127, 127, 0, 128, 0, 31, 0, 0]
D ANALOG DER. ABAJO:   [127, 127, 0, 128, 255, 79, 0, 0]
D ANALOG DER. DERECHA: [127, 127, 0, 255, 128, 47, 0, 0]
D ANALOG DER. IZQUIERDA: [127, 127, 0, 0, 128, 143, 0, 0]
```

En analog OFF el stick izquierdo sigue enviando sus ejes (byte 0/1) y la
cruzeta (byte 5 nibble bajo) igual que en analog ON:

```
ARRIBA:    [127, 0, 0, 128, 128, 15, 0, 0]
ABAJO:     [127, 255, 0, 128, 128, 15, 0, 0]
IZQUIERDA: [0, 127, 0, 128, 128, 15, 0, 0]
DERECHA:   [255, 127, 0, 128, 128, 15, 0, 0]
```

### Analog OFF — botones / gatillos / system

Las mismas máscaras que en analog ON sobre byte 5 nibble alto y byte 6:

```
TRIANGULO: [127, 127, 0, 128, 128, 31, 0, 0]
CIRCULO:   [127, 127, 0, 128, 128, 47, 0, 0]
EQUIS:     [127, 127, 0, 128, 128, 79, 0, 0]
CUADRADO:  [127, 127, 0, 128, 128, 143, 0, 0]
R1: [127, 127, 0, 128, 128, 15, 2, 0]
R2: [127, 127, 0, 128, 128, 15, 8, 0]
L1: [127, 127, 0, 128, 128, 15, 1, 0]
L2: [127, 127, 0, 128, 128, 15, 4, 0]
SELECT: [127, 127, 0, 128, 128, 15, 16, 0]
START:  [127, 127, 0, 128, 128, 15, 32, 0]
L3:     [127, 127, 0, 128, 128, 15, 64, 0]
R3:     [127, 127, 0, 128, 128, 15, 128, 0]
ANALOG ON:  [128, 128, 0, 128, 128, 15, 0, 0]
ANALOG OFF: [127, 127, 0, 128, 128, 15, 0, 0]
```