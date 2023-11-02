/****************************************************************************************************************************
  Async_ConfigOnDoubleReset.ino
  For Ethernet shields using ESP8266_W5500 (ESP8266 + LwIP W5500)

  WebServer_ESP8266_W5500 is a library for the ESP8266 with Ethernet W5500 to run WebServer

  Modified from
  1. Tzapu               (https://github.com/tzapu/WiFiManager)
  2. Ken Taylor          (https://github.com/kentaylor)
  3. Alan Steremberg     (https://github.com/alanswx/ESPAsyncWiFiManager)
  4. Khoi Hoang          (https://github.com/khoih-prog/ESPAsync_WiFiManager)

  Built by Khoi Hoang https://github.com/khoih-prog/AsyncESP8266_W5500_Manager
  Licensed under MIT license
 *****************************************************************************************************************************/
/****************************************************************************************************************************
   This example will open a configuration portal when the reset button is pressed twice.
   This method works well on Wemos boards which have a single reset button on board. It avoids using a pin for launching the configuration portal.

   Settings
   There are two values to be set in the sketch.

   DRD_TIMEOUT - Number of seconds to wait for the second reset. Set to 10 in the example.
   DRD_ADDRESS - The address in ESP8266 RTC RAM to store the flag. This memory must not be used for other purposes in the same sketch. Set to 0 in the example.

   This example, originally relied on the Double Reset Detector library from https://github.com/datacute/DoubleResetDetector
   To support ESP32, use ESP_DoubleResetDetector library from //https://github.com/khoih-prog/ESP_DoubleResetDetector
 *****************************************************************************************************************************/

//////////////////////////////////////////////////////////////
// Using GPIO4, GPIO16, or GPIO5
#define CSPIN             16
#define MQTT_HOST "nodered.local"
#define MQTT_PORT 1883
#define MQTT_USERNAME "laplacier"
#define MQTT_PASSWORD "laplacier"
#define MQTT_WILL_TOPIC "home/location/device/will"
#define NUM_PUZZLES 1
#define NUM_SENSORS 2
#define RESET_DELAY 10 // seconds

#define PUZZLE_SOLVED "99"
#define PUZZLE_READY "1"
#define PUZZLE_RESET "0"

const char *sub_topic[] = {
  "home/location/device/set",
};

const char *state_topic[] = {
  "home/location/device/state",
};

const char *input_topic[] = {
  "home/location/device/sensor1/state",
  "home/location/device/sensor2/state",
};

const char *set_message[]{
  "Puzzle forced open",
  "Puzzle forced closed",
  "Puzzle forced to reset",
};

const char *state_message[]{
  "Puzzle solved, box open",
  "Puzzle unsolved, box closed",
};
//////////////////////////////////////////////////////////

#if !( defined(ESP8266) )
  #error This code is intended to run on the (ESP8266 + W5500) platform! Please check your Tools->Board setting.
#endif

// Use from 0 to 4. Higher number, more debugging messages and memory usage.
#define _ESPASYNC_ETH_MGR_LOGLEVEL_    4

// To not display stored SSIDs and PWDs on Config Portal, select false. Default is true
// Even the stored Credentials are not display, just leave them all blank to reconnect and reuse the stored Credentials
//#define DISPLAY_STORED_CREDENTIALS_IN_CP        false

#include <FS.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <ESPAsyncDNSServer.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
Ticker puzzleResetRoutine;
bool flag_mqtt_online = true;
uint8_t game_state[NUM_PUZZLES];
uint16_t last_sensor_state[NUM_SENSORS];
bool flag_solved[NUM_PUZZLES];
bool flag_reset = false;
uint8_t resetCounter = 0;

void puzzle_init(void);
void checkSolution(void);
void puzzle_loop(void);
void reset_loop(void);
void resetCountdown(void);
void resumeGame(void);

const char *disconnectReason[]{
  "TCP Disconnected",
  "MQTT protocol version not accepted",
  "MQTT indentifier rejected",
  "MQTT server not found",
  "Malformed MQTT credentials",
  "MQTT connection not authorized",
  "ESP8266 out of space!!",
  "TLS bad fingerprint"
};

#define USE_LITTLEFS      true

#if USE_LITTLEFS
  #include <LittleFS.h>
  FS* filesystem =      &LittleFS;
  #define FileFS        LittleFS
  #define FS_Name       "LittleFS"
