#ifndef __PORTS_H
#define __PORTS_H

// 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 47, 48

 enum  Ports
{
    
    TXPORT =    1,
    RXPORT =    2,

    LRC =       8,
    BCLK =      18,
    DATA =      17,

    GPS_RX =    9,
    GPS_TX =    46,

    TRIGGER =   48,
    RELOAD =    4,
    RIGHT =     0,
    ZOOM =      47,
    LEFT =      19,
};








#endif