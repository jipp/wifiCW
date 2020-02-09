#include <Arduino.h>

// TODO: add esp8266

#include <iostream>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Bounce2.h>
#include <SPI.h>

#include "CWDecoder.hpp"
#include "SignalDecoder.hpp"

#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
#elif ESP8266
#include <espnow.h>
#include <ESP8266WiFi.h>
#else
#endif

#ifndef SPEED
#define SPEED 115200
#endif

// WIFI defenitions
const uint8_t wifiChannel = 3; // allowed values 1-14

// TFT defenitions
#define TFT_CS 14  //for D32 Pro
#define TFT_DC 27  //for D32 Pro
#define TFT_RST 33 //for D32 Pro
#define TS_CS 12   //for D32 Pro

// buttons and buzzer
const uint8_t buttonPin = 13;
const uint8_t channelUpPin = 2;
const uint8_t channelDownPin = 15;
const uint8_t buzzerPin = 25;

// datastructure
typedef struct
{
  uint8_t channel = 0;
  bool pressed = false;
} Payload;

uint16_t wifiFrequency[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484};
esp_now_peer_info_t slave;
uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const esp_now_peer_info_t *peer = &slave;
Payload payload;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

Bounce contact = Bounce();
Bounce channelUp = Bounce();
Bounce channelDown = Bounce();

const uint8_t ledChannel = 0;
const double freq = 500;
const uint8_t resolution = 8;

const uint8_t maxChannel = 10;
uint8_t channel = 0;

SignalDecoder signalDecoderSend = SignalDecoder();
SignalDecoder signalDecoderReceive = SignalDecoder();

const uint8_t displayLetters = 15;
std::string bufRX;
std::string bufTX;

void displayInfo(uint8_t channel)
{
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(5, 10);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.print("WiFi: ");
  tft.println(wifiChannel);
  tft.setCursor(5, 20);
  tft.print("(F0 = ");
  tft.print(wifiFrequency[wifiChannel - 1]);
  tft.println("MHz)");
  tft.setCursor(5, 35);
  tft.print("CW channel: ");
  tft.println(channel);
}

void displayWPM(uint8_t rxWPM, uint8_t txWPM)
{
  tft.fillRect(0, 50, tft.width(), 8, ST7735_BLACK);
  tft.setCursor(5, 50);
  tft.print("WPM (tx/rx): ");
  tft.print(rxWPM);
  tft.print("/");
  tft.print(txWPM);
}

void displayLetter(int16_t y, std::string text, std::string letter)
{
  tft.fillRect(0, y, tft.width(), 8, ST7735_BLACK);
  tft.setCursor(5, y);
  tft.print(text.c_str());
  tft.print(letter.c_str());
}

void statusUpdate(bool pressed)
{
  if (pressed)
  {
    digitalWrite(BUILTIN_LED, LOW);
    ledcWriteTone(ledChannel, freq);
  }
  else
  {
    digitalWrite(BUILTIN_LED, HIGH);
    ledcWriteTone(ledChannel, 0);
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status != ESP_NOW_SEND_SUCCESS)
    std::cout << "Send Fail!" << std::endl;
}

void dataSend(uint8_t channel, bool pressed)
{
  payload.channel = channel;
  payload.pressed = pressed;

  esp_now_send(slave.peer_addr, (uint8_t *)&payload, sizeof(payload));
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  Payload *payload = (Payload *)data;

  if (channel == payload->channel)
  {
    statusUpdate(payload->pressed);
    signalDecoderReceive.contactStatus(payload->pressed);
  }
}

void espNowInit()
{
#ifdef ESP32
  if (esp_now_init() == ESP_OK)
#else
  if (esp_now_init() != 0)
#endif
  {
    std::cout << "ESP NOW init!" << std::endl;
  }
  else
  {
    std::cout << "ESP NOW init failed ..." << std::endl;
    ESP.restart();
  }
}

void addressInit()
{
  memcpy(&slave.peer_addr, &broadcastMac, 6);
  slave.channel = wifiChannel;
  slave.encrypt = 0;

  if (esp_now_add_peer(peer) == ESP_OK)
  {
    std::cout << "Address added!" << std::endl;
  }
}

void getBuffer(int16_t y, std::string text, std::string &buf, SignalDecoder &signalDecoder1, SignalDecoder &signalDecoder2)
{
  CWDecoder cwDecoder = CWDecoder();

  if (signalDecoder1.status == signalDecoder1.Status::characterReceived)
  {
    std::cout << cwDecoder.decode(signalDecoder1.code) << " " << signalDecoder1.code << std::endl;
    buf += cwDecoder.decode(signalDecoder1.code);

    if (buf.length() > displayLetters)
      buf.erase(0, buf.length() - displayLetters);

    signalDecoder1.code.clear();
    displayLetter(y, text, buf);
    signalDecoder1.status = signalDecoder1.Status::waitingWordReceived;
  }

  if (signalDecoder1.status == signalDecoder1.Status::wordReceived)
  {
    std::cout << std::endl;
    displayWPM(signalDecoder1.wpm, signalDecoder2.wpm);
    buf += " ";
    signalDecoder1.status = signalDecoder1.Status::waiting;
  }
}

void setup()
{
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(channelUpPin, INPUT_PULLUP);
  pinMode(channelDownPin, INPUT_PULLUP);
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(BUILTIN_LED, HIGH);

  Serial.begin(SPEED);

  tft.initR(INITR_144GREENTAB);
  tft.setTextWrap(true); // Allow text to run off right edge

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  espNowInit();
  addressInit();
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  contact.attach(buttonPin);
  contact.interval(25);
  channelUp.attach(channelUpPin);
  channelUp.interval(25);
  channelDown.attach(channelDownPin);
  channelDown.interval(25);

  displayInfo(channel);
  displayWPM(signalDecoderSend.wpm, signalDecoderReceive.wpm);
  displayLetter(75, "tx: ", bufTX);
  displayLetter(85, "rx: ", bufRX);

  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(buzzerPin, ledChannel);
}

void loop()
{
  contact.update();
  channelUp.update();
  channelDown.update();

  signalDecoderReceive.contactUpdate();

  getBuffer(75, "tx: ", bufTX, signalDecoderSend, signalDecoderReceive);
  getBuffer(85, "rx: ", bufRX, signalDecoderReceive, signalDecoderSend);

  if (contact.fell())
  {
    statusUpdate(true);
    dataSend(channel, true);
    signalDecoderSend.pressing();
  }

  if (contact.rose())
  {
    statusUpdate(false);
    dataSend(channel, false);
    signalDecoderSend.releasing();
  }

  if (contact.read() == HIGH)
  {
    signalDecoderSend.released();
  }

  if (channelUp.fell())
  {
    channel++;
    if (channel == maxChannel + 1)
      channel = 0;
    displayInfo(channel);
    displayWPM(signalDecoderSend.wpm, signalDecoderReceive.wpm);
    displayLetter(75, "tx: ", bufTX);
    displayLetter(85, "rx: ", bufRX);
    ledcWriteTone(ledChannel, 0);
  }

  if (channelDown.fell())
  {
    channel--;
    if (channel == UINT8_MAX)
      channel = maxChannel;
    displayInfo(channel);
    displayWPM(signalDecoderSend.wpm, signalDecoderReceive.wpm);
    displayLetter(75, "tx: ", bufTX);
    displayLetter(85, "rx: ", bufRX);
    ledcWriteTone(ledChannel, 0);
  }
}