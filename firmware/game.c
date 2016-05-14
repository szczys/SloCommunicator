#include <avr/io.h>
#include "game.h"
#include "oledControl.h"

void gameInitBuffer(void) {
    for (uint8_t page=0; page<4; page++) {
        for (uint8_t col=9; col<64; col++) {
            buffer[page][col] = 0x00;
        }
    }
}

void gameSetMetaPixel(uint8_t x, uint8_t y, uint8_t status) {
    //status is ON (non-zero) or OFF (zero)
    //oled is 128x64, our meta board will be 0-64 (x) and 0-32 (y)

    //oled is written in 'pages' of 8-bits. This particular screen
    //cannot be read so the game buffer must be used to ensure writing
    //a new game pixel doesn't overwrite existing pixels.

    //make our changes to the buffer
    uint8_t metaPage = y/8; 
    uint8_t metaPageY = 1<<(y%8);
    if (status) { buffer[metaPage][x] |= metaPageY; }
    else { buffer[metaPage][x] &= ~metaPageY; }

    //write to the screen accounting for 2x pixel sizes meta-v-actual
    uint8_t subMetaPage = (y%8)/4;
    uint8_t actualPage = (metaPage*2) + subMetaPage;
    uint8_t actualX = x*2;
    
    uint8_t metaData = buffer[metaPage][x];
    if (subMetaPage) { metaData>>=4; }  //Shift data to get to upper nibble if necessary
    uint8_t actualData = 0x00;
    for (uint8_t i=0; i<4; i++) {
        if (metaData & (1<<i)) {
            //Each '1' in the meta data is amplified to two bits in the actual
            actualData |= (0b11<<(i*2));
        }
    }
    
    oledSetCursor(actualX,actualPage);
    oledWriteData(actualData);
    oledWriteData(actualData);
}
