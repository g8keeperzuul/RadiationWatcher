#ifndef UTILS_H
#define UTILS_H

#include <string>
//#include <map>

#define btoa(x) ((x)?"true":"false")

#ifdef DISABLE_SERIAL_OUTPUT
#define logRAM()
#define logRAMlabel(x)
#else
#define logRAM() Serial.printf("RAM: %d bytes\n", freeMemory())
#define logRAMlabel(x) Serial.print(x); Serial.printf(" RAM: %d bytes\n", freeMemory())
#endif


//void replaceStringInline(std::string& subject, const std::string& search, const std::string& replace);
//std::string map2json(std::map<std::string,std::string> kvp);
//void replace_value_at_key(std::map<std::string,std::string> m, std::string k, std::string v);
std::string to_string( float x, const char* format );
std::string to_string( int x );
//char *dtostrf (double val, signed char width, unsigned char prec, char *sout);
std::string uint8_to_hex_string(const uint8_t *v, const size_t s);
std::string uint32_to_ip(uint32_t ip_as_int);

bool sufficientChange(uint16_t test, uint16_t last, float percent_threshold);

int freeMemory();

#endif