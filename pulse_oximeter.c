#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

//==== pin mapping ====//
#define e 17
#define rs 16
#define FIRST_GP 18
//==== command mapping ====//
#define CLRSCR 0x01

#define INITALIZE 0x03
#define MODE_4BIT 0x02



#define lcd_delay 4200 // us
#define OUT 1
#define IN 0
const uint32_t bit_mask = 0xF << FIRST_GP; // four data pins


void lcd_pulse() {
    gpio_put(e, 1);
    sleep_us(lcd_delay);
    gpio_put(e, 0);
    sleep_us(lcd_delay);
}



void lcd_data(uint8_t data, bool r) {
    gpio_put(e, 0); // ensure not sending data
    gpio_put(rs, r); // 0 for cmd 1 for message

    // send higher nibble
    uint32_t value = (0xF0 & data) << (FIRST_GP - 4);
    gpio_put_masked(bit_mask, value);

    lcd_pulse();


    // send lower nibble
    value = (0x0F & data) << FIRST_GP;
    gpio_put_masked(bit_mask, value);

    lcd_pulse();

}


void lcd_write(char* str, int len) {

    for(int i=0; i<len; i++) 
    {
        lcd_data(str[i], 1); // 1 for message
    }
}

void lcd_init() {
    gpio_put(rs, 0); // low for command 
    gpio_put(e, 0);

    uint32_t value = INITALIZE << FIRST_GP;
    
    gpio_put_masked(bit_mask, value);
    lcd_pulse();
    sleep_us(lcd_delay);

    gpio_put_masked(bit_mask, value);
    lcd_pulse();
    sleep_us(lcd_delay);

    gpio_put_masked(bit_mask, value);
    lcd_pulse();
    sleep_us(lcd_delay);


    value = MODE_4BIT << FIRST_GP;
    gpio_put_masked(bit_mask, value);
    lcd_pulse();
    sleep_us(lcd_delay);
    
    lcd_data(0x28, 0); // set # lines, font size
    sleep_us(lcd_delay);

    lcd_data(0x0C, 0); 
    sleep_us(lcd_delay);

    lcd_data(0x01, 0); // clear screen
    sleep_us(lcd_delay);

    lcd_data(0x06, 0); 
    sleep_us(lcd_delay);


}

int main() {

    sleep_ms(50);

    gpio_init(e);
    gpio_init(rs);
    gpio_set_dir(e, OUT);
    gpio_set_dir(rs, OUT);

    gpio_init_mask(bit_mask);
    gpio_set_dir_out_masked(bit_mask);

    lcd_init();
    lcd_write("Line 1", 10);

    lcd_data(0xC0, 0); // move to line 2
    sleep_us(lcd_delay);

    lcd_write("Line 2", 15);

    while(1)
    {
      sleep_us(lcd_delay);  
    }

    return 0;
}