#else
  FS* filesystem =      &SPIFFS;
  #define FileFS        SPIFFS
  #define FS_Name       "SPIFFS"
#endif

//////////////////////////////////////////////////////////

#define ESP_getChipId()   (ESP.getChipId())

#define LED_ON      LOW
#define LED_OFF     HIGH

//////////////////////////////////////////////////////////

// These defines must be put before #include <ESP_DoubleResetDetector.h>
// to select where to store DoubleResetDetector's variable.
// For ESP32, You must select one to be true (EEPROM or SPIFFS)
// For ESP8266, You must select one to be true (RTC, EEPROM, SPIFFS or LITTLEFS)
// Otherwise, library will use default EEPROM storage

// These defines must be put before #include <ESP_DoubleResetDetector.h>
// to select where to store DoubleResetDetector's variable.
// For ESP32, You must select one to be true (EEPROM or SPIFFS)
// Otherwise, library will use default EEPROM storage

// For DRD
// These defines must be put before #include <ESP_DoubleResetDetector.h>
// to select where to store DoubleResetDetector's variable.
// For ESP8266, You must select one to be true (RTC, EEPROM, SPIFFS or LITTLEFS)
// Otherwise, library will use default EEPROM storage
#if USE_LITTLEFS
  #define ESP_DRD_USE_LITTLEFS    true
  #define ESP_DRD_USE_SPIFFS      false
#else
  #define ESP_DRD_USE_LITTLEFS    false
  #define ESP_DRD_USE_SPIFFS      true
#endif

#define ESP_DRD_USE_EEPROM      false
#define ESP8266_DRD_USE_RTC     false

#define DOUBLERESETDETECTOR_DEBUG       true  //false

#include <ESP_DoubleResetDetector.h>      //https://github.com/khoih-prog/ESP_DoubleResetDetector

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

//DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
DoubleResetDetector* drd;

//////////////////////////////////////////////////////////

// Onboard LED I/O pin on NodeMCU board
const int PIN_LED = 2; // D4 on NodeMCU and WeMos. GPIO2/ADC12 of ESP32. Controls the onboard LED.

// You only need to format the filesystem once
//#define FORMAT_FILESYSTEM       true
#define FORMAT_FILESYSTEM         false

//////////////////////////////////////////////////////////

// Assuming max 49 chars
#define TZNAME_MAX_LEN            50
#define TIMEZONE_MAX_LEN          50

typedef struct
{
  char TZ_Name[TZNAME_MAX_LEN];     // "America/Toronto"
  char TZ[TIMEZONE_MAX_LEN];        // "EST5EDT,M3.2.0,M11.1.0"
  uint16_t checksum;
} EthConfig;

EthConfig         Ethconfig;

#define  CONFIG_FILENAME              F("/eth_cred.dat")

//////////////////////////////////////////////////////////

// Indicates whether ESP has credentials saved from previous session, or double reset detected
bool initialConfig = false;

// Use false if you don't like to display Available Pages in Information Page of Config Portal
// Comment out or use true to display Available Pages in Information Page of Config Portal
// Must be placed before #include <AsyncESP8266_W5500_Manager.h>
#define USE_AVAILABLE_PAGES     true  //false

// To permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
// You'll loose the feature of dynamically changing from DHCP to static IP, or vice versa
// You have to explicitly specify false to disable the feature.
#define USE_STATIC_IP_CONFIG_IN_CP          false

// Use false to disable NTP config. Advisable when using Cellphone, Tablet to access Config Portal.
// See Issue 23: On Android phone ConfigPortal is unresponsive (https://github.com/khoih-prog/ESP_WiFiManager/issues/23)
#define USE_ESP_ETH_MANAGER_NTP     false

// Just use enough to save memory. On ESP8266, can cause blank ConfigPortal screen
// if using too much memory
#define USING_AFRICA        false
#define USING_AMERICA       true
#define USING_ANTARCTICA    false
#define USING_ASIA          false
#define USING_ATLANTIC      false
#define USING_AUSTRALIA     false
#define USING_EUROPE        false
#define USING_INDIAN        false
#define USING_PACIFIC       false
#define USING_ETC_GMT       false

