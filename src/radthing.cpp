//#include <Arduino.h>  appears to be automatically added by framework specified in platformio.ini
#include "RadiationWatch.h"
#include <wifi-helper.h>
#include <mqtt-ha-helper.h>
#include "radthing.h"
#include "env.h"
#include "utils.h"

/*
Built for Sparkfun's ESP8266 board with 512KB Flash
https://www.sparkfun.com/products/13711

!!! Note !!!
To make this program fit within the 512KB flash, you will need to need to reassign the 64KB used by SPIFF
to the available sketch area.

In platformio.ini:
  board_build.ldscript = eagle.flash.512k.ld
*/

// *** Global Variables ***
WiFiClient wificlient;
MQTTClient mqttclient(768); // default is 128 bytes;  https://github.com/256dpi/arduino-mqtt#notes

/*
  Since the fully constructed list of discovery_config (topics and payloads) consumes considerable RAM, reduce it to just the facts.
  Generate each discovery_config one at a time at the point of publishing the message in order to conserve RAM.
  Once everything is successfully published, purge list contents (no longer needed).
*/
std::vector<discovery_metadata> discovery_metadata_list;                                            // list of data used to construct discovery_config for sensor discovery
std::vector<discovery_config_metadata> discovery_config_metadata_list;                              // list of data used to construct discovery_config for config/control discovery
std::vector<discovery_measured_diagnostic_metadata> discovery_measured_diagnostic_metadata_list;    // list of data used to construct discovery_config for sensor measured diagnostics (like RSSI)
std::vector<discovery_fact_diagnostic_metadata> discovery_fact_diagnostic_metadata_list;            // list of data used to construct discovery_config for fact diagnostics (like IP address)

// Last will and testament topic
const std::string AVAILABILITY_TOPIC = buildAvailabilityTopic("sensor", std::string(DEVICE_ID)); // homeassistant/sensor/esp8266thing/availability

const std::string DIAGNOSTIC_TOPIC = buildDiagnosticTopic("sensor", std::string(DEVICE_ID)); // homeassistant/sensor/esp8266thing/diagnostics

// All sensor updates are published in a single complex json payload to a single topic
const std::string STATE_TOPIC = buildStateTopic("sensor", std::string(DEVICE_ID)); // homeassistant/sensor/esp8266thing/state


// radiation (gamma) [alpha, beta only measurable at close range, without shielding plates]
// Radiation Watch Type 5 Sensor
RadiationWatch radiationWatch(SIG_PIN, NS_PIN);

/*
  device_class : https://developers.home-assistant.io/docs/core/entity/sensor?_highlight=device&_highlight=class#available-device-classes
                 https://www.home-assistant.io/integrations/sensor/#device-class
  has_sub_attr : true if you want to provide sub attributes under the main attribute
  icon : https://materialdesignicons.com/
  unit : (depends on device class, see link above)
*/
// build discovery message - step 1 of 4
std::vector<discovery_metadata> ICACHE_FLASH_ATTR getAllDiscoveryMessagesMetadata(){
  discovery_metadata radiation;

  // temp.device_class = "temperature";
  // temp.has_sub_attr = false;
  // temp.icon = "mdi:home-thermometer";
  // temp.unit = "°C";

  // humidity.device_class = "humidity";
  // humidity.has_sub_attr = false;
  // humidity.icon = "mdi:water-percent";
  // humidity.unit = "%";

  // There is no radiation device class; frequency (as it relates to geiger gamma ray cpm) was the closest match
  // Actual sieverts will be passed as sub-attributes. 
  radiation.device_type = "sensor";
  radiation.device_class = "frequency"; 
  radiation.has_sub_attr = true;
  radiation.icon = "mdi:radioactive";
  radiation.unit = "Hz"; // must convert count per minute to count per second (Hz)

  std::vector<discovery_metadata> dm = { radiation };  
  return dm;
}

