
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "ttgo_2_4.h"


void drawImage(spi_device_handle_t spi){
    uint8_t *data_wb = malloc(GxGDEW029Z10_BUFFER_SIZE * sizeof(uint8_t));
    uint8_t *data_r = malloc(GxGDEW029Z10_BUFFER_SIZE * sizeof(uint8_t));
    setAllScreen(data_wb, data_r, 1);
    setRectangle(data_wb, data_r, 0, 10, 100, 50, 150);
    setRectangle(data_wb, data_r, 2, 50, 128, 100, 200);
    setRectangle(data_wb, data_r, 0, 30, 110, 180, 280);
    updateEPaper(spi, data_wb, data_r);
    free(data_wb);
    free(data_r);
}

void app_main()
{
    spi_device_handle_t spi;
    initTTGO24SPIHandler(&spi);
    wakeUp(spi);
    drawImage(spi);
    sleep(5);
    updateWindow(spi, 70, 40, 15, 250);
    deepSleep(spi);
    printf("finished\n");
}