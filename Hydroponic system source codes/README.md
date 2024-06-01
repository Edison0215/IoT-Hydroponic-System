Remark: Two ESP32s are implemented, namely master and slave.
# hydroponic_master.ino

Description:
- This code primarily uses the self-modified ECPH library to measure nutrient electrical conductivity (EC) and pH in the hydroponic system reservoir. (master ESP32)
- Both parameters' measurement precision is improved from 12-bit to 16-bit when ADS1115 is applied. 
- It can also send both readings to a ThingSpeak channel online every 15 seconds. 
- Users need to calibrate the EC and pH probes before immersing them into the reservoir for real-time measurement.
- Users can also choose whether to activate necessary peristaltic pumps for nutrient regulation in the reservoir.

Calibration procedure:
1. Connect the ESP32 power port to a laptop.
2. Connect the EC probe and pH probe to the respective BNC connectors.
3. Open the Arduino IDE (make sure your laptop is readily compatible with ESP32).
4. After noticing random values appearing on the Arduino IDE serial monitor over time, a successful connection is indicated.
5. Prompt "ENTEREC" on the serial monitor to enter EC calibration mode.
6. Clean the EC probe.
7. Immerse the EC probe into the 12.88 mS/cm EC calibration solution.
8. Wait until the uncalibrated EC readings displayed on the serial monitor become stable.
9. Prompt "CALEC" on the serial monitor to calibrate the EC probe.
10. Clean the EC probe and immerse it into the 1.413 mS/cm EC calibration solution.
11. Wait until the uncalibrated EC readings displayed on the serial monitor become stable.
12. Prompt "CALEC" on the serial monitor to calibrate the EC probe.
13. Prompt "EXITEC" on the serial monitor to exit EC calibration mode.
14. EC calibration status converts from "Pending" to "Done", indicating a successful two-point EC calibration.
15. Clean the EC probe.
16. Immerse the EC probe into the reservoir.

17. Prompt "ENTERPH" on the serial monitor to enter pH calibration mode.
18. Clean the pH probe.
19. Immerse the pH probe into the pH 7 calibration solution.
20. Wait until the uncalibrated pH readings displayed on the serial monitor become stable.
21. Prompt "CALPH" on the serial monitor to calibrate the pH probe.
22. Clean the pH probe and immerse it into the pH 4 calibration solution.
23. Wait until the uncalibrated pH readings displayed on the serial monitor become stable.
24. Prompt "CALPH" on the serial monitor to calibrate the pH probe.
25. Clean the pH probe.
26. Immerse the pH probe into the reservoir.
27. After readings are stabilized, prompt "EXITPH" on the serial monitor to exit pH calibration mode.
28. The message of "Transmitting data" and the lower rate of both readings displayed on the serial monitor indicate successful pH calibration (Online data transmission starts).

Cleaning procedure:
1. EC probe:
- Use a small dry cloth or tissue to wipe the rod surface.
- DO NOT TOUCH THE METAL PLATES AT THE TIP OF THE PROBE.
- ONLY WIPE THE PLASTIC SURFACE.
- If the probe is unused, secure the probe with its cap. (Internal cap and its tips should be dry.)
2. pH probe:
- Use a small dry cloth or tissue to wipe the rod surface.
- DO NOT TOUCH THE GLASS BALL AT THE TIP OF THE PROBE.
- ONLY WIPE THE PASTIC SURFACE.
- If the probe is unused, secure the probe with its cap. (Internal cap contains its protective solution).
- DO NOT SPILL THE PROTECTIVE SOLUTION.

Calibration criterion:
- EC calibration and pH calibration can be done regardless of its sequence.
- calibration points for each parameter can be done regardless of their sequences.
- 12.88 mS/cm: only successful when the uncalibrated EC readings are between 8.0 mS/cm and 16.8 mS/cm.
- 1.413 mS/cm: only successful when the uncalibrated EC readings are between 0.7 mS/cm and 1.8 mS/cm.
- Calibration fails may be caused by contaminated calibration solutions.
- Try again after replacing the calibration solutions.

Nutrient regulation commands:
- Prompt "ECPHDOWN" to activate the nutrient regulation process. (status changes from 0 to 1)
- Prompt "ECPHUP" to deactivate the nutrient regulation process. (status changes from 1 to 0)
- These 2 commands only affects the nutrient A pump, nutrient B pump, pH up pump and pH down pump.
- Other actuators such as cooling fan, grow lights, reservoir pump and water pump are not affected.
- If "ECPH" status on OLED does not change and regulation does not start when "ECPHDOWn" is promptede, please check your internet connection (reset the WiFi Dongle device and the slave ESP32).

# hydroponic_slave.ino

Description: 
- This code is used to collect enviromental data such as light intensity, room humidity, water level, reservoir level, reservoir temperature, and surrounding temperature. (slave ESP32)
- All these 6 data are delivered online to another ThingSpeak channel every 15 seconds.
- It is also capable to obtain EC and pH readings from the first ThingSpeak channel used by master ESP32 to send both data.
- All 8 data are accessed by the slave ESP32 to trigger the activation of actuators such as cooling fan, grow lights, nutrient pumps, water pumps, pH buffering pumps and reservoir pump.
