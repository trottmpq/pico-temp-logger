#include <stdio.h>
#include <stdint.h>
#include <string>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"
// #include <logo.hpp>
#include <GFX.hpp>
#include <MAX31865.hpp>

#define READ_BIT 0x80

static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

static void write_register(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg;  // remove read bit as this is a write
    buf[1] = data;
    cs_select();
    spi_write_blocking(spi0, buf, 2);
    cs_deselect();
    sleep_ms(10);
}

static void read_registers(uint8_t reg, uint8_t *buf, uint16_t len) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.
    cs_select();
    spi_write_blocking(spi0, &reg, 1);
    sleep_ms(10);
    spi_read_blocking(spi0, 0, buf, len);
    cs_deselect();
    sleep_ms(10);
}




int main() {
    uint8_t temperature = 0;

    //setup
    stdio_init_all();

    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // This example will use SPI0 at 0.5MHz.
    spi_init(spi0, 500 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI));

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(PICO_DEFAULT_SPI_CSN_PIN, "SPI CS"));

    GFX oled(0x3C, size::W128xH32, i2c0);   //Declare oled instance
    // MAX31865 temp(spi0);

    // temp.begin(MAX31865_3WIRE);
    // temp.setWires(MAX31865_3WIRE);
    // Set 3 Wire
    write_register(0x80, 0xD1);
   
    uint8_t buffer;

    read_registers(0x00, &buffer, 1);


    while(true) 
    {
        sleep_ms(1000);
        temperature = buffer;
        // temperature = temp.temperature(100, 430);
        // temperature = rand() % 100 + 1;
        oled.clear();                       //Clear buffer
        oled.drawString(0, 0, "Pico Temp Logger");
        oled.drawHorizontalLine(0,9,oled.getWidth());
        oled.drawString(0, 11, "Temperature :");
        oled.drawString(oled.getWidth() - 20, 11, std::to_string(temperature));
        oled.drawProgressBar(0, oled.getHeight()-5, oled.getWidth(), 5, temperature);

        oled.display();                     //Send buffer to the screen
    }
    return 0;
}