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
* rumble no implementado para este joystick (`caps = 0`, sin canal de salida)
* Analog OFF no fue resuelto en esta fase (ver apéndice: comportamiento
  observado del dispositivo, no implementado)
* cualquier otro dispositivo requiere un perfil explícito con su decoder

## F. Estado de validación

| Item                      | Estado                                                                 |
|---------------------------|------------------------------------------------------------------------|
| tests host-side (`labs/hidpadtest/`) | Ejecutados: gcc (MinGW) 4.6.2, 469/469 checks OK               |
| compilación PS2SDK        | Pendiente (no hay toolchain instalada)                                |
| prueba con hardware real  | Pendiente (no se probó el driver en consola para este cierre)         |

Las capturas crudas del dispositivo reproducidas en el apéndice fueron
registradas durante el desarrollo y no se han reproducido físicamente en este
cierre.

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