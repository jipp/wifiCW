#include <Arduino.h>

// TODO: add esp8266
// https://github.com/oe1wkl/Morserino-32/blob/master/Software/src/morse_3_v2.3/morse_3_v2.3.ino

#include <iostream>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <Bounce2.h>
#include <Decoder.hpp>
#include <SPI.h>

#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
#elif ESP8266
#include <espnow.h>
#include <ESP8266WiFi.h>
#else
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

uint64_t startTimerPressed = 0;
uint64_t durationTimerPressed = 0;
uint64_t startTimerNotPressed = 0;
uint64_t durationTimerNotPressed = 0;
const uint16_t dit = 60;
const uint16_t dah = 3 * dit;
const uint16_t symbolBreak = dit;
const uint16_t letterBreak = 3 * dah;
const uint16_t wordBreak = 7 * dit;
uint16_t ditAvg = dit;
uint16_t dahAvg = dah;
uint64_t threshold;
uint8_t d_wpm = 15;
float lacktime = 2.2;
bool newCharacter = false;
bool newWord = false;
std::string code;

Decoder decoder = Decoder();

void displayInfo(uint8_t channel)
{
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(5, 10);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.print("WiFi: ");
  tft.println(wifiChannel);
  tft.setCursor(5, 20);
  tft.print("(F0 = ");
  tft.print(wifiFrequency[wifiChannel - 1]);
  tft.println("MHz)");
  tft.setCursor(5, 50);
  tft.print("selected channel: ");
  tft.println(channel);
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
  //std::cout << "channel: " << (uint16_t)payload.channel << " pressed: " << payload.pressed << std::endl;
  esp_now_send(slave.peer_addr, (uint8_t *)&payload, sizeof(payload));
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  Payload *payload = (Payload *)data;

  std::cout << "Received " << data_len << " Bytes; Channel: " << payload->channel << " / " << payload->pressed << std::endl;

  if (channel == payload->channel)
  {
    statusUpdate(payload->pressed);
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

void recalculateDit(uint64_t duration, uint64_t ditAvg)
{
  ditAvg = (uint64_t)((4 * ditAvg + duration) / 5);
}

void recalculateDah(uint64_t duration, uint64_t ditAvg, uint64_t dahAvg)
{
  if (duration > 2 * dahAvg)
  {
    dahAvg = (dahAvg + 2 * duration) / 3;
    ditAvg = ditAvg / 2 + dahAvg / 6;
  }
  else
  {
    dahAvg = (3 * ditAvg + dahAvg + duration) / 3;
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
  tft.setTextWrap(false); // Allow text to run off right edge

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

  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(buzzerPin, ledChannel);
}

void loop()
{
  contact.update();
  channelUp.update();
  channelDown.update();

  if (contact.fell())
  {
    statusUpdate(true);
    dataSend(channel, true);
    startTimerPressed = millis();
    durationTimerNotPressed = millis() - startTimerNotPressed;
    if (durationTimerNotPressed < ditAvg * 2.4)
      recalculateDit(durationTimerNotPressed, ditAvg);
    //std::cout << "duration released: " << durationTimerNotPressed << ", ditAvg: " << ditAvg << ", dahAvg: " << dahAvg << std::endl;
  }

  if (contact.rose())
  {
    statusUpdate(false);
    dataSend(channel, false);
    durationTimerPressed = millis() - startTimerPressed;
    startTimerNotPressed = millis();
    threshold = (uint64_t)(ditAvg * sqrt(dahAvg / ditAvg));
    if (durationTimerPressed > (ditAvg * 0.5) && durationTimerPressed < (dahAvg * 2.5))
    {
      if (durationTimerPressed < threshold)
      {
        recalculateDit(durationTimerPressed, ditAvg);
        code += ".";
      }
      else
      {
        recalculateDah(durationTimerPressed, ditAvg, dahAvg);
        code += "_";
      }
      newCharacter = true;
    }
    //std::cout << "duration pressed: " << durationTimerPressed << ", threshold: " << threshold << ", ditAvg: " << ditAvg << ", dahAvg: " << dahAvg << std::endl;
  }

  if (contact.read() == LOW) // presssed
  {
  }

  if (newCharacter and contact.read() == HIGH) // released
  {
    durationTimerNotPressed = millis() - startTimerNotPressed;
    lacktime = 2.2;

    if (durationTimerNotPressed > lacktime * ditAvg)
    {
      newCharacter = false;
      newWord = true;
      std::cout << code << " " << decoder.decode(code) << std::endl;
      code.clear();
      d_wpm = (d_wpm + (int)(7200 / (dahAvg + 3 * ditAvg))) / 2;
    }
  }

  if (newWord and contact.read() == HIGH) // released
  {
    durationTimerNotPressed = millis() - startTimerNotPressed;
    lacktime = 5;

    if (d_wpm > 35)
      lacktime = 6;
    else if (d_wpm > 30)
      lacktime = 5.5;

    if (durationTimerNotPressed > lacktime * ditAvg)
    {
      newWord = false;
      std::cout << std::endl;
    }
  }

  if (channelUp.fell())
  {
    channel++;
    if (channel == maxChannel + 1)
      channel = 0;
    displayInfo(channel);
    ledcWriteTone(ledChannel, 0);
  }

  if (channelDown.fell())
  {
    channel--;
    if (channel == UINT8_MAX)
      channel = maxChannel;
    displayInfo(channel);
    ledcWriteTone(ledChannel, 0);
  }
}