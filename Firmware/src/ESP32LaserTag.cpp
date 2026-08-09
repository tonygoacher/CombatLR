#include "ESP32LaserTag.h"


/*
 * Timer instance.
 */
ESP32LaserTag*
    ESP32LaserTag::_timerInstance = nullptr;


/*
 * Constructor.
 */
ESP32LaserTag::ESP32LaserTag(
    uint8_t transmitPin,
    uint8_t receivePin)
    :
    _transmitPin(transmitPin),
    _receivePin(receivePin),
    _rxTimer(nullptr),
    _rxState(RxState::Idle),
    _rxBitCount(0),
    _rxData(0),
    _lastEdgeTime(0),
    _receivedData{},
    _dataAvailable(false)
{
}


/*
 * =============================================================
 * INITIALISATION
 * =============================================================
 */
void ESP32LaserTag::Init()
{
    /*
     * ---------------------------------------------------------
     * IR transmitter
     * ---------------------------------------------------------
     *
     * Using the LEDC API provided by the installed framework:
     *
     *     ledcSetup()
     *     ledcAttachPin()
     *     ledcWrite(channel, duty)
     */
    ledcSetup(
        TX_LEDC_CHANNEL,
        CARRIER_FREQUENCY,
        PWM_RESOLUTION);

    ledcAttachPin(
        _transmitPin,
        TX_LEDC_CHANNEL);

    CarrierOff();


    /*
     * ---------------------------------------------------------
     * IR receiver
     * ---------------------------------------------------------
     *
     * TSOP4856 is open collector and active LOW.
     */
    pinMode(
        _receivePin,
        INPUT_PULLUP);


    /*
     * ---------------------------------------------------------
     * Receive timeout timer
     * ---------------------------------------------------------
     *
     * Timer clock:
     *
     *     80 MHz / 80 = 1 MHz
     *
     * Therefore:
     *
     *     1 timer tick = 1 microsecond
     */
    _rxTimer = timerBegin(
        0,
        80,
        true);


    if (_rxTimer == nullptr)
    {
        return;
    }


    /*
     * Associate this object with the timer ISR.
     */
    _timerInstance = this;


    timerAttachInterrupt(
        _rxTimer,
        RxTimeoutISR,
        true);


    /*
     * Configure the alarm but leave it disabled.
     */
    timerAlarmWrite(
        _rxTimer,
        RX_TIMEOUT_US,
        false);

    timerAlarmDisable(
        _rxTimer);


    /*
     * ---------------------------------------------------------
     * IR receiver interrupt
     * ---------------------------------------------------------
     *
     * We need both edges:
     *
     * FALLING = start of carrier mark
     * RISING  = end of carrier mark
     */
    attachInterruptArg(
        _receivePin,
        RxEdgeISR,
        this,
        CHANGE);
}


/*
 * =============================================================
 * PUBLIC RECEIVE INTERFACE
 * =============================================================
 */

bool ESP32LaserTag::LaserTagDataAvailable()
{
    return _dataAvailable;
}


bool ESP32LaserTag::GetLasertagData(
    LaserTagData& data)
{
    if (!_dataAvailable)
    {
        return false;
    }


    /*
     * Prevent the receive ISR from modifying the packet while
     * it is being copied.
     */
    noInterrupts();

    data = _receivedData;

    _dataAvailable = false;

    interrupts();

    return true;
}


/*
 * =============================================================
 * GPIO INTERRUPT
 * =============================================================
 */

void IRAM_ATTR ESP32LaserTag::RxEdgeISR(
    void* arg)
{
    ESP32LaserTag* instance =
        static_cast<ESP32LaserTag*>(arg);

    instance->HandleRxEdge();
}


/*
 * =============================================================
 * HARDWARE TIMER INTERRUPT
 * =============================================================
 */

void IRAM_ATTR ESP32LaserTag::RxTimeoutISR()
{
    if (_timerInstance != nullptr)
    {
        _timerInstance->HandleRxTimeout();
    }
}


/*
 * =============================================================
 * TIMEOUT HANDLER
 * =============================================================
 */