// Use true to enable CloudFlare NTP service. System can hang if you don't have Internet access while accessing CloudFlare
// See Issue #21: CloudFlare link in the default portal (https://github.com/khoih-prog/ESP_WiFiManager/issues/21)
#define USE_CLOUDFLARE_NTP          false

#define USING_CORS_FEATURE          true

////////////////////////////////////////////

// Use USE_DHCP_IP == true for dynamic DHCP IP, false to use static IP which you have to change accordingly to your network
#if (defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP)
  // Force DHCP to be true
  #if defined(USE_DHCP_IP)
    #undef USE_DHCP_IP
  #endif
  #define USE_DHCP_IP     true
#else
  // You can select DHCP or Static IP here
  #define USE_DHCP_IP     true
  //#define USE_DHCP_IP     false
#endif

#if ( USE_DHCP_IP )
  // Use DHCP

  #if (_ESPASYNC_ETH_MGR_LOGLEVEL_ > 3)
    #warning Using DHCP IP
  #endif

  IPAddress stationIP   = IPAddress(0, 0, 0, 0);
  IPAddress gatewayIP   = IPAddress(192, 168, 1, 1);
  IPAddress netMask     = IPAddress(255, 255, 255, 0);

#else
  // Use static IP

  #if (_ESPASYNC_ETH_MGR_LOGLEVEL_ > 3)
    #warning Using static IP
  #endif

  IPAddress stationIP   = IPAddress(192, 168, 1, 186);
  IPAddress gatewayIP   = IPAddress(192, 168, 1, 1);
  IPAddress netMask     = IPAddress(255, 255, 255, 0);
#endif

//////////////////////////////////////////////////////////

#define USE_CONFIGURABLE_DNS      true

IPAddress dns1IP      = gatewayIP;
IPAddress dns2IP      = IPAddress(8, 8, 8, 8);

#include <AsyncESP8266_W5500_Manager.h>               //https://github.com/khoih-prog/AsyncESP8266_W5500_Manager

#define HTTP_PORT     80

//////////////////////////////////////////////////////////

/******************************************
   // Defined in AsyncESP8266_W5500_Manager.hpp
  typedef struct
  {
    IPAddress _sta_static_ip;
    IPAddress _sta_static_gw;
    IPAddress _sta_static_sn;
    #if USE_CONFIGURABLE_DNS
    IPAddress _sta_static_dns1;
    IPAddress _sta_static_dns2;
    #endif
  }  ETH_STA_IPConfig;
******************************************/

ETH_STA_IPConfig EthSTA_IPconfig;

//////////////////////////////////////////////////////////

void initSTAIPConfigStruct(ETH_STA_IPConfig &in_EthSTA_IPconfig)
{
  in_EthSTA_IPconfig._sta_static_ip   = stationIP;
  in_EthSTA_IPconfig._sta_static_gw   = gatewayIP;
  in_EthSTA_IPconfig._sta_static_sn   = netMask;
#if USE_CONFIGURABLE_DNS
  in_EthSTA_IPconfig._sta_static_dns1 = dns1IP;
  in_EthSTA_IPconfig._sta_static_dns2 = dns2IP;
#endif
}

//////////////////////////////////////////////////////////

void displayIPConfigStruct(ETH_STA_IPConfig in_EthSTA_IPconfig)
{
  LOGERROR3(F("stationIP ="), in_EthSTA_IPconfig._sta_static_ip, ", gatewayIP =", in_EthSTA_IPconfig._sta_static_gw);
  LOGERROR1(F("netMask ="), in_EthSTA_IPconfig._sta_static_sn);
#if USE_CONFIGURABLE_DNS
  LOGERROR3(F("dns1IP ="), in_EthSTA_IPconfig._sta_static_dns1, ", dns2IP =", in_EthSTA_IPconfig._sta_static_dns2);
#endif
}

//////////////////////////////////////////////////////////

#if USE_ESP_ETH_MANAGER_NTP
void printLocalTime()
{
  static time_t now;

  now = time(nullptr);

  if ( now > 1451602800 )
  {
    Serial.print("Local Date/Time: ");
    Serial.print(ctime(&now));
  }
}
#endif

//////////////////////////////////////////////////////////

