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
    uint8_t receivePin,
    uint8_t irLowPort,
    uint8_t irMedPort,
    uint8_t irHighPort)
    :
    _transmitPin(transmitPin),
    _receivePin(receivePin),
    _rxTimer(nullptr),
    _rxState(RxState::Idle),
    _rxBitCount(0),
    _rxData(0),
    _lastEdgeTime(0),
    _receivedData{},
    _dataAvailable(false),
    _irLowPort(irLowPort),
    _irMedPort(irMedPort),
    _irHighPort(irHighPort)
{
    pinMode(_irLowPort, OUTPUT);
    pinMode(_irMedPort, OUTPUT);
    pinMode(_irHighPort, OUTPUT);
    AllOff();
}


/*
 * =============================================================
 * INITIALISATION
 * =============================================================
 */
void ESP32LaserTag::Init()
{
    // 1. Setup IR Transmitter Pin & LEDC PWM
    pinMode(_transmitPin, OUTPUT);
    digitalWrite(_transmitPin, LOW);
    ledcAttachChannel(_transmitPin, CARRIER_FREQUENCY, PWM_RESOLUTION, TX_LEDC_CHANNEL);
    CarrierOff();

    // 2. Setup IR Receiver Pin
    pinMode(_receivePin, INPUT_PULLUP);

    // 3. Setup Hardware Timer (1 MHz = 1us per tick)
     _rxTimer = timerBegin(1000000);

    if (_rxTimer == nullptr)
    {
        Serial.println("LT: Timer init failed!");
        return;
    }

    _timerInstance = this;

    timerAttachInterrupt(_rxTimer, RxTimeoutISR);

    // Stop timer until armed by the GPIO edge ISR
    timerStop(_rxTimer);

    // 4. Attach GPIO Edge Interrupt
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

void  ESP32LaserTag::RxEdgeISR(
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

void  ESP32LaserTag::RxTimeoutISR()
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

void  ESP32LaserTag::HandleRxTimeout()
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

void  ESP32LaserTag::ResetReceiver()
{
    _rxState = RxState::Idle;

    _rxBitCount = 0;

    _rxData = 0;

    _lastEdgeTime = 0;
}


/*
 * =============================================================
 * ARM RECEIVE TIMEOUT
 * =============================================================
 */

void  ESP32LaserTag::ArmRxTimeout()
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
     * Configure and enable a one-shot alarm.
     *
     * Arduino-ESP32 3.x combines the old:
     *
     *     timerAlarmWrite()
     *     timerAlarmEnable()
     *
     * into timerAlarm().
     *
     * The timer is running at 1 MHz, therefore one tick
     * represents one microsecond.
     */
    timerAlarm(
        _rxTimer,
        RX_TIMEOUT_US,
        false,
        0);
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
void  ESP32LaserTag::HandleRxEdge()
{
    // esp_timer_get_time() returns 64-bit microsecond timestamp safely inside IRAM
    const uint32_t now = (uint32_t)esp_timer_get_time();

    // gpio_get_level is faster and safer inside ISRs than digitalRead
    const bool level = gpio_get_level((gpio_num_t)_receivePin);


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


           _rxBitCount = _rxBitCount + 1;


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


    data.shotPowerIndex =
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
     *
     * Arduino-ESP32 3.x uses the pin rather than the
     * LEDC channel for ledcWrite().
     */
    ledcWrite(
        _transmitPin,
        128);
}


void ESP32LaserTag::CarrierOff()
{
    ledcWrite(
        _transmitPin,
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
            dataToSend.shotPowerIndex & 0x0F);


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

void ESP32LaserTag::AllOff()
{
    digitalWrite(_irLowPort, LOW);
    digitalWrite(_irMedPort, LOW);
    digitalWrite(_irHighPort, LOW);        
}

void ESP32LaserTag::SetIRPower(IRPowerLevel powerLevel)
{
    Serial.printf("Setting IR power to %d\n", static_cast<int>(powerLevel));
    AllOff();
    switch(powerLevel)
    {
        case IRPowerLevel::IRPowerLow:
        {
            digitalWrite(_irLowPort, HIGH);
        }
        break;
        
        case IRPowerLevel::IRPowerMedium:
        {
            digitalWrite(_irMedPort, HIGH);
        }
        break;
                
        
        case IRPowerLevel::IRPowerHigh:
        {
            digitalWrite(_irHighPort, HIGH);
        }
        break;

        default:
        {
            Serial.printf("Invalid ir power value\n");
            digitalWrite(_irLowPort, HIGH);
        }
    }
}
