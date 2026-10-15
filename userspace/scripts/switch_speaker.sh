#!/bin/sh
CARD="gmshamrock"
if ! amixer -c "$CARD" info >/dev/null 2>&1; then
    amixer -c "gm-shamrock" info >/dev/null 2>&1 && CARD="gm-shamrock" || CARD="0"
fi

# Enable AW8738 Hardware via GPIO 38 (gpiochip512 + 38 = 550)
# Use Mode 2 (Clean Boost, Low Noise Floor) instead of aggressive Mode 5
if [ ! -d /sys/class/gpio/gpio550 ]; then
    echo 550 > /sys/class/gpio/export 2>/dev/null
fi
if [ -d /sys/class/gpio/gpio550 ]; then
    echo out > /sys/class/gpio/gpio550/direction 2>/dev/null
    python3 -c "
import os, time
try:
    fd = os.open('/sys/class/gpio/gpio550/value', os.O_WRONLY)
    os.write(fd, b'0')
    time.sleep(0.01)
    for _ in range(2):
        os.write(fd, b'1')
        os.write(fd, b'0')
    os.write(fd, b'1')
    os.close(fd)
except Exception:
    pass
" 2>/dev/null || true
fi

# Route audio to HPH_R with DAPM consumer active
amixer -c "$CARD" cset name='Headphone Jack Switch' 1 >/dev/null 2>&1
amixer -c "$CARD" cset name='EAR_S' ZERO >/dev/null 2>&1
amixer -c "$CARD" cset name='HPHL' ZERO >/dev/null 2>&1
amixer -c "$CARD" cset name='RX2 MIX1 INP1' RX2 >/dev/null 2>&1
amixer -c "$CARD" cset name='RX2 MIX1 INP2' RX1 >/dev/null 2>&1
amixer -c "$CARD" cset name='RDAC2 MUX' RX2 >/dev/null 2>&1
amixer -c "$CARD" cset name='HPHR' Switch >/dev/null 2>&1
amixer -c "$CARD" cset name='RX2 Digital Volume' 62 >/dev/null 2>&1

echo "[+] AW8738 Speaker active (Low-Noise Mode 2)."
