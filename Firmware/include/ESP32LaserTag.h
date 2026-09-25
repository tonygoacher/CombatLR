#pragma once

#include <Arduino.h>
#include "IRLaserTag.h"




/*
 * Generic laser-tag interface.
 */


/*
 * ESP32 implementation of MilesTag 2.
 */
class ESP32LaserTag : public IRLaserTag
{
public:

    ESP32LaserTag(
        uint8_t transmitPin,
        uint8_t receivePin,
        uint8_t irLowPort, uint8_t irMedPort, uint8_t irHighPort);

    void Init() override;

    bool LaserTagDataAvailable() override;

    bool GetLasertagData(
        LaserTagData& data) override;

    void SendLasertagData(
        LaserTagData& dataToSend) override;

    void SetIRPower(IRPowerLevel powerLevel) override;

private:

    /*
     * Hardware.
     */
    uint8_t _transmitPin;
    uint8_t _receivePin;

    uint8_t _irLowPort;
    uint8_t _irMedPort;
    uint8_t _irHighPort;

    static constexpr uint8_t TX_LEDC_CHANNEL = 0;

    hw_timer_t* _rxTimer;


    /*
     * Receiver state machine.
     */
    enum class RxState : uint8_t
    {
        Idle,
        HeaderMark,
        HeaderGap,
        DataMark,
        DataGap
    };

    volatile RxState _rxState;

    volatile uint8_t _rxBitCount;

    volatile uint16_t _rxData;

    volatile uint32_t _lastEdgeTime;


    /*
     * Completed packet.
     */
    LaserTagData _receivedData;

    volatile bool _dataAvailable;


    /*
     * The timer API in the installed Arduino framework
     * takes a void() callback rather than a void* argument.
     *
     * One ESP32LaserTag instance is therefore associated with
     * the timer.
     */
    static ESP32LaserTag* _timerInstance;


    /*
     * Interrupt handlers.
     */
    static void IRAM_ATTR RxEdgeISR(void* arg);

    static void IRAM_ATTR RxTimeoutISR();


    /*
     * Receiver.
     */
    void IRAM_ATTR HandleRxEdge();

    void IRAM_ATTR HandleRxTimeout();

    void IRAM_ATTR ResetReceiver();

    void IRAM_ATTR ArmRxTimeout();


    /*
     * Packet decoding.
     */
    static LaserTagData DecodePacket(
        uint16_t packet);


    /*
     * Transmitter.
     */
    void CarrierOn();

    void CarrierOff();

    void TransmitBit(bool bit);

    void AllOff();


    /*
     * MilesTag timing.
     */
    static constexpr uint32_t CARRIER_FREQUENCY = 56000;

    static constexpr uint8_t PWM_RESOLUTION = 8;

    static constexpr uint32_t HEADER_US = 2400;

    static constexpr uint32_t ZERO_MARK_US = 600;

    static constexpr uint32_t ONE_MARK_US = 1200;

    static constexpr uint32_t GAP_US = 600;


    /*
     * Receiver tolerances.
     */
    static constexpr uint32_t HEADER_MIN_US = 2000;
    static constexpr uint32_t HEADER_MAX_US = 2800;

    static constexpr uint32_t ZERO_MIN_US = 450;
    static constexpr uint32_t ZERO_MAX_US = 800;

    static constexpr uint32_t ONE_MIN_US = 950;
    static constexpr uint32_t ONE_MAX_US = 1400;

    static constexpr uint32_t GAP_MIN_US = 400;
    static constexpr uint32_t GAP_MAX_US = 800;


    /*
     * Maximum time allowed for an expected edge.
     */
    static constexpr uint32_t RX_TIMEOUT_US = 3000;


    /*
     * MilesTag shot packet length.
     */
    static constexpr uint8_t DATA_BITS = 14;
};