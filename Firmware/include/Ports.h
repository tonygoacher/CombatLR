#ifndef __PORTS_H
#define __PORTS_H

// 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 47, 48

 enum  Ports
{
    
    RIGHT =             0,
    IR_TXPORT =         1,
    IR_RXPORT =         2,
    RELOAD =            4,
    HIT_LED =           5,
    MUZZLE_LED =        6,
    LRC =               8,
    GPS_RX =            9,

        //DISPLAY
    CS_DISPAY =         10,
    DIN_DISPLAY =       11,
    SCK_DISPLAY =       12,
    DC_DISPLAY =        13,

    DATA =              17,
    BCLK =              18,
    LEFT =              19,

    TX_UART2    =   38,
    RX_UART2    =   39,
    IR_POWER0   =   40,
    IR_POWER1   =   41,
    IR_POWER2   =   42,


    GPS_TX =        46,
    ZOOM =          47,
    TRIGGER =       48,

};








#endif