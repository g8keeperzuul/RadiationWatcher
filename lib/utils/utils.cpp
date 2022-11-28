/*
    Contains application generic functions (mostly string-manipulation),
    especially for HA-specific MQTT topic and discovery payload formats.
    Intent is to be able to copy this unchanged to similar new projects.
*/
#include <cstdio>
#include <cassert>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "utils.h"


#ifdef DISABLE_SERIAL_OUTPUT
// disable Serial output
// https://arduino.stackexchange.com/questions/9857/can-i-make-the-arduino-ignore-serial-print
#define Serial SomeOtherwiseUnusedName
static class {
public:
    void begin(...) {}
    void print(...) {}
    void println(...) {}
    void printf(...) {}
} Serial;
#endif


/*
// https://stackoverflow.com/questions/4643512/replace-substring-with-another-substring-c
void replaceStringInline(std::string& subject, const std::string& search, const std::string& replace) {
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}
*/

/**
 * @brief Given a map, build a json string.
 * 
  std::map <std::string,std::string> payload;
  payload.insert({"device_class","\"temperature\""});
  payload.insert({"unit_of_measurement","\"°C\""});
  payload.insert({"availability_topic", ""});
  payload.insert({"state_class", "\"measurement\""});
  payload.insert({"expire_after", "3600"});
  payload.insert({"force_update", "true"});
  payload.insert({"unique_id", ""});
  payload.insert({"device", ""});
  payload.insert({"name", ""});
  payload.insert({"icon", ""});
  payload.insert({"state_topic", ""});
  payload.insert({"value_template", "\"{{ value_json.temperature }}\""});

  std::map <std::string,std::string> payload = {{"device_class","\"temperature\""},{"unit_of_measurement","\"°C\""}};

  Remove unneeded key from template:
  payload.erase("value_template");

  To replace value at key:
  std::map<std::string, std::string>::iterator itr = payload.find("icon");  OR auto itr = payload.find("icon");
  if (itr != payload.end())
    itr->second = "\"new-value\"";
 * 
 * @param payload Map of keys and values needed for json MQTT payload message. 
 * Quotes are not required for the keys, but are required for values to be interpreted as JSON strings.
 * @return std::string MQTT payload message for discovery config message
 */
/*
std::string map2json(std::map<std::string,std::string> kvp){
  
  std::string payload = "{";
  for (std::map<std::string,std::string>::iterator itr = kvp.begin(); itr != kvp.end(); ) {
    payload = payload+"\""+(itr->first)+"\":"+(itr->second);
    if (++itr != kvp.end())
      payload = payload+",";
  }
  payload = payload+"}";
  return payload;
}
*/
/**
 * @brief Replaces value at key in given map.
 * 
 * @param m A map of string keys and values
 * @param k A string key that exists
 * @param v A string value to replace existing value at key
 */
/*
void replace_value_at_key(std::map<std::string,std::string> m, std::string k, std::string v){
    std::map<std::string, std::string>::iterator itr = m.find(k);  // OR auto itr = m.find(k);
    if (itr != m.end())
    itr->second = v;
}
*/

/*
 * double to string (char array)
 */
/*
char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  uint32_t iPart = (uint32_t)val;
  sprintf(sout, "%d", iPart);
    
  if (prec > 0) {
    uint8_t pos = strlen(sout);
    sout[pos++] = '.';
    uint32_t dPart = (uint32_t)((val - (double)iPart) * pow(10, prec));

    for (uint8_t i = (prec - 1); i > 0; i--) {
      size_t pow10 = pow(10, i);
      if (dPart < pow10) {
        sout[pos++] = '0';
      }
      else {
        sout[pos++] = '0' + dPart / pow10;
        dPart = dPart % pow10;
      }
    }

    sout[pos++] = '0' + dPart;
    sout[pos] = '\0';
  }
  
  return sout;
}  
*/

// https://stackoverflow.com/questions/4668760/converting-an-int-to-stdstring
std::string to_string( int x ) {
  int length = snprintf( NULL, 0, "%d", x );
  assert( length >= 0 );
  char* buf = new char[length + 1];
  snprintf( buf, length + 1, "%d", x );
  std::string str( buf );
  delete[] buf;
  return str;
}

/**
 * @brief Convert a float to a std::string
 * 
 * @param x float to be formatted and converted to std::string
 * @param format Should be related to float like "%4.2f" or "%3.1g"
 * @return std::string 
 */
std::string to_string( float x, const char* format) {
  int length = snprintf( NULL, 0, format, x );
  assert( length >= 0 );
  char* buf = new char[length + 1];
  snprintf( buf, length + 1, format, x );
  std::string str( buf );
  delete[] buf;
  return str;
}

// https://gist.github.com/miguelmota/4fc9b46cf21111af5fa613555c14de92
std::string uint8_to_hex_string(const uint8_t *v, const size_t s) {
  std::stringstream ss;

  ss << std::hex << std::setfill('0');

  for (unsigned int i = 0; i < s; i++) {
    if(i>0){ ss << ":"; }    
    ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
  }

  return ss.str();
}

std::string uint32_to_ip(uint32_t ip_as_int)
{
    std::stringstream ss;    
    ss << (ip_as_int & 0xff);
    ss << ".";
    ss << ((ip_as_int >> 8) & 0xff);    
    ss << ".";
    ss << ((ip_as_int >> 16) & 0xff);
    ss << ".";
    ss << (ip_as_int >> 24);
    return ss.str();
}

/**
  Return true if 'test' equals or exceeds 'percent_threshold' difference from 'last'
*/
bool sufficientChange(uint16_t test, uint16_t last, float percent_threshold){
  return (abs(1 - (test/last)) >= percent_threshold); 
}


#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

/*
  Works on AVR and ARM (M0) processors.
  
  What freeMemory() is actually reporting is the space between the heap and the stack. 
  It does not report any de-allocated memory that is buried in the heap. 
  Buried heap space is not usable by the stack, and may be fragmented enough that it is 
  not usable for many heap allocations either. The space between the heap and the stack 
  is what you really need to monitor if you are trying to avoid stack crashes.

  https://learn.adafruit.com/memories-of-an-arduino/measuring-free-memory
*/
int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

        /*
        Example of checking addresses of variables:

        Serial.printf("\tTopic: %s\n", dconf.topic.c_str());
        //Serial.printf("\tPayload: %s\n", dconf.payload.c_str());
        Serial.printf("\tPublished: %s\n",  btoa(dconf.published));
        discovery_config* dconf_ptr = &dconf;
        Serial.printf("\taddress of discovery_messages[%d] is: 0x%0X\n", i, (unsigned)&dconf_ptr);
        bool* published_ptr = &(dconf.published);
        Serial.printf("\taddress of [%d].published is: 0x%0X\n", i, (unsigned)&published_ptr);
        //bool** pub_ptr = &pub;
        //Serial.printf("address of pointer to pointer is: 0x%0X\n", (unsigned)&pub_ptr);
        Serial.println(" ");
        */