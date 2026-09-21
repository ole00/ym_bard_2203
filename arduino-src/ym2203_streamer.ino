// YM2203 streamer for Arduino Nano and Arduino NANO-ESP32 and compatible
// Streams up to two YM2203 chips.

#include "si5351.h"

#ifdef CONFIG_IDF_TARGET_ESP32S3
#define USE_ESP32S3
#endif

#ifdef _RENESAS_RA_
#include "pwm.h"
#define USE_RENESAS_RA
#endif


#ifdef USE_ESP32S3
#define USE_FAST_MCU
#define PIN_CS1 1
#define PIN_CS2 2

#define PIN_WR  47
#define PIN_RESET 3

#define PIN_DB0 5
#define PIN_DB1 6
#define PIN_DB2 7
#define PIN_DB3 8
#define PIN_DB4 9
#define PIN_DB5 10
#define PIN_DB6 17
#define PIN_DB7 21
#define PIN_ADDR 38
#define PIN_YM_CLK 18

#define PIN_BOARD_LED 45
#define LED_INVERSE 1

#elif defined(USE_RENESAS_RA)
// Renesas based Arduino NANO R4
#define USE_FAST_MCU
#define PIN_CS1 A0
#define PIN_CS2 A1

#define PIN_WR  12
#define PIN_RESET A2

#define PIN_DB0 2
#define PIN_DB1 3
#define PIN_DB2 4
#define PIN_DB3 5
#define PIN_DB4 6
#define PIN_DB5 7
#define PIN_DB6 8
#define PIN_DB7 10
#define PIN_ADDR 11
#define PIN_YM_CLK 9

#define PIN_BOARD_LED LED_BUILTIN
#define LED_INVERSE 0
#else
// Atmega based arduinos
#define PIN_CS1 A0
#define PIN_CS2 A1

#define PIN_WR  12
#define PIN_RESET A2

#define PIN_DB0 2
#define PIN_DB1 3
#define PIN_DB2 4
#define PIN_DB3 5
#define PIN_DB4 6
#define PIN_DB5 7
#define PIN_DB6 8
#define PIN_DB7 10
#define PIN_ADDR 11
#define PIN_YM_CLK 9

#define PIN_BOARD_LED LED_BUILTIN
#define LED_INVERSE 0

#define M_D0 (1 << PIN_DB0)
#define M_D1 (1 << PIN_DB1)
#define M_D2 (1 << PIN_DB2)
#define M_D3 (1 << PIN_DB3)
#define M_D4 (1 << PIN_DB4)
#define M_D5 (1 << PIN_DB5)
#define M_D6 (1 << (PIN_DB6 - 8))
#define M_D7 (1 << (PIN_DB7 - 8))
#define M_WR (1 << (PIN_WR - 8))
#define M_ADDR (1 << (PIN_ADDR - 8))

#endif


#define MAGIC 0xF3

#define XON  0x11
#define XOFF 0x13

// Hysteresis Levels for a 256-byte buffer
#define BUFFER_HIGH_WATER 96 // Send XOFF if buffer fills past this
#define BUFFER_LOW_WATER  14  // Send XON if buffer empties below this

#define LOCAL_FIFO_SIZE 128
uint8_t localFifo[LOCAL_FIFO_SIZE];
uint8_t fifoHead = 0;
uint8_t fifoTail = 0;
uint8_t parserState = 0;


bool masterTxPaused = false; // Tracks if we told Linux to stop

