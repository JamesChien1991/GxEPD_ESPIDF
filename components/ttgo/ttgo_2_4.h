#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

//DEFINE
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

#define PIN_NUM_DC   17
#define PIN_NUM_RST  16
#define PIN_NUM_BCKL 4

#define GxGDEW029Z10_WIDTH 128
#define GxGDEW029Z10_HEIGHT 296
#define GxGDEW029Z10_BUFFER_SIZE ((uint32_t)(GxGDEW029Z10_WIDTH) * (uint32_t)(GxGDEW029Z10_HEIGHT) / 8)

#define CMD_PANEL_SETTING 0x00
#define CMD_POWER_SETTING 0x01
#define CMD_POWER_OFF 0x02
#define CMD_POWER_ON 0x04
#define CMD_BOOSTER_SOFT_START 0x06
#define CMD_DEEP_SLEEP 0x07
#define CMD_DISPLAY_START_TRANSMISSION_W_B 0x10
#define CMD_DATA_STOP 0x11
#define CMD_DISPLAY_REFRESH 0x12
#define CMD_DISPLAY_START_TRANSMISSION_R 0x13
#define CMD_PLL_CONTROL 0x30
#define CMD_CDI 0x50
#define CMD_RESOLUTION_SETTING 0x61
#define CMD_VCM_DC_SETTING 0x82
#define CMD_PARTIAL_WINDOW 0x90
#define CMD_PARTIAL_IN 0x91
#define CMD_PARTIAL_OUT 0x92

void initTTGO24SPIHandler(spi_device_handle_t *spi);
void drawScreen(spi_device_handle_t spi, uint8_t* image_data_w_b, uint8_t* image_data_r);
void wakeUp(spi_device_handle_t spi);
void spi_pre_transfer_callback(spi_transaction_t *t);
void writeCommand(spi_device_handle_t spi, const uint8_t cmd);
void writeData(spi_device_handle_t spi, const uint8_t data);
void setAllScreen(uint8_t* image_data_w_b, uint8_t* image_data_r, uint8_t color);
void setRectangle(uint8_t* image_data_w_b, uint8_t* image_data_r, uint8_t color, uint16_t sx, uint16_t ex, uint16_t sy, uint16_t ey);
void eraseDisplay(spi_device_handle_t spi, uint8_t color);
uint16_t _setPartialRamArea(spi_device_handle_t spi, uint16_t x, uint16_t y, uint16_t xe, uint16_t ye);
void updateWindow(spi_device_handle_t spi, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void updateEPaper(spi_device_handle_t spi, uint8_t* image_data_w_b, uint8_t* image_data_r);
void deepSleep(spi_device_handle_t spi);
void waitWhileBusy();

