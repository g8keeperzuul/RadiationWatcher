#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <string>
#include <ESP8266WiFi.h>

// *********************************************************************************************************************
// *** Must Declare ***
//extern WiFiClient wificlient;

// *** Must Implement ***
bool connectWifi(const char *ssid, const char *passphrase);
bool assertNetworkConnectivity(const char *ssid, const char *passphrase);
void printNetworkDetails();
std::string getMAC();
std::string getIP();
int getRSSI();

#endif