void IRAM_ATTR ESP32LaserTag::HandleRxTimeout()
{
    /*
     * We were waiting for another edge and it didn't arrive.
     *
     * The packet is therefore incomplete and is discarded.
     */
    ResetReceiver();
}


/*
 * =============================================================
 * RESET RECEIVER
 * =============================================================
 */

void IRAM_ATTR ESP32LaserTag::ResetReceiver()
{
    _rxState = RxState::Idle;

    _rxBitCount = 0;

    _rxData = 0;

    _lastEdgeTime = 0;


    /*
     * Stop the one-shot timeout alarm.
     */
    if (_rxTimer != nullptr)
    {
        timerAlarmDisable(
            _rxTimer);
    }
}


/*
 * =============================================================
 * ARM RECEIVE TIMEOUT
 * =============================================================
 */

void IRAM_ATTR ESP32LaserTag::ArmRxTimeout()
{
    if (_rxTimer == nullptr)
    {
        return;
    }


    /*
     * Reset timer counter.
     */
    timerWrite(
        _rxTimer,
        0);


    /*
     * Configure a one-shot alarm.
     */
    timerAlarmWrite(
        _rxTimer,
        RX_TIMEOUT_US,
        false);


    /*
     * Start the alarm.
     */
    timerAlarmEnable(
        _rxTimer);
}


/*
 * =============================================================
 * RECEIVE STATE MACHINE
 * =============================================================
 *
 * TSOP4856:
 *
 *     HIGH = no carrier
 *     LOW  = 56 kHz carrier detected
 *
 * Therefore:
 *
 *     falling edge = beginning of mark
 *     rising edge  = end of mark
 *
 * Both mark and gap durations are checked.
 *
 * This is important because the MilesTag shot packet has no
 * checksum/error-detection field.
 */
void IRAM_ATTR ESP32LaserTag::HandleRxEdge()
{
    const uint32_t now = micros();

    const bool level =
        digitalRead(_receivePin);


    /*
     * ---------------------------------------------------------
     * IDLE
     * ---------------------------------------------------------
     *
     * A falling edge begins a possible header.
     */
    if (_rxState == RxState::Idle)
    {
        if (level == LOW)
        {
            _lastEdgeTime = now;

            _rxState = RxState::HeaderMark;

            ArmRxTimeout();
        }

        return;
    }


    /*
     * Time since the previous edge.
     */
    const uint32_t duration =
        now - _lastEdgeTime;

    _lastEdgeTime = now;


    switch (_rxState)
    {
        /*
         * -----------------------------------------------------
         * HEADER MARK
         * -----------------------------------------------------
         *
         * Expect a rising edge after approximately 2400 us.
         */
        case RxState::HeaderMark:
        {
            if (level != HIGH)
            {
                ResetReceiver();
                return;
            }


            if (duration < HEADER_MIN_US ||
                duration > HEADER_MAX_US)
            {
                ResetReceiver();
                return;
            }


            _rxState = RxState::HeaderGap;

            ArmRxTimeout();

            break;
        }


        /*
         * -----------------------------------------------------
         * HEADER GAP
         * -----------------------------------------------------
         *
         * Expect a falling edge after approximately 600 us.
         */
        case RxState::HeaderGap:
        {
            if (level != LOW)
            {
                ResetReceiver();
                return;
            }


            if (duration < GAP_MIN_US ||
                duration > GAP_MAX_US)
            {
                ResetReceiver();
                return;
            }


            _rxBitCount = 0;

            _rxData = 0;

            _rxState = RxState::DataMark;

            ArmRxTimeout();

            break;
        }


        /*
         * -----------------------------------------------------
         * DATA MARK
         * -----------------------------------------------------
         *
         * Rising edge terminates the current mark.
         */
        case RxState::DataMark:
        {
            if (level != HIGH)
            {
                ResetReceiver();
                return;
            }


            bool bit;


            /*
             * 600 us = zero.
             */
            if (duration >= ZERO_MIN_US &&
                duration <= ZERO_MAX_US)
            {
                bit = false;
            }


            /*
             * 1200 us = one.
             */
            else if (duration >= ONE_MIN_US &&
                     duration <= ONE_MAX_US)
            {
                bit = true;
            }


            /*
             * Invalid mark.
             */
            else
            {
                ResetReceiver();
                return;
            }


            /*
             * MilesTag transmits MSB first.
             */
            _rxData <<= 1;

            if (bit)
            {
                _rxData |= 1;
            }


            ++_rxBitCount;


            /*
             * Complete packet.
             *
             * We don't need to wait for the final gap.
             */
            if (_rxBitCount == DATA_BITS)
            {
                /*
                 * Shot packet type bit must be zero.
                 */
                if ((_rxData & 0x2000) == 0)
                {
                    _receivedData =
                        DecodePacket(_rxData);

                    _dataAvailable = true;
                }


                ResetReceiver();

                return;
            }


            /*
             * More bits remain.
             *
             * Wait for the falling edge starting the next mark.
             */
            _rxState = RxState::DataGap;

            ArmRxTimeout();

            break;
        }


        /*
         * -----------------------------------------------------
         * DATA GAP
         * -----------------------------------------------------
         *
         * Falling edge starts the next carrier mark.
         */
        case RxState::DataGap:
        {
            if (level != LOW)
            {
                ResetReceiver();
                return;
            }


            if (duration < GAP_MIN_US ||
                duration > GAP_MAX_US)
            {
                ResetReceiver();
                return;
            }


            _rxState = RxState::DataMark;

            ArmRxTimeout();

            break;
        }


        default:
        {
            ResetReceiver();
            break;
        }
    }
}


