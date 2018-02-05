#define ARDUINOJSON_ENABLE_PROGMEM 1
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ArduinoJson.h>

#include "SH1106Brzo.h"
#include <WiFiClientSecure.h>

#include <ESP8266WiFi.h>
#include "ESP8266HTTPClient.h"

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic


// ********* Bank Setup ****************
const char hostFingerprint[] = "3F:CF:55:F4:03:2D:9A:C5:85:E3:C8:61:94:64:85:05:DC:71:70:32";
const int   httpsPort     = 443;
//TODO: Add to configurable params?
const char tokenAddress[]  = "https://api.sbanken.no/identityserver/connect/token";
const char accountsAddress[] = "https://api.sbanken.no/Bank/api/v1/Accounts/";

int tokenCount = 0;
bool validToken = false;
char token[1500];
char disposable[32];
char accountname[32];
char appKey[33];
char appPassword[100];
char userID[12];
char accountName[30];

SH1106Brzo  display(0x3c, D3, D5);

void setup() {
  Serial.begin(115200);

    if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(appKey, json["appKey"]);
          strcpy(appPassword, json["appPassword"]);
          strcpy(userID, json["userID"]);
          strcpy(accountName, json["accountname"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }

  
  display.init();
  display.flipScreenVertically();

  WiFiManager wifiManager;
  //wifiManager.resetSettings (); //debug
  wifiManager.setAPCallback(configModeCallback);
  WiFiManagerParameter appkey("app-key", "app key", appKey, 33);
  wifiManager.addParameter(&appkey);
  WiFiManagerParameter apppassword("app-password", "app password", appPassword, 100);
  wifiManager.addParameter(&apppassword);
  WiFiManagerParameter userid("app-userid", "app userid", userID, 12);
  wifiManager.addParameter(&userid);
  WiFiManagerParameter accountname("app-accountname", "app accountname", accountName, 30);
  wifiManager.addParameter(&accountname);

  //first parameter is name of access point, second is the password
  wifiManager.autoConnect();

  strcpy(appKey, appkey.getValue());
  strcpy(appPassword, apppassword.getValue());
  strcpy(userID, userid.getValue());
  strcpy(accountName, accountname.getValue());

    //save the custom parameters to FS
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["appKey"] = appKey;
    json["appPassword"] = appPassword;
    json["userID"] = userID;
    json["accountname"] = accountName;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  showOnDisplay(WiFi.softAPIP().toString(),ArialMT_Plain_16);
}

bool getToken() {
  HTTPClient http;
   Serial.println(tokenAddress);
   Serial.println(hostFingerprint);
  bool beginCode = http.begin(tokenAddress, hostFingerprint);
  if (!beginCode) {
    Serial.println(beginCode);
    return false;
  }
  Serial.println(beginCode);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");
  http.setAuthorization(appKey,appPassword);
  int httpCode = http.POST("grant_type=client_credentials");

  if (httpCode != HTTP_CODE_OK) {
    Serial.println("HTTP Post error: ");
    Serial.println(httpCode);
    return false;
  }
  DynamicJsonBuffer jsonBuffer(2000);
  JsonObject& root = jsonBuffer.parseObject(http.getString());
  if(!root.containsKey("access_token")){
    Serial.println("Response does not contain key");
    return false;
  }
  strcpy(&token[0], "Bearer ");
  strcpy(&token[7],root["access_token"]);
  
  http.end();
  return true;


}
void showOnDisplay(String text, const char* fontData){
    display.clear();
    display.setFont(fontData);
    // The coordinates define the center of the text
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, text);
    display.display();
}


bool getDisposable() {
    HTTPClient http;
    String addr = accountsAddress;
    addr += userID;
    Serial.println(addr);
    bool beginCode = http.begin(addr, hostFingerprint);
    if (!beginCode) {
    Serial.println(beginCode);
    return false;
    }
    http.setAuthorization(token);
    http.addHeader("Accept", "application/json");
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
     Serial.println("HTTP Post error: ");
     Serial.println(httpCode);
     return false;
    }
    Serial.println(httpCode);
  
    DynamicJsonBuffer jsonBuffer;
    JsonObject& accounts = jsonBuffer.parseObject(http.getString()); //root;
    int noAccounts = atoi(accounts["availableItems"]);
    char accountNames[30];
    for(int i=0;i<noAccounts;i++){
      strcpy(accountNames, accounts["items"][i]["name"]);
      if(String(accountNames) == String(accountName)){
        strcpy(disposable, accounts["items"][i]["available"]);
      }
    }
    Serial.println(accountname);
    Serial.println(disposable);
    int disp = atof(disposable);
    showOnDisplay(String(disp) + " kr",ArialMT_Plain_24);
    http.end();
    return true;
}


void loop() {
  if(!validToken){
    if(getToken()){
      Serial.println("Got token: ");
      Serial.println(token);
      validToken = true;
    }
  }
  if(validToken){
    getDisposable();
  }
  delay(15000);
}
