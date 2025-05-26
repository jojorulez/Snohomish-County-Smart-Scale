# Snohomish-County-Smart-Scale

This smart scale is designed to work with the LinkLabs SuperTag device to send data to a live dashboard for the use of the ballot pickup team during elections.

The **DiagnosticSketch** program displays all the readings of the battery and weight on the OLED display live so that it is easy to see what is going on while assembling the scale.

The **DeploymentSketch** program is meant for real use and is optimized for battery life so the OLED will not display anything, and the BLE signals will update slower due to the weight measurements only being taken every 5 minutes. This weighing interval can be modified by changing the measurementInterval variable at the beginning of the program. Both programs can be easily uploaded to the Heltec WiFi LoRa V3 ESP 32 with Arduino IDE using a laptop and a USB-C cable.

## Bluetooth Advertisements:
Bluetooth advertisements are sent out once per second and they contain data describing the frame type, battery percentage, and box fullness tier. The tiers are in increments of 50lbs each tier represents 50 lbs more added to the ballot box. These tiers can easily be changed by modifying some of the variables in the deployment program. Each 50 lb tier is intended to be interpreted as a 25% increase in ballot box fullness where 200 lbs represents a 100% full box. These tiers do not stop at 100%, they continue onward so that overfilling is still quantified (260 lbs -> tier 5 -> 125% full). When a tier change is registered the frame type is updated to 0x03 for 15 minutes so that the SuperTag knows to transmit this update to the cloud.

## Use Instructions:
This scale is designed to be safe to stand on even with the ballot box being completely full. The total Capacity is over 2000 lbs, and each leg of the scale can hold over 500 lbs so workers need not worry about breaking it. Once ballots have been removed and empty crates have been placed, workers should reach under the access handle and press the tare button for at least 2 seconds to ensure that the press is registered. Batteries are not likely to last long enough to span more than a few months, so it probably makes the most sense to remove the batterries for charging after each election cycle. The best schedule for removing and replacing batteries will probably need to be determined after several elections of use when there is better data about real life battery usage.