void heartBeatPrint()
{
#if USE_ESP_ETH_MANAGER_NTP
  printLocalTime();
#else
  static int num = 1;

  if (eth.connected())
    Serial.print(F("H"));        // H means connected to Ethernet
  else
    Serial.print(F("F"));        // F means not connected to Ethernet
  if (num == 80)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(F(" "));
  }

#endif
}

//////////////////////////////////////////////////////////

void check_status()
{
  static ulong checkstatus_timeout  = 0;

  static ulong current_millis;

#if USE_ESP_ETH_MANAGER_NTP
#define HEARTBEAT_INTERVAL    60000L
#else
#define HEARTBEAT_INTERVAL    10000L
#endif

  current_millis = millis();

  // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
  if ((current_millis > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    heartBeatPrint();
    checkstatus_timeout = current_millis + HEARTBEAT_INTERVAL;
  }
}

//////////////////////////////////////////////////////////

int calcChecksum(uint8_t* address, uint16_t sizeToCalc)
{
  uint16_t checkSum = 0;

  for (uint16_t index = 0; index < sizeToCalc; index++)
  {
    checkSum += * ( ( (byte*) address ) + index);
  }

  return checkSum;
}

//////////////////////////////////////////////////////////

bool loadConfigData()
{
  File file = FileFS.open(CONFIG_FILENAME, "r");
  LOGERROR(F("LoadCfgFile "));

  memset((void *) &Ethconfig,       0, sizeof(Ethconfig));
  memset((void *) &EthSTA_IPconfig, 0, sizeof(EthSTA_IPconfig));

  if (file)
  {
    file.readBytes((char *) &Ethconfig,   sizeof(Ethconfig));
    file.readBytes((char *) &EthSTA_IPconfig, sizeof(EthSTA_IPconfig));
    file.close();

    LOGERROR(F("OK"));

    if ( Ethconfig.checksum != calcChecksum( (uint8_t*) &Ethconfig, sizeof(Ethconfig) - sizeof(Ethconfig.checksum) ) )
    {
      LOGERROR(F("Ethconfig checksum wrong"));

      return false;
    }

    displayIPConfigStruct(EthSTA_IPconfig);

    return true;
  }
  else
  {
    LOGERROR(F("failed"));

    return false;
  }
}

//////////////////////////////////////////////////////////

void saveConfigData()
{
  File file = FileFS.open(CONFIG_FILENAME, "w");
  LOGERROR(F("SaveCfgFile "));

  if (file)
  {
    Ethconfig.checksum = calcChecksum( (uint8_t*) &Ethconfig, sizeof(Ethconfig) - sizeof(Ethconfig.checksum) );

    file.write((uint8_t*) &Ethconfig, sizeof(Ethconfig));

    displayIPConfigStruct(EthSTA_IPconfig);

    file.write((uint8_t*) &EthSTA_IPconfig, sizeof(EthSTA_IPconfig));
    file.close();

    LOGERROR(F("OK"));
  }
  else
  {
    LOGERROR(F("failed"));
  }
}

//////////////////////////////////////////////////////////

void initEthernet()
{
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  LOGWARN(F("Default SPI pinout:"));
  LOGWARN1(F("MOSI:"), MOSI);
  LOGWARN1(F("MISO:"), MISO);
  LOGWARN1(F("SCK:"),  SCK);
  LOGWARN1(F("CS:"),   CSPIN);
  LOGWARN(F("========================="));

#if !USING_DHCP
  //eth.config(localIP, gateway, netMask, gateway);
  eth.config(EthSTA_IPconfig._sta_static_ip, EthSTA_IPconfig._sta_static_gw, EthSTA_IPconfig._sta_static_sn,
             EthSTA_IPconfig._sta_static_dns1);
#endif

  eth.setDefault();

  if (!eth.begin())
  {
    Serial.println("No Ethernet hardware ... Stop here");

    while (true)
    {
      delay(1000);
    }
  }
  else
  {
    Serial.print("Connecting to network : ");

    if (!eth.connected())
    {
      Serial.print(".");
      //delay(1000);
    }
  }

  Serial.println();

#if USING_DHCP
  Serial.print("Ethernet DHCP IP address: ");
#else
  Serial.print("Ethernet Static IP address: ");
#endif

  Serial.println(eth.localIP());

}

//////////////////////////////////////////////////////////

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  for(int i=0; i<NUM_PUZZLES; i++){
    // Subscribe
    mqttClient.subscribe(sub_topic[i], 2);

    // Publish game state
    String a = String(game_state[i]);
    if(game_state[i] < 10){
      char message[1];
      a.toCharArray(message, 2);
      mqttClient.publish(state_topic[i], 1, true, message);
    }
    else{
      char message[2];
      a.toCharArray(message, 3); 
      mqttClient.publish(state_topic[i], 1, true, message);
    }
  }

  // Update last will
  mqttClient.publish(MQTT_WILL_TOPIC, 1, true, "online");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.print("Disconnected from MQTT. Reason: ");
  Serial.println(disconnectReason[(uint8_t)reason]);
  mqttReconnectTimer.once(2, connectToMqtt);
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
}

