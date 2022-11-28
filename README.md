# RadiationWatcher #

Overview | Details
---|---
Language: | C++ 
IDE: | [PlatformIO](https://platformio.org/) (VS Code + PlatformIO extension)
Microcontroller: |  [Sparkfun ESP8266 Thing Dev](https://www.sparkfun.com/products/13711)
Radiation Sensor: | [Type 5 Pocket Geiger Radiation Sensor]( https://www.sparkfun.com/products/14209) from [Radiation Watch](http://www.radiation-watch.org/)

![Sparkfun ESP8622](doc/esp8266thingdev.jpg)
![Type 5 Pocket Geiger](doc/PocketGeiger.png)

The radiation detector interface uses a 3.5mm audio jack as a connector as well as pads on the board. I wanted to keep the option of putting it back into the white plastic case it comes to use it with a phone. So I soldered a 90 degree pin header on (so that it could remain on even if placed back in its case).
Conveniently, the radiation detector accepts any voltage between 3V and 9V. As the Esp8266 runs at 3.3V I simply used that. 

![Pocket Geiger connector](doc/PocketGeiger_interface.png)

I created a daughtercard for the ESP8266 Thing upon which I could mount the radiation detector. There are no mounting holes, hence the dodgy zip ties to hold to two together. I placed the two brass plates that come with the radiation detector on either side of the board. These plates block out any alpha and beta particles so our measurements will strictly be gamma only. The sensor area with the plates is wrapped in electrical tape.

![RadiationWatcher](doc/RadiationWatcher01.jpg)

On the bottom view you can see how pins 4 (yellow) and 12 (green) map to noise (NS) and signal (SIG) respectively.

![RadiationWatcher](doc/RadiationWatcher02.jpg)

## MQTT ##

When the device starts, it establishes a wifi connection (rename [sample_env.h](src/sample-env.h) to env.h and edit for your own environment) and sends a few Home Assistant auto-discovery messages. It announces itself with an "online" message on its availability topic. 
Then it settles into sending sensor updates. The updates happen when the sensor is triggered. In my experience there can be anywhere from 1 second to 90 seconds between triggers, though the median is probably in the lower third range. 
Each sensor update sends the following message:
```
homeassistant/sensor/esp8266thing/state
{
  "frequency": 0.0461,
  "frequency_details": {
    "dose": 0.04,
    "dose_err": 0.01,
    "cpm": 2.3
  }
}
``` 
You can change the device id ("esp8266thing") by updating [DEVICE_ID in radthing.h](include/radthing.h).

The "cpm" measurement is clicks-per-minute, like a traditional geiger tube counter. It counts the frequency of gamma particle impacts and is translated into equivalent dose in micro sieverts per hour. 
Home Assistant has no radiation measurement support, but does support frequency. The CPM is converted to cycles per second (hertz) and announced as a frequency update. The CPM and dose values are also included in the payload. 

Every 5 minutes the device will send diagnostic information and refresh its "online" availability status.
```
homeassistant/sensor/esp8266thing/diagnostics
{
  "wifi_rssi": -36,
  "wifi_ip": "10.0.0.48",
  "wifi_mac": "5C:CF:7F:AE:DE:0A"
}
```

## Home Assistant Integration ##

The sensor state (frequency) and diagnostics all have MQTT auto-discovery messages that are sent as the device is starting up. These are retained messages, so will be available to Home Assistant in the event of an HA restart. 

![Home Assistant Device](doc/esp8266thing-ha-device.png)

The discovery message for frequency:
```
homeassistant/sensor/esp8266thing/frequency/config
{
  "device_class": "frequency",
  "unit_of_measurement": "Hz",
  "state_class": "measurement",
  "availability_topic": "homeassistant/sensor/esp8266thing/availability",
  "unique_id": "esp8266thing_frequency",
  "device": {
    "name": "RadiationWatcher",
    "identifiers": "5C:CF:7F:AE:DE:0A",
    "mf": "Sparkfun",
    "mdl": "ESP8266 Thing Dev",
    "sw": "20221127.1800"
  },
  "name": "esp8266thing frequency",
  "icon": "mdi:radioactive",
  "state_topic": "homeassistant/sensor/esp8266thing/state",
  "value_template": "{{ value_json.frequency }}",
  "json_attributes_topic": "homeassistant/sensor/esp8266thing/state",
  "json_attributes_template": "{{ value_json.frequency_details | tojson }}"
}
```

The discovery messages for diagnostics, specifically measurable (but unitless) RSSI:
```
homeassistant/sensor/esp8266thing/wifi_rssi/config
{
  "state_class": "measurement",
  "entity_category": "diagnostic",
  "availability_topic": "homeassistant/sensor/esp8266thing/availability",
  "unique_id": "esp8266thing_wifi_rssi",
  "device": {
    "name": "RadiationWatcher",
    "ids": "5C:CF:7F:AE:DE:0A"
  },
  "name": "esp8266thing wifi_rssi",
  "icon": "mdi:wifi-strength-2",
  "state_topic": "homeassistant/sensor/esp8266thing/diagnostics",
  "value_template": "{{ value_json.wifi_rssi }}"
}
```
The other diagnostics (MAC, IP) are similar, but obviously not measurable so lack the state_class.