// build discovery config/control message - step 1 of 4
std::vector<discovery_config_metadata> ICACHE_FLASH_ATTR getAllDiscoveryConfigMessagesMetadata(){
  // discovery_config_metadata refrate;

  // refrate.device_type = "number";
  // refrate.control_name = "refreshrate";
  // refrate.custom_settings = "\"min\": 1, \"max\": 60, \"step\": 1";
  // refrate.icon = "mdi:refresh-circle";
  // refrate.unit = "minutes";
  
  std::vector<discovery_config_metadata> dcm = { };  
  return dcm;
}

std::vector<discovery_measured_diagnostic_metadata> ICACHE_FLASH_ATTR getAllDiscoveryMeasuredDiagnosticMessagesMetadata(){
  discovery_measured_diagnostic_metadata rssi;

  rssi.device_type = "sensor";
  rssi.device_class = "";  // battery | date | duration | timestamp | ... In some cases may be "None" - https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes    
  //rssi.state_class = "measurement";  Accept default
  rssi.diag_attr = "wifi_rssi";
  rssi.icon = "mdi:wifi-strength-2";  // https://materialdesignicons.com/
  rssi.unit = ""; // RSSI is unitless
  
  std::vector<discovery_measured_diagnostic_metadata> dmdm = { rssi };  
  return dmdm;
}

std::vector<discovery_fact_diagnostic_metadata> ICACHE_FLASH_ATTR getAllDiscoveryFactDiagnosticMessagesMetadata(){
  discovery_fact_diagnostic_metadata ip, mac;
  
  ip.device_type = "sensor";
  ip.diag_attr = "wifi_ip";
  ip.icon = "mdi:ip-network";  

  mac.device_type = "sensor";
  mac.diag_attr = "wifi_mac";
  mac.icon = "mdi:network-pos";  


  std::vector<discovery_fact_diagnostic_metadata> dfdm = { ip, mac };  
  return dfdm;
}

/* If the buildShortDevicePayload() is used, and there are no config/controls that use the full device payload, then HA will never see it.
   So the full device payload is used for both kinds of discovery message. The short version is used for diagnostic messages because they 
   are always accompanied by sensor and/or control messages. */

// build discovery message - step 3 of 4
discovery_config ICACHE_FLASH_ATTR getDiscoveryMessage(discovery_metadata disc_meta){
    return getDiscoveryMessage(disc_meta, DEVICE_ID, buildDevicePayload(DEVICE_NAME, getMAC(), DEVICE_MANUFACTURER, DEVICE_MODEL, DEVICE_VERSION), AVAILABILITY_TOPIC, STATE_TOPIC);
}

// build discovery config/control message - step 3 of 4
discovery_config ICACHE_FLASH_ATTR getDiscoveryMessage(discovery_config_metadata disc_meta){  
  return getDiscoveryConfigMessage(disc_meta, DEVICE_ID, buildDevicePayload(DEVICE_NAME, getMAC(), DEVICE_MANUFACTURER, DEVICE_MODEL, DEVICE_VERSION), AVAILABILITY_TOPIC);
}

// build discovery measured diagnostic message - step 3 of 4
discovery_config ICACHE_FLASH_ATTR getDiscoveryMessage(discovery_measured_diagnostic_metadata disc_meta){  
  return getDiscoveryMeasuredDiagnosticMessage(disc_meta, DEVICE_ID, buildShortDevicePayload(DEVICE_NAME, getMAC()), AVAILABILITY_TOPIC, DIAGNOSTIC_TOPIC);
}

// build discovery diagnostic fact message - step 3 of 4
discovery_config ICACHE_FLASH_ATTR getDiscoveryMessage(discovery_fact_diagnostic_metadata disc_meta){  
  return getDiscoveryFactDiagnosticMessage(disc_meta, DEVICE_ID, buildShortDevicePayload(DEVICE_NAME, getMAC()), AVAILABILITY_TOPIC, DIAGNOSTIC_TOPIC);
}

