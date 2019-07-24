#include "ttgo_2_4.h"

void spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}

void initTTGO24SPIHandler(spi_device_handle_t *spi){
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_INPUT);
    esp_err_t ret;
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
    ret=spi_bus_add_device(VSPI_HOST, &devcfg, spi);
    ESP_ERROR_CHECK(ret);
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
    t.length=8;                     //transaction length is in bits.
    t.tx_buffer=&data;              //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

// color{b/w, r}: red={0,0}or{1,0}, white={1,1}, black={0,1} 
// color: 0=black, 1=white, 2=red
void setAllScreen(uint8_t* image_data_w_b, uint8_t* image_data_r, uint8_t color){
    switch(color){
        case 0:
            for (uint32_t i = 0; i < GxGDEW029Z10_BUFFER_SIZE; i++){
                *(image_data_w_b+i) = 0x00;
                *(image_data_r+i) = 0xFF;
            }
            break;
        case 1:
            for (uint32_t i = 0; i < GxGDEW029Z10_BUFFER_SIZE; i++){
                *(image_data_w_b+i) = 0xFF;
                *(image_data_r+i) = 0xFF;
            }
            printf("set color %d \n", color);
            break;
        case 2:
            for (uint32_t i = 0; i < GxGDEW029Z10_BUFFER_SIZE; i++){
                *(image_data_w_b+i) = 0xFF;
                *(image_data_r+i) = 0xFF;
            }
            break;
        default:
            printf("wrong color %d \n", color);
            break;
    }
}

//x: 128, y: 296; x,y: 0,0 -> 127,0 -> 0,1 -> 127,1 ...
void setRectangle(uint8_t* image_data_w_b, uint8_t* image_data_r, uint8_t color, uint16_t sx, uint16_t ex, uint16_t sy, uint16_t ey){
    uint8_t data = 0x00;
    uint8_t bit_shift = 0;
    uint8_t bit_data = 0;
    // writeCommand(spi, CMD_DISPLAY_START_TRANSMISSION_W_B);
    // 每8bit為單位，從bit7 ~ 0對應pixel0 ~7
    for (uint32_t i = 0; i < GxGDEW029Z10_BUFFER_SIZE*8; i++){
        bit_shift = 7 - (i % 8);
        bit_data = 0x01 << bit_shift;
        if (i%GxGDEW029Z10_WIDTH >= sx && i%GxGDEW029Z10_WIDTH < ex && i/GxGDEW029Z10_WIDTH >= sy && i/GxGDEW029Z10_WIDTH < ey){
            switch(color){
                case 0:
                    //black
                    *(image_data_w_b+(i/8)) = (*(image_data_w_b+(i/8))) & (~bit_data);
                    *(image_data_r+(i/8)) = (*(image_data_r+(i/8))) | bit_data;
                    break;
                case 1:
                    //white
                    *(image_data_w_b+(i/8)) = (*(image_data_w_b+(i/8))) | bit_data;
                    *(image_data_r+(i/8)) = (*(image_data_r+(i/8))) | bit_data;
                    break;
                case 2:
                    //red
                    *(image_data_w_b+(i/8)) = (*(image_data_w_b+(i/8))) & (~bit_data);
                    *(image_data_r+(i/8)) = (*(image_data_r+(i/8))) & (~bit_data);
                    break;
                default:
                    printf("wrong color: %d \n", color);
                    break;
            }
        }
    }
}

uint16_t _setPartialRamArea(spi_device_handle_t spi, uint16_t x, uint16_t y, uint16_t xe, uint16_t ye){
    x &= 0xFFF8; // byte boundary
    xe = (xe - 1) | 0x0007; // byte boundary - 1
    writeCommand(spi, CMD_PARTIAL_WINDOW); // partial window
    uint8_t data[1];
    writeData(spi, x % 256);
    writeData(spi, xe % 256);
    writeData(spi, y / 256);
    writeData(spi, y % 256);
    writeData(spi, ye / 256);
    writeData(spi, ye % 256);
    writeData(spi, 0x01);
    return (7 + xe - x) / 8; // number of bytes to transfer per line
}

void updateWindow(spi_device_handle_t spi, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  if (x >= GxGDEW029Z10_WIDTH) return;
  if (y >= GxGDEW029Z10_HEIGHT) return;
  // x &= 0xFFF8; // byte boundary, not here, use encompassing rectangle
  uint16_t xe = x + w;
  uint16_t ye = y + h;
  // x &= 0xFFF8; // byte boundary, not needed here
  uint16_t xs_bx = x / 8;
  uint16_t xe_bx = (xe + 7) / 8;
  wakeUp(spi);
  writeCommand(spi, 0x91); // partial in
  _setPartialRamArea(spi, x, y, xe, ye);
  writeCommand(spi, 0x10);
  for (int16_t y1 = y; y1 <= ye; y1++)
  {
    for (int16_t x1 = xs_bx; x1 < xe_bx; x1++)
    {
      writeData(spi, 0x00);
    }
  }
  writeCommand(spi, 0x13);
  for (int16_t y1 = y; y1 <= ye; y1++)
  {
    for (int16_t x1 = xs_bx; x1 < xe_bx; x1++)
    {
      writeData(spi, 0x00);
    }
  }
  writeCommand(spi, 0x12);      //display refresh
  waitWhileBusy();
  writeCommand(spi, 0x92); // partial out
  sleep(1); // don't stress this display
}

void updateEPaper(spi_device_handle_t spi, uint8_t* image_data_w_b, uint8_t* image_data_r){
    writeCommand(spi, CMD_DISPLAY_START_TRANSMISSION_W_B);
    for (uint32_t i = 0; i < GxGDEW029Z10_BUFFER_SIZE; i++){
        writeData(spi, *(image_data_w_b+i));
    }
    writeCommand(spi, CMD_DISPLAY_START_TRANSMISSION_R);
    for (uint32_t i = 0; i < GxGDEW029Z10_BUFFER_SIZE; i++){
        writeData(spi, *(image_data_r+i));
    }
    writeCommand(spi, CMD_DISPLAY_REFRESH);
    waitWhileBusy();
}

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

void wakeUp(spi_device_handle_t spi){
    gpio_set_level(PIN_NUM_RST, 0);
    sleep(1);
    gpio_set_level(PIN_NUM_RST, 1);
    sleep(1);
    writeCommand(spi, CMD_BOOSTER_SOFT_START);
    writeData(spi, 0x17);
    writeData(spi, 0x17);
    writeData(spi, 0x17);
    writeCommand(spi, CMD_POWER_ON);
    waitWhileBusy();
    writeCommand(spi, CMD_PANEL_SETTING);
    writeData(spi, 0x8f);
    writeCommand(spi, CMD_CDI);
    writeData(spi, 0x77);
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