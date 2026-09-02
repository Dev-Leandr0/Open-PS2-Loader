import hid
import time

d = hid.device()
d.open(0x0079, 0x0006)

d.set_nonblocking(True)

print("Sin soltar: presiona cada boton y mueve cada stick, Ctrl+C para terminar.")

while True:
    data = d.read(64, timeout_ms=200)

    if data:
        print(data)

    time.sleep(0.05)