#include <Arduino.h>

const char *ssid = "";
const char *password = "";

#define HEALTHCHECKER_INTERVAL 86400000
String HEALTHCHECKER_ENDPOINT = "https://api.telegram.org/<BOTTOKEN>/sendMessage?chat_id=<CHATID>&text=";
