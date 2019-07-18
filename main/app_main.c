
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
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


void spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}

void waitWhileBusy(){
    uint32_t count = 0;
    while(1){
        if (gpio_get_level(PIN_NUM_BCKL) == 1) break;
        count++;
        sleep(1);
        if (count > 15){
            printf("Busy time out");
            break;
        }
    }
    printf("spent %d sec waiting busy!\n", count);
}

void writeCommand(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&cmd;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
    printf("\n=========================\n");
    printf("writeCommand cmd: %x,\n", cmd);
}

void writeData(spi_device_handle_t spi, const uint8_t data)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8;                     //Len is in bytes, transaction length is in bits.
    t.tx_buffer=&data;              //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

void wakeUp(spi_device_handle_t spi){
    gpio_set_level(PIN_NUM_RST, 0);
    sleep(1);
    gpio_set_level(PIN_NUM_RST, 1);
    sleep(1);
    writeCommand(spi, CMD_BOOSTER_SOFT_START);
    writeData(spi, 0x17);
    writeData(spi, 0x17);
    writeData(spi, 0x17);
    //0x03, 0x00, 0x2b, 0x2b, 0x09
    // writeCommand(spi, CMD_POWER_SETTING);
    // writeData(spi, 0x03);
    // writeData(spi, 0x00);
    // writeData(spi, 0x2b);
    // writeData(spi, 0x2b);
    // writeData(spi, 0x09);
    writeCommand(spi, CMD_POWER_ON);
    waitWhileBusy();
    writeCommand(spi, CMD_PANEL_SETTING);
    writeData(spi, 0x8f);
    // writeCommand(spi, CMD_PARTIAL_OUT);
    writeCommand(spi, CMD_CDI);
    writeData(spi, 0x77);
    // writeCommand(spi, CMD_PLL_CONTROL);
    // writeData(spi, 0x3a);
    // writeCommand(spi, CMD_VCM_DC_SETTING);
    // writeData(spi, 0x77);
    writeCommand(spi, CMD_RESOLUTION_SETTING);
    writeData(spi, 0x80);
    writeData(spi, 0x01);
    writeData(spi, 0x28);
}

void deepSleep(spi_device_handle_t spi){
    writeCommand(spi, CMD_POWER_OFF);
    writeCommand(spi, CMD_DEEP_SLEEP);
    writeData(spi, 0xa5);
}

uint16_t setPartialRamArea(spi_device_handle_t spi, uint16_t x, uint16_t y, uint16_t xe, uint16_t ye){
    x &= 0xFFF8; // byte boundary
    xe = (xe - 1) | 0x0007; // byte boundary - 1
    writeCommand(spi, CMD_PARTIAL_WINDOW); // partial window
    uint8_t data[1];
    data[0] =  x % 256;
    writeData(spi, data);
    data[0] =  xe % 256;
    writeData(spi, data);
    data[0] =  y / 256;
    writeData(spi, data);
    data[0] =  y % 256;
    writeData(spi, data);
    data[0] =  ye / 256;
    writeData(spi, data);
    data[0] =  ye % 256;
    writeData(spi, data);
    data[0] =  0x01;
    writeData(spi, data);
    // don't see any difference
    //IO.writeDataTransaction(0x00); // don't see any difference
    return (7 + xe - x) / 8; // number of bytes to transfer per line
}

// color{b/w, r}: red={0,0}or{1,0}, white={1,1}, black={0,1} 
// color: 0=black, 1=white, 2=red
void eraseDisplay(spi_device_handle_t spi, uint8_t color){
    uint8_t colorWB = 0xFF;
    uint8_t colorR = 0xFF;
    switch(color){
        case 0:
            colorWB =0x00;
            colorR = 0xFF;
            printf("erase black");
            break;
        case 1:
            colorWB =0xFF;
            colorR = 0xFF;
            printf("erase white");
            break;
        case 2:
            colorWB =0x00;
            colorR = 0x00;
            printf("erase red");
            break;
        default:
            printf("erase white");
            break;
    }
    writeCommand(spi, CMD_DISPLAY_START_TRANSMISSION_W_B);
    for (uint32_t i = 0; i < GxGDEW029Z10_BUFFER_SIZE; i++){
        writeData(spi, colorWB);
    }
    writeCommand(spi, CMD_DISPLAY_START_TRANSMISSION_R);
    for (uint32_t i = 0; i < GxGDEW029Z10_BUFFER_SIZE; i++){
        writeData(spi, colorR);
    }
    writeCommand(spi, CMD_DISPLAY_REFRESH);
    waitWhileBusy();
}

void app_main()
{
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_INPUT);
    esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg={
        .miso_io_num=-1,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=4000000,           //Clock out at 26 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=50,                      //We want to be able to queue 7 transactions at a time
        .pre_cb=spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(VSPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    wakeUp(spi);
    eraseDisplay(spi, 0);
    sleep(5);
    eraseDisplay(spi, 1);
    sleep(5);
    eraseDisplay(spi, 2);
    deepSleep(spi);
    printf("finished\n");
}