/*
  CPM       : counts (gamma rays) per minute
  frequency : CPM * 0.02 = Hertz (cycles per second)
  Dose      : micro-sieverts per hour (uSv/h) +/- error (uSv/h)  
*/
void ICACHE_FLASH_ATTR publishSensorData(){

// payload size is 91 characters
      std::string payload = "{\
\"frequency\": "+to_string((radiationWatch.cpm() * 0.02), "%2.4f")+", \
\"frequency_details\": {\"dose\": "+to_string(radiationWatch.uSvh(), "%3.2f")+", \
\"dose_err\": "+to_string(radiationWatch.uSvhError(), "%3.2f")+", \
\"cpm\": "+to_string(radiationWatch.cpm(), "%4.2f")+" }}";

      const char* payload_ch = payload.c_str();

      Serial.print(F("Publishing sensor readings: "));  
      Serial.println(payload_ch);

      mqttclient.publish(STATE_TOPIC.c_str(), payload_ch, NOT_RETAINED, QOS_0); 
}

void ICACHE_FLASH_ATTR publishDiagnosticData(){
      std::string payload = "{\
\"wifi_rssi\": "+to_string(getRSSI())+", \
\"wifi_ip\": \""+getIP()+"\", \
\"wifi_mac\": \""+getMAC()+"\" }";

      const char* payload_ch = payload.c_str();

      Serial.print(F("Publishing diagnostic readings: "));  
      Serial.println(payload_ch);

      mqttclient.publish(DIAGNOSTIC_TOPIC.c_str(), payload_ch, NOT_RETAINED, QOS_0);   
}

void ICACHE_FLASH_ATTR onRadiationPulse()
{
  digitalWrite(LED_BUILTIN, LED_ON);

  Serial.print(radiationWatch.uSvh());
  Serial.print(" uSv/h +/- ");
  Serial.println(radiationWatch.uSvhError());

  // Build MQTT payload and publish
  publishSensorData(); // radiationWatch is a global var  

  digitalWrite(LED_BUILTIN, LED_OFF);
}

#ifndef DISABLE_SERIAL_OUTPUT
void ICACHE_FLASH_ATTR onNoise()
{
  Serial.println("Noise!");
}
#endif

void ICACHE_FLASH_ATTR initRadiationWatch(){
  Serial.println(F("Initialize RadiationWatch sensor..."));
  radiationWatch.setup();
  // Register the callback
  radiationWatch.registerRadiationCallback(&onRadiationPulse); 
   
#ifndef DISABLE_SERIAL_OUTPUT
  radiationWatch.registerNoiseCallback(&onNoise);
#endif
}

void ICACHE_FLASH_ATTR messageReceived(String &topic, String &payload) {
  Serial.println("\nIncoming message: " + topic + " : " + payload);

  // Note: Do not use the mqttclient in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `mqttclient.loop()`.

  // Normally we only care about listening to the ".../refresh_rate/set" topic
  // However, early on we want to check the ".../refresh_rate/get" topic to see if a previously saved setting is available.
  // After checking, we will then unsubscribe from the ".../refresh_rate/get" topic. 
  // String set_rate_topic_str = String(SUB_RATE_TOPIC);
  // String get_rate_topic_str = String(PUB_RATE_TOPIC);  
  // if ((set_rate_topic_str.equals(topic)) || (get_rate_topic_str.equals(topic))){
  // }
}

void ICACHE_FLASH_ATTR indicateMQTTProblem(byte return_code){
  Serial.println(F("ERROR: MQTT problem!"));
}

// populate list of topics to subscribe to
std::vector<std::string> ICACHE_FLASH_ATTR getAllSubscriptionTopics(){  
  std::vector<std::string> topics = { }; 
  return topics;
}

