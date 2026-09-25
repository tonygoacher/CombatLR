#ifndef __PORTS_H
#define __PORTS_H

// 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 47, 48

 enum  Ports
{
    
    RIGHT =             14,
    IR_TXPORT =         1,
    IR_RXPORT =         2,
    RELOAD =            21,
    HIT_LED =           38,
    MUZZLE_LED =        39,
    LRC =               4,
    GPS_RX =            7,

        //DISPLAY
    CS_DISPAY =         16,
    DIN_DISPLAY =       17,
    SCK_DISPLAY =       18,
    DC_DISPLAY =        8,

    I2S_DATA =          6,
    BCLK =              5,
    LEFT =              13,

    TX_UART2    =   9,
    RX_UART2    =   10,
    IR_POWERLOW   =   40,
    IR_POWERMED   =   41,
    IR_POWERHIGH   =   42,


    GPS_TX =        15,
    ZOOM =          12,
    TRIGGER =       47,

};








#endif