void onMqttUnsubscribe(uint16_t packetId) {
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  String message;
  String strTopic;
  for(int i = 0; i < len; i++) {
    message += (char)payload[i];
  }
  for(int i=0; i<strlen(topic); i++){
    strTopic += (char)topic[i];
  }
  Serial.print("Message received: ");
  Serial.println(message);
  for(int i=0; i<NUM_PUZZLES; i++){
    if(strTopic == sub_topic[i] && message == PUZZLE_SOLVED){
      mqttClient.publish(state_topic[i], 1, true, PUZZLE_SOLVED);
      Serial.println(set_message[0]);
      game_state[i] = atoi(PUZZLE_SOLVED);
      flag_reset = false;
      puzzleResetRoutine.detach();
    }
    else if(strTopic == sub_topic[i] && message == PUZZLE_READY){
      mqttClient.publish(state_topic[i], 1, true, PUZZLE_READY);
      Serial.println(set_message[1]);
      game_state[i] = atoi(PUZZLE_READY);
      flag_reset = false;
      puzzleResetRoutine.detach();
    }
  }
  if(message == PUZZLE_RESET){
    for(int i=0; i<NUM_PUZZLES; i++){
      mqttClient.publish(state_topic[i], 1, true, PUZZLE_RESET);
      game_state[i] = atoi(PUZZLE_RESET);
    }
    Serial.println(set_message[2]);
    flag_reset = true;
    resetCounter = RESET_DELAY;
    puzzleResetRoutine.detach();
    puzzleResetRoutine.attach(1, resetCountdown);
  }
}

void onMqttPublish(uint16_t packetId) {
}

void resetCountdown(void){
  if(resetCounter <= 0){
    Serial.println("Reset complete!");
    flag_reset = false;
    puzzleResetRoutine.detach();
    for(int i=0; i<NUM_PUZZLES; i++){
      mqttClient.publish(state_topic[i], 1, true, PUZZLE_READY);
    }
    resumeGame();
  }
  else{
    Serial.print("Resuming in ");
    Serial.print(resetCounter);
    Serial.println("...");
    resetCounter--;
  }
}

void resumeGame(void){
  flag_reset = false;
  for(int i=0; i<NUM_PUZZLES; i++){
    game_state[i] = atoi(PUZZLE_READY);
  }
}