// Set up 3MHz clock for YM2203 on pin D9
#ifdef USE_ESP32S3
void setupPwm() {
    // 1. MANDATORY ELEMENT: Force the LEDC system driver to hook into 
    // the 80 MHz APB clock instead of the default 40 MHz XTAL clock source.
    ledcSetClockSource(LEDC_AUTO_CLK); 

    uint32_t frequency = 3000000; // Exactly 3,000,000 Hz
    uint8_t resolution_bits = 1;   // 4-bit resolution 

    // 2. Attach the pin to the high-speed clock bus
    ledcAttach(PIN_YM_CLK, frequency, resolution_bits);

    // 3. Write a 50% duty cycle square wave
    ledcWrite(PIN_YM_CLK, 1); 
}
#elif defined(USE_RENESAS_RA)
 //Arduino NANO R4 (Renesas ARM at 5V)
 void setupPwm() {
    PwmOut* high_speed_clock = new PwmOut(PIN_YM_CLK);

    // Configure HS clock via the core's native raw microsecond parameter initialization.
    // Arguments: (period_units, pulse_width_units, raw_mode_flag, timer_divider)
    // To get 3MHz signal from the 48MHz background bus clock (PCLKD):
    // Period Cycles = 48,000,000 / 3,000,000 = 16 cycles
    // Duty Cycle (50% Square Wave) = 16 / 2 = 8 cycles
    uint32_t period_cycles = 16;
    uint32_t pulse_cycles = 8;

    // Passing 'true' alerts the driver to use raw CPU clock cycles 
    // instead of abstract microsecond timing blocks.
    bool success = high_speed_clock->begin(
        period_cycles, 
        pulse_cycles, 
        true, 
        TIMER_SOURCE_DIV_1
    );
 }
#else
// Atmega 328p or compatible
// Can't produce 3MH clock, the closest match is 3.2MHz.
void setupPwm() {
  // Set Pin 9 (Port B, Pin 1) as an OUTPUT
  pinMode(PIN_YM_CLK, OUTPUT);

  // 1. Clear Timer 1 Control Registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // 2. Set Fast PWM mode with ICR1 as the TOP value (Mode 14)
  // WGM13 and WGM12 are in TCCR1B. WGM11 is in TCCR1A.
  TCCR1A |= (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << WGM12);

  // 3. Set Toggle/Clear behavior for Channel A (Pin 9)
  // This toggles Pin 9 on every match to create the 8 MHz wave
  TCCR1A |= (1 << COM1A1);

  // 4. Set TOP to 1. 
  // Formula: Frequency = Clock / (Prescaler * (1 + TOP))
  // 8,000,000 = 16,000,000 / (1 * (1 + 1))
  //ICR1 = 1; // 8MHz
  //ICR1 = 2; // 5.333 MHz
  //ICR1 = 3; // 4.0 MHz
  ICR1 = 4; // 3.2 MHz
  //ICR1 = 5; // 2.66 MHz
  //ICR1 = 6; // 2.28 MHz
  //ICR1 = 7; // 2 MHz
  //ICR1 = 8; // 1.77 MHz
  //ICR1 = 9; // 1.6 MHz
  //ICR1 = 10; // 1.45 MHz


  // 5. Set the Match value to 0 to trigger the toggle
  OCR1A = 1; // set to 0 for 8 MHz, set to 1 for up to 3.2 MHz, set to 2 for lower

  // 6. Start the timer with NO prescaler (divide clock by 1)
  TCCR1B |= (1 << CS10);
}
#endif


// Setup clock for YM2203. Use Si5251 clock generator if present, or else
// try to generate 3 MHz clock on pin D9.
static void setupClockOutput() {
    // Check whether we have Si5351 programmable clock.
    // If so, set 3 Mhz clock.
    if (Si5351_init()) {
        Si5351_set_3_mhz();
        digitalWrite(PIN_BOARD_LED, LED_INVERSE ? LOW : HIGH);
    } else
    // Fallback method of using PWM based clock.
    // Closest possible frequency is 3.2 MHz on Atmel arduinos, so music is a bit higher pitch.
    // ESP32-S3 based Arduinos can generate precise 3MHz clock frequency on their own.
    {    
        digitalWrite(PIN_BOARD_LED, LED_INVERSE ? HIGH : LOW); 
        setupPwm();
    }
}