/*
 * =============================================================
 * DECODE PACKET
 * =============================================================
 *
 * Wire format:
 *
 *     0 ppppppp tt dddd
 *
 *     bit 13       packet type
 *     bits 12..6   player ID
 *     bits 5..4    team
 *     bits 3..0    damage
 */
LaserTagData ESP32LaserTag::DecodePacket(
    uint16_t packet)
{
    LaserTagData data{};


    data.playerId =
        (packet >> 6) & 0x7F;


    data.team =
        (packet >> 4) & 0x03;


    data.damage =
        packet & 0x0F;


    return data;
}


/*
 * =============================================================
 * TRANSMITTER
 * =============================================================
 */

void ESP32LaserTag::CarrierOn()
{
    /*
     * 50% duty cycle.
     */
    ledcWrite(
        TX_LEDC_CHANNEL,
        128);
}


void ESP32LaserTag::CarrierOff()
{
    ledcWrite(
        TX_LEDC_CHANNEL,
        0);
}


/*
 * =============================================================
 * TRANSMIT BIT
 * =============================================================
 *
 *     0 = 600 us carrier + 600 us gap
 *     1 = 1200 us carrier + 600 us gap
 */
void ESP32LaserTag::TransmitBit(
    bool bit)
{
    CarrierOn();


    if (bit)
    {
        delayMicroseconds(
            ONE_MARK_US);
    }
    else
    {
        delayMicroseconds(
            ZERO_MARK_US);
    }


    CarrierOff();


    delayMicroseconds(
        GAP_US);
}


/*
 * =============================================================
 * SEND MILESTAG 2 SHOT
 * =============================================================
 */

void ESP32LaserTag::SendLasertagData(
    LaserTagData& dataToSend)
{
    /*
     * Construct:
     *
     *     0 ppppppp tt dddd
     */
    uint16_t packet = 0;


    packet |=
        static_cast<uint16_t>(
            dataToSend.playerId & 0x7F)
        << 6;


    packet |=
        static_cast<uint16_t>(
            dataToSend.team & 0x03)
        << 4;


    packet |=
        static_cast<uint16_t>(
            dataToSend.damage & 0x0F);


    /*
     * ---------------------------------------------------------
     * Header
     * ---------------------------------------------------------
     */
    CarrierOn();

    delayMicroseconds(
        HEADER_US);

    CarrierOff();

    delayMicroseconds(
        GAP_US);


    /*
     * ---------------------------------------------------------
     * 14 data bits, MSB first
     * ---------------------------------------------------------
     */
    for (int8_t bit = DATA_BITS - 1;
         bit >= 0;
         --bit)
    {
        TransmitBit(
            (packet >> bit) & 1);
    }
}