void setup()
{
  // put your setup code here, to run once:
  // initialize the LED digital pin as an output.
  pinMode(PIN_LED, OUTPUT);

  puzzle_init();
  
  for(int i=0; i<NUM_PUZZLES; i++){
    game_state[i] = 1;
    flag_solved[i] = false;
  }
  for(int i=0; i<NUM_SENSORS; i++){
    last_sensor_state[i] = 0;
  }

  Serial.begin(115200);

  while (!Serial && millis() < 5000);

  delay(200);

  Serial.print(F("\nStarting Async_ConfigOnDoubleReset using "));
  Serial.print(FS_Name);
  Serial.print(F(" on "));
  Serial.print(ARDUINO_BOARD);
  Serial.print(F(" with "));
  Serial.println(SHIELD_TYPE);
  Serial.println(ASYNC_ESP8266_W5500_MANAGER_VERSION);
  Serial.println(ESP_DOUBLE_RESET_DETECTOR_VERSION);

  Serial.setDebugOutput(false);

#if FORMAT_FILESYSTEM
  Serial.println(F("Forced Formatting."));
  FileFS.format();
#endif

  // Format FileFS if not yet
  if (!FileFS.begin())
  {
    FileFS.format();

    Serial.println(F("SPIFFS/LittleFS failed! Already tried formatting."));

    if (!FileFS.begin())
    {
      // prevents debug info from the library to hide err message.
      delay(100);

#if USE_LITTLEFS
      Serial.println(F("LittleFS failed!. Please use SPIFFS or EEPROM. Stay forever"));
#else
      Serial.println(F("SPIFFS failed!. Please use LittleFS or EEPROM. Stay forever"));
#endif

      while (true)
      {
        delay(1);
      }
    }
  }

  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);

  unsigned long startedAt = millis();

  initSTAIPConfigStruct(EthSTA_IPconfig);

  //Local intialization. Once its business is done, there is no need to keep it around
  // Use this to default DHCP hostname to ESP8266-XXXXXX
  //AsyncESP8266_W5500_Manager AsyncESP8266_W5500_manager(&webServer, &dnsServer);
  // Use this to personalize DHCP hostname (RFC952 conformed)
  AsyncWebServer webServer(HTTP_PORT);

  AsyncDNSServer dnsServer;

  AsyncESP8266_W5500_Manager AsyncESP8266_W5500_manager(&webServer, &dnsServer, "AsyncConfigOnDoubleReset");

  //////////////////////////////////////////////////////////

  // Inject custom parameters into config page for MQTT credentials
  char mqttHost[30] = MQTT_HOST;
  uint16_t mqttPort = 1883;
  char mqttUser[30] = MQTT_USERNAME;
  char mqttPass[30] = MQTT_PASSWORD;
  
  ESPAsync_EMParameter p_mqttHost("mqttHost", "MQTT Host/IP", mqttHost, 30);
  
  char convertedValue[5];
  sprintf(convertedValue, "%d", mqttPort);
  ESPAsync_EMParameter p_mqttPort("mqttPort", "MQTT Port", convertedValue, 5);
  
  ESPAsync_EMParameter p_mqttUser("mqttUser", "MQTT Username", mqttUser, 30);
  ESPAsync_EMParameter p_mqttPass("mqttPass", "MQTT Password", mqttPass, 30);
  
  AsyncESP8266_W5500_manager.addParameter(&p_mqttHost);
  AsyncESP8266_W5500_manager.addParameter(&p_mqttPort);
  AsyncESP8266_W5500_manager.addParameter(&p_mqttUser);
  AsyncESP8266_W5500_manager.addParameter(&p_mqttPass);

  ///////////////////////////////////////////////////////////

#if !USE_DHCP_IP
  // Set (static IP, Gateway, Subnetmask, DNS1 and DNS2) or (IP, Gateway, Subnetmask)
  AsyncESP8266_W5500_manager.setSTAStaticIPConfig(EthSTA_IPconfig);
#endif

#if USING_CORS_FEATURE
  AsyncESP8266_W5500_manager.setCORSHeader("Your Access-Control-Allow-Origin");