// read bytes from serial port into an internal FIFO buffer
static inline uint8_t prefetchSerialBytes(uint8_t max, bool wait = false) {
#ifdef USE_FAST_MCU
    unsigned long time;
    unsigned long startWait = micros();
#endif    
    while (Serial.available() > 0) {
        uint8_t nextHead = (fifoHead + 1) & 0x7F; //% LOCAL_FIFO_SIZE;
        if (nextHead != fifoTail) { // Prevent local overflow
            localFifo[fifoHead] = Serial.read();
            fifoHead = nextHead;
            max--;
            if (!max) {
                break;
            }
        } else {
            break; // Local FIFO is full, stop reading
        }
    }
#ifdef USE_FAST_MCU
    if (wait) {
        time = (micros() - startWait);
        if (time < max)  {
            delayMicroseconds(max - time);
        }
    }
#endif    

    return max;
}

#ifdef USE_FAST_MCU
// Write a byte to the data bus, also set the address bit.
// This is a generic function for fast MCUs.
static void writeDataBus(uint8_t value, uint8_t data) {
    digitalWrite(PIN_ADDR, data); // Address 0,  Data 1
    prefetchSerialBytes(12, true);

    digitalWrite(PIN_DB0, value & 1);
    value >>= 1;
    digitalWrite(PIN_DB1, value & 1);
    value >>= 1;
    digitalWrite(PIN_DB2, value & 1);
    value >>= 1;
    digitalWrite(PIN_DB3, value & 1);
    value >>= 1;
    digitalWrite(PIN_DB4, value & 1);
    value >>= 1;
    digitalWrite(PIN_DB5, value & 1);
    value >>= 1;
    digitalWrite(PIN_DB6, value & 1);
    value >>= 1;
    digitalWrite(PIN_DB7, value & 1);

    prefetchSerialBytes(12, true);

    digitalWrite(PIN_WR, 0);

    prefetchSerialBytes(8, true);
    digitalWrite(PIN_WR, 1);
}
#else
// Write a byte to the data bus, also set the address bit.
// This is Atmel specific function with fast GPIO access.
static void writeDataBus(uint8_t value, uint8_t data) {
    // set Address line
    if (data) {
        PORTB |= M_ADDR;
    } else {
        PORTB &= ~M_ADDR;
    }
    prefetchSerialBytes(12);

    // set data bus
    if (value & 1) {
        PORTD |= M_D0;
    } else {
        PORTD &= ~M_D0;
    }
    value >>= 1;

    if (value & 1) {
        PORTD |= M_D1;
    } else {
        PORTD &= ~M_D1;
    }
    value >>= 1;

    if (value & 1) {
        PORTD |= M_D2;
    } else {
        PORTD &= ~M_D2;
    }
    value >>= 1;

    if (value & 1) {
        PORTD |= M_D3;
    } else {
        PORTD &= ~M_D3;
    }
    value >>= 1;

    if (value & 1) {
        PORTD |= M_D4;
    } else {
        PORTD &= ~M_D4;
    }
    value >>= 1;

    if (value & 1) {
        PORTD |= M_D5;
    } else {
        PORTD &= ~M_D5;
    }
    value >>= 1;

    if (value & 1) {
        PORTB |= M_D6;
    } else {
        PORTB &= ~M_D6;
    }
    value >>= 1;

    if (value & 1) {
        PORTB |= M_D7;
    } else {
        PORTB &= ~M_D7;
    }
    prefetchSerialBytes(12);

    // pulse Write line
    PORTB &= ~M_WR;
    prefetchSerialBytes(8);
    PORTB |= M_WR;
}
#endif

// High level function that writes a byte to specific YM2203 register.
// chip: 0 or 1 - which YM2203 to target
// addr: register address
// value: register value
static void writeRegister(uint8_t chip, uint8_t addr, uint8_t value) {
    uint8_t csPin = chip ? PIN_CS2 : PIN_CS1;
    digitalWrite(csPin, LOW); 

    writeDataBus(addr, 0);
    writeDataBus(value, 1);

    digitalWrite(csPin, HIGH); 
}