// Runs until network and broker connectivity established and all subscriptions successful
// If network connectivity is lost and re-established, re-initialize the MQTT client
// If subscriptions fail, then will disconnect from broker, reconnect and try again.
void ICACHE_FLASH_ATTR assertConnectivity(){
  bool pass = false;
  bool subscription_required = false;
  bool mqtt_connected = false;  
  do {
    do{
      if(assertNetworkConnectivity(LOCAL_ENV_WIFI_SSID, LOCAL_ENV_WIFI_PASSWORD)){ // will block until connected, waiting WIFI_ATTEMPT_COOLDOWN between attempts 
        // new connection established, ASSUME need to re-initialize MQTT client
        initMQTTClient(LOCAL_ENV_MQTT_BROKER_HOST, LOCAL_ENV_MQTT_BROKER_PORT, AVAILABILITY_TOPIC.c_str());          
        mqttclient.disconnect();  // necessary after init?       
        subscription_required = true;  
        delay(100); // yield to allow for mqtt client activity post initialization
      }         
      mqtt_connected = connectMQTTBroker(DEVICE_ID, LOCAL_ENV_MQTT_USERNAME, LOCAL_ENV_MQTT_PASSWORD);
      if(!mqtt_connected){
        delay(MQTT_ATTEMPT_COOLDOWN);  // yield to allow for network and mqtt client activity between broker connection attempts   
      }
    } 
    while(!mqtt_connected); // will try once to connect to broker

    // at this point has network connection and broker connection

    if(subscription_required){ // only happens when MQTT client is (re)initialized due to network disconnect
      if(!subscribeTopics(getAllSubscriptionTopics())){
        // if there is a problem with subscribing to a topic, then disconnect from the broker and try again
        indicateMQTTProblem(MQTT_SUB_ERR);
        mqttclient.disconnect();              
      }
      else{        
        pass = true;
      }
    }
    else {
      pass = true;
    }    
  }
  while(!pass);
}

void ICACHE_FLASH_ATTR setup()
{
  #ifndef DISABLE_SERIAL_OUTPUT
  Serial.begin(9600);
  while (!Serial)
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens   
  #endif

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF); // initialize to off

  Serial.println(DEVICE_NAME);
  Serial.println(F("************************************"));

  initRadiationWatch();

  Serial.println(F("************************************"));

  // Connect to wifi & mqtt & subscribe
  assertConnectivity();  // Runs until network and broker connectivity established and all subscriptions successful
  printNetworkDetails();

  Serial.println(F("************************************"));

  // This metadata is assembled into HA-compatible discovery topics and payloads
  discovery_metadata_list = getAllDiscoveryMessagesMetadata(); 
  discovery_config_metadata_list = getAllDiscoveryConfigMessagesMetadata();
  discovery_measured_diagnostic_metadata_list = getAllDiscoveryMeasuredDiagnosticMessagesMetadata();
  discovery_fact_diagnostic_metadata_list = getAllDiscoveryFactDiagnosticMessagesMetadata();
  
  // Must successfully publish all discovery messages before proceding

  int discovery_messages_pending_publication;
  do {
    discovery_messages_pending_publication = publishDiscoveryMessages(); // Create the discovery messages and publish for each topic. Update published flag upon successful publication.
  }
  while(discovery_messages_pending_publication != 0);

  // no longer need discovery metadata, so purge it from memory
  purgeDiscoveryMetadata();

  // Publish availability online message (just once after all discovery messages have been successfully published)
  publishOnline(AVAILABILITY_TOPIC.c_str());
  publishDiagnosticData();  
}

unsigned long refresh_rate = 60000*5; // 5 minutes; frequency of sensor updates in milliseconds
unsigned long lastMillis = 0;
void ICACHE_FLASH_ATTR loop()
{
  radiationWatch.loop(); // potential call to onRadiationPulse(), onNoise()
  mqttclient.loop(); // potential call to messageReceived()
  assertConnectivity(); // Runs until network and broker connectivity established and all subscriptions successful

  if (millis() - lastMillis > refresh_rate) {   
    lastMillis = millis();
    
    publishOnline(AVAILABILITY_TOPIC.c_str());
    publishDiagnosticData();
  }
}