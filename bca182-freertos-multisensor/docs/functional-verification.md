# Wokwi Functional Verification

Project: BCA182 FreeRTOS Multisensor
Environment: STM32 Blue Pill simulation in Wokwi

Firmware build command: `pio run -e bluepill_f103c8`

Test date: September 28, 2026

|           ID          | Input / Stimulus | Expected Result | Actual Observation | Result |
| -------  |---|---|---|---|
| FT-01 | Set DHT22 temperature to 28 °C | OLED displays 28 °C | OLED displayed 28 °C | PASS |
| FT-02 | Set DHT22 humidity to 70% | OLED displays 70% humidity | OLED displayed 70% humidity | PASS |
| FT-03 | Change LDR setting from 500 to 100 lux | Displayed light level changes | OLED light-level percentage changed | PASS |
| FT-04 | From Temperature, rotate encoder clockwise once | Humidity page appears | OLED switched to the Humidity page | PASS |
| FT-05 | From Humidity, rotate encoder counterclockwise once | Temperature page appears | OLED switched to the Temperature page | PASS |
| FT-06 | Set DHT22 temperature to 31 °C | Temperature alarm activates and buzzer sounds | Temperature alarm activated and buzzer sounded | PASS |
| FT-07 | Return DHT22 temperature to 25 °C | Temperature alarm clears and buzzer stops | Temperature alarm cleared and buzzer stopped | PASS |
| FT-08 | Trigger PIR motion | System is ACTIVE | System entered or remained ACTIVE | PASS |
| FT-09 | Wait 15 seconds after PIR output returns LOW | System enters INACTIVE | System entered INACTIVE | PASS |
| FT-10 | Trigger PIR motion while INACTIVE | System returns to ACTIVE | System returned to ACTIVE | PASS |