// Reset both YM2203 chips by toggling their Reset pin.
static void resetFm() {
    digitalWrite(PIN_RESET, HIGH);  // Release Reset
    delay(10); 
    digitalWrite(PIN_RESET, LOW);   // Pull Reset low
    delay(5);                       // Hold for a few milliseconds
    digitalWrite(PIN_RESET, HIGH);  // Release Reset
    delay(10); 

}

void setup() {
    Serial.begin(115200);

    pinMode(PIN_BOARD_LED, OUTPUT);
    setupClockOutput();

    // Set Chip select pins
    pinMode(PIN_CS1, OUTPUT);   
    digitalWrite(PIN_CS1, HIGH); 

    pinMode(PIN_CS2, OUTPUT);   
    digitalWrite(PIN_CS2, HIGH); 

    // Setup Write pin
    pinMode(PIN_WR, OUTPUT);
    digitalWrite(PIN_WR, HIGH);

    // Setup data bus pins
    pinMode(PIN_DB0, OUTPUT);
    pinMode(PIN_DB1, OUTPUT);
    pinMode(PIN_DB2, OUTPUT);
    pinMode(PIN_DB3, OUTPUT);
    pinMode(PIN_DB4, OUTPUT);
    pinMode(PIN_DB5, OUTPUT);
    pinMode(PIN_DB6, OUTPUT);
    pinMode(PIN_DB7, OUTPUT);
    pinMode(PIN_ADDR, OUTPUT);

    pinMode(PIN_RESET, OUTPUT);

    resetFm();
    Serial.write(XON);
    localFifo[0] = 0;
    fifoHead = 0;
    fifoTail = 0;
    parserState = 0;
}

void loop() {

    // Try to read serial port bytes into local FIFO buffer.
    prefetchSerialBytes(32);

    // Process bytes from our local buffer
    while (fifoTail != fifoHead) {
        uint8_t streamData;
        uint8_t streamAddr;
        uint8_t streamChip;
        uint8_t packetType;
        uint8_t b = localFifo[fifoTail];
        uint8_t buffered = fifoHead > fifoTail ? fifoHead - fifoTail : fifoHead + LOCAL_FIFO_SIZE - fifoTail;
        fifoTail = (fifoTail + 1) & 0x7F; //% LOCAL_FIFO_SIZE;


        // Check if the buffer is filling up too fast
        if (!masterTxPaused && buffered > BUFFER_HIGH_WATER) {
            Serial.write(XOFF); // Tell master to FREEZE transmission
            masterTxPaused = true;
        } else
        if (masterTxPaused && buffered < BUFFER_LOW_WATER) {
            Serial.write(XON); // Tell master it is safe to RESUME pushing data
            masterTxPaused = false;
        }
        switch (parserState) {
            // first byte: Top nibble is MAGIC (0xF), bit 3 defines packet type, bits 2 to 0
            // define FM chip index.
            case 0: 
                packetType = (b & 0xF8);
                // short 3 byte packet
                if (packetType == 0xF0) {
                    parserState = 1;
                } 
                 else
                // long 12 byte packet
                if (packetType == 0xF8) {
                    parserState = 3;
                }               
                streamChip = b & 0x7;
                prefetchSerialBytes(12);
                break;
            // Short packet: 2nd byte - read register address to set
            case 1:
                streamAddr = b;
                parserState = 2;
                prefetchSerialBytes(12);
                break;
            // Short packet: 3rd byte - read register value to set
            // then write the value to the register
            case 2: 
                streamData = b;
                // special combination of address and data resets the YM chips
                if (streamAddr == 0xFF && streamData == 0xFF) {
                    resetFm();
                } else {
                    writeRegister(streamChip, streamAddr, streamData);
                }
                parserState = 0; 
                break;
            default:
                // handle data bytes of a long packet
                // 11 bytes of register 0 to register 10 are processed
                streamAddr = parserState - 3;
                // check exit condition that forces to read header magic
                if (streamAddr == 10) {
                    parserState = 0; // read header magic
                } else {
                    parserState++;
                }
                writeRegister(streamChip, streamAddr, b);
        }
    }  

}
