
# Hardware Wiring Notes

- ESP32 GPIO → Relay IN pins (active HIGH assumed). Use opto-isolated relay module.
- LDR divider: LDR between 3.3V and ADC (GPIO34), 10k resistor from ADC to GND.
- PIR: HC-SR501 OUT → GPIO13; power PIR from 5V, but OUT is 3.3V logic compatible.
- DHT11: DATA → GPIO4 with 4.7k pull-up to 3.3V.
- Buttons: BTN_LIGHT → GPIO33 to GND (INPUT_PULLUP), BTN_FAN → GPIO25 to GND.
- Buzzer: GPIO14 → Buzzer +, Buzzer − → GND (for active buzzer); for passive, add transistor driver.

## ASCII Schematic (logic-level only)

            +3.3V
              |
             [LDR]
              |-----> ADC GPIO34  (LDR reading)
             [10k]
              |
             GND

PIR OUT ---- GPIO13  (motion)

DHT11  ---- GPIO4    (data)

GPIO26 ---- Relay1 IN ----> Light (AC via relay)
GPIO27 ---- Relay2 IN ----> Fan (AC via relay)
GPIO14 ---- Buzzer

BTN_LIGHT -- GPIO33 (to GND, INPUT_PULLUP)
BTN_FAN   -- GPIO25 (to GND, INPUT_PULLUP)

ESP32 5V -> PIR Vcc, Relay Vcc (as per module); keep grounds common for logic.
**Keep mains AC isolated from low-voltage area**.
