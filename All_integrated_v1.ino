#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <max6675.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display largura, em pixels
#define SCREEN_HEIGHT 64 // OLED display altura, em pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int ktcSO = 25;  // Pino SO termopar laranja
int ktcCS = 26;  // Pino CS termopar verde
int ktcCLK = 33; // Pino SCK termopar azul

int relePin = 32; // Pino ativacao do rele

MAX6675 ktc(ktcCLK, ktcCS, ktcSO);

// Replace with your network credentials
const char *ssid = "WIFI-NAME";
const char *password = "WIFI-PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

String message = "";
String objValue001 = "0";   // slider
String objValue002 = "off"; // on/off status
String objValue003 = "0";   // max
String objValue004 = "0";   // atual
String objValue005 = "off"; // rele status
String objValue006 = "";    // chart array

// Json Variable to Hold Obj Values
JSONVar objValues;

// Get Obj Values
String getObjValues()
{
    objValues["objValue001"] = String(objValue001);
    objValues["objValue002"] = String(objValue002);
    objValues["objValue003"] = String(objValue003);
    objValues["objValue004"] = String(objValue004);
    objValues["objValue005"] = String(objValue005);
    objValues["objValue006"] = String(objValue006);

    String jsonString = JSON.stringify(objValues);
    return jsonString;
}

// Set the display mode
String displayMode = "std";

// Initialize SPIFFS
void initFS()
{
    SPIFFS.begin(true);
    if (!SPIFFS.begin())
    {
        Serial.println("An error has occurred while mounting SPIFFS");
    }
    else
    {
        Serial.println("SPIFFS mounted successfully");
    }
}

// Initialize WiFi
void initWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(1000);
    }
    Serial.println(WiFi.localIP());
}

void notifyClients(String objValues)
{
    ws.textAll(objValues);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        data[len] = 0;
        message = (char *)data;
        if (message.indexOf("001s") >= 0)
        {
            objValue001 = message.substring(4);
        }
        if (message.indexOf("002s") >= 0)
        {
            objValue002 = message.substring(4);
        }
        if (message.indexOf("003s") >= 0)
        {
            objValue003 = message.substring(4);
        }
        if (message.indexOf("004s") >= 0)
        {
            objValue004 = message.substring(4);
        }
        // objValue005 only send message to web, not recive
        // if (message.indexOf("005s") >= 0)
        //{
        //    objValue005 = message.substring(4);
        //}
        if (message.indexOf("006s") >= 0)
        {
            objValue006 = message.substring(4);
        }
        Serial.print(getObjValues());
        Serial.print("\n");
    }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void initWebSocket()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

void setup()
{
    Serial.begin(115200);

    initFS();
    initWiFi();
    initWebSocket();

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", "text/html"); });

    server.serveStatic("/", SPIFFS, "/");

    // Start server
    server.begin();

    // Rele config
    pinMode(relePin, OUTPUT);

    // Temperatura config
    Serial.println("Max6675 test");
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (128x64)
    delay(500);
    display.clearDisplay();
    display.setCursor(15, 15);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println("Inicializando...");
    display.setCursor(25, 35);
    display.setTextSize(1);
    display.print(WiFi.localIP());
    display.display();
    delay(5000);
}

void loop()
{
    notifyClients(getObjValues());

    float DC = ktc.readCelsius();    // Le temperatura em Celsius
    float DF = ktc.readFahrenheit(); // Le temperatura em Fahrenheit

    // Display config
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(20, 0);
    display.print("Temperatura");

    display.setTextSize(2);
    display.setCursor(10, 20);
    display.print(ktc.readCelsius());
    display.print((char)247);
    display.print("C");

    display.setTextSize(2);
    display.setCursor(10, 45);
    display.print(objValue003);
    display.print(" Max");
    display.display();

    objValue004 = String(ktc.readCelsius());

    // Rele config
    if (ktc.readCelsius() <= objValue003.toInt())
    {
        digitalWrite(relePin, HIGH);
        objValue005 = "on";
    }
    else
    {
        digitalWrite(relePin, LOW);
        objValue005 = "off";
    }

    ws.cleanupClients();
    delay(250);
}
