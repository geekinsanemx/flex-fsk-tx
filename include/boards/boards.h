#ifndef BOARDS_H
#define BOARDS_H

#include <stdint.h>

#if defined(TTGO_LORA32_V21)

constexpr uint8_t LORA_CS_PIN = 18;
constexpr uint8_t LORA_IRQ_PIN = 26;
constexpr uint8_t LORA_RST_PIN = 23;
constexpr uint8_t LORA_GPIO_PIN = 33;
constexpr uint8_t LORA_SCK_PIN = 5;
constexpr uint8_t LORA_MOSI_PIN = 27;
constexpr uint8_t LORA_MISO_PIN = 19;

constexpr uint8_t OLED_SDA_PIN = 21;
constexpr uint8_t OLED_SCL_PIN = 22;
constexpr int8_t OLED_RST_PIN = -1;
constexpr int8_t VEXT_PIN = -1;

constexpr uint8_t BATTERY_ADC_PIN = 35;
constexpr uint8_t LED_PIN = 25;

constexpr uint8_t GSM_TX_PIN = 15;
constexpr uint8_t GSM_RX_PIN = 14;
constexpr uint8_t GSM_PWR_PIN = 13;

#elif defined(HELTEC_WIFI_LORA32_V2)

constexpr uint8_t LORA_CS_PIN = 18;
constexpr uint8_t LORA_IRQ_PIN = 26;
constexpr uint8_t LORA_RST_PIN = 14;
constexpr uint8_t LORA_GPIO_PIN = 35;
constexpr uint8_t LORA_SCK_PIN = 5;
constexpr uint8_t LORA_MOSI_PIN = 27;
constexpr uint8_t LORA_MISO_PIN = 19;

constexpr uint8_t OLED_SDA_PIN = 4;
constexpr uint8_t OLED_SCL_PIN = 15;
constexpr int8_t OLED_RST_PIN = 16;
constexpr int8_t VEXT_PIN = 21;

constexpr uint8_t BATTERY_ADC_PIN = 37;
constexpr uint8_t LED_PIN = 25;

constexpr uint8_t GSM_TX_PIN = 17;
constexpr uint8_t GSM_RX_PIN = 13;
constexpr uint8_t GSM_PWR_PIN = 23;

#else
#error "No board defined. Please define either TTGO_LORA32_V21 or HELTEC_WIFI_LORA32_V2"
#endif

#endif