#endif

  bool configDataLoaded = false;

  if (loadConfigData())
  {
    configDataLoaded = true;

    //If no access point name has been previously entered disable timeout
    AsyncESP8266_W5500_manager.setConfigPortalTimeout(120);

    Serial.println(F("Got stored Credentials. Timeout 120s for Config Portal"));

#if USE_ESP_ETH_MANAGER_NTP

    if ( strlen(Ethconfig.TZ_Name) > 0 )
    {
      LOGERROR3(F("Current TZ_Name ="), Ethconfig.TZ_Name, F(", TZ = "), Ethconfig.TZ);

      configTime(Ethconfig.TZ, "pool.ntp.org");
    }
    else
    {
      Serial.println(F("Current Timezone is not set. Enter Config Portal to set."));
    }

#endif
  }
  else
  {
    // Enter CP only if no stored Credentials on flash and file
    Serial.println(F("Open Config Portal without Timeout: No stored Credentials."));
    initialConfig = true;
  }

  //////////////////////////////////

  // Connect ETH now if using STA
  initEthernet();

  //////////////////////////////////

  if (drd->detectDoubleReset())
  {
    // DRD, disable timeout.
    AsyncESP8266_W5500_manager.setConfigPortalTimeout(0);

    Serial.println(F("Open Config Portal without Timeout: Double Reset Detected"));
    initialConfig = true;
  }

  if (initialConfig)
  {
    Serial.print(F("Starting configuration portal @ "));
    Serial.println(eth.localIP());

    digitalWrite(PIN_LED, LED_ON); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.

    //sets timeout in seconds until configuration portal gets turned off.
    //If not specified device will remain in configuration mode until
    //switched off via webserver or device is restarted.
    //AsyncESP8266_W5500_manager.setConfigPortalTimeout(600);

    // Starts an access point
    if (!AsyncESP8266_W5500_manager.startConfigPortal())
      Serial.println(F("Not connected to ETH network but continuing anyway."));
    else
    {
      Serial.println(F("ETH network connected...yeey :)"));
    }

#if USE_ESP_ETH_MANAGER_NTP
    String tempTZ   = AsyncESP8266_W5500_manager.getTimezoneName();

    if (strlen(tempTZ.c_str()) < sizeof(Ethconfig.TZ_Name) - 1)
      strcpy(Ethconfig.TZ_Name, tempTZ.c_str());
    else
      strncpy(Ethconfig.TZ_Name, tempTZ.c_str(), sizeof(Ethconfig.TZ_Name) - 1);

    const char * TZ_Result = AsyncESP8266_W5500_manager.getTZ(Ethconfig.TZ_Name);

    if (strlen(TZ_Result) < sizeof(Ethconfig.TZ) - 1)
      strcpy(Ethconfig.TZ, TZ_Result);
    else
      strncpy(Ethconfig.TZ, TZ_Result, sizeof(Ethconfig.TZ_Name) - 1);

    if ( strlen(Ethconfig.TZ_Name) > 0 )
    {
      LOGERROR3(F("Saving current TZ_Name ="), Ethconfig.TZ_Name, F(", TZ = "), Ethconfig.TZ);

      configTime(Ethconfig.TZ, "pool.ntp.org");
    }
    else
    {
      LOGERROR(F("Current Timezone Name is not set. Enter Config Portal to set."));
    }

#endif

    AsyncESP8266_W5500_manager.getSTAStaticIPConfig(EthSTA_IPconfig);
    
    saveConfigData();

#if !USE_DHCP_IP

    // Reset to use new Static IP, if different from current eth.localIP()
    if (eth.localIP() != EthSTA_IPconfig._sta_static_ip)
    {
      Serial.print(F("Current IP = "));
      Serial.print(eth.localIP());
      Serial.print(F(". Reset to take new IP = "));
      Serial.println(EthSTA_IPconfig._sta_static_ip);

      ESP.reset();
      delay(2000);
    }

#endif
  }

  digitalWrite(PIN_LED, LED_OFF); // Turn led off as we are not in configuration mode.

  startedAt = millis();

  Serial.print(F("After waiting "));
  Serial.print((float) (millis() - startedAt) / 1000);
  Serial.print(F(" secs more in setup(), connection result is "));

  if (eth.connected())
  {
    Serial.print(F("connected. Local IP: "));
    Serial.println(eth.localIP());
  }

  // Getting posted form values and overriding local variables parameters
  // Config file is written regardless the connection state
  strcpy(mqttHost, p_mqttHost.getValue());
  mqttPort = atoi(p_mqttPort.getValue());
  strcpy(mqttUser, p_mqttUser.getValue());
  strcpy(mqttPass, p_mqttPass.getValue());

  Serial.print("MQTT Host: ");
  Serial.println(mqttHost);
  Serial.print("MQTT Port: ");
  Serial.println(mqttPort);
  Serial.print("MQTT Username: ");
  Serial.println(mqttUser);
  Serial.print("MQTT Password: ");
  Serial.println(mqttPass);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  //mqttClient.setServer(mqttHost, mqttPort);
  //mqttClient.setCredentials(mqttUser, mqttPass);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USERNAME, MQTT_PASSWORD);
  mqttClient.setWill(MQTT_WILL_TOPIC, 2, true, "offline");
  mqttClient.setKeepAlive(15);
  mqttClient.setCleanSession(true);

  delay(2000);
  connectToMqtt();
}

void loop()
{
  // Call the double reset detector loop method every so often,
  // so that it can recognise when the timeout expires.
  // You can also call drd.stop() when you wish to no longer
  // consider the next reset as a double reset.
  drd->loop();

  // put your main code here, to run repeatedly
  check_status();
  if(!flag_reset){
    puzzle_loop();
  }
  else{
    reset_loop();
  }
}
