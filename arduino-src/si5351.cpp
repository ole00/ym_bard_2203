
#include "si5351.h"

#if USE_SI5351_CLOCK
#include <Wire.h>
#include <Arduino.h>

#ifdef CONFIG_IDF_TARGET_ESP32S3
#define USE_ESP32S3
#endif

#ifdef USE_ESP32S3
#define PIN_SDA 11
#define PIN_SCL 12

#else
#define PIN_SDA A4
#define PIN_SCL A5
#endif

#define SI5351_ADDRESS 0x60

// Simple helper function to write data to a specific Si5351 register
static void writeSiRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(SI5351_ADDRESS);
  Wire.write(reg);   // Target register address
  Wire.write(value); // Data byte to write
  Wire.endTransmission();
}

static uint8_t readSiRegister(uint8_t regAddress) {
  // Step 1: Tell the Si5351 which register you want to read
  Wire.beginTransmission(SI5351_ADDRESS);
  Wire.write(regAddress);
  
  // The 'false' parameter sends a "Repeated Start".
  // This keeps the I2C line open so no other device interrupts the read.
  Wire.endTransmission(false); 

  // Step 2: Request 1 byte of data from the Si5351
  Wire.requestFrom(SI5351_ADDRESS, 1);
  
  // Step 3: Wait for the byte to arrive and return it
  if (Wire.available()) {
    return Wire.read();
  } 
  return 0xFF; // Return an error code if the bus fails to read
}

bool Si5351_init() {

    Wire.begin(PIN_SDA, PIN_SCL);

    uint8_t r = readSiRegister(183);

    //Chip not detected
    if (r == 0xFF) {
        return false;
    }
    // Disable all clock outputs first (Register 3)
    // Writing 0xFF turns off all outputs so we can configure safely    
    writeSiRegister(3, 0xFF);

    // Reset the internal Crystal Load Capacitance (Register 183)
    // Most breakout boards use a 10pF crystal (0x80 sets it to 10pF)
    writeSiRegister(183, 0x80);
    return true;
}


void Si5351_set_3_mhz() {

  writeSiRegister(3, 0xFF);

  // 2. Set Crystal Load Capacitance to 10pF (Register 183 = 0x80)
  writeSiRegister(183, 0x80);

  // Set PLL to 600 MHz (24 * 25 MHz crystal) and divide it by 200 to get 3MHz
  // 3. Split up the working Multiplier (PLL A) into single register writes
  // multiplier 24:  128 * 24 => 3072, 3072 - 512 => 2560 (0xA00)
  writeSiRegister(26, 0x00);
  writeSiRegister(27, 0x01);
  writeSiRegister(28, 0x00); // Top most byte
  writeSiRegister(29, 0x0A); // Mid byte
  writeSiRegister(30, 0x00); // Low byte

  // 4. Split up the working Divider (Multisynth 0) into single register writes
  // divider 200: 128 * 200 => 25600, 25600 - 512 => 25088 (0x6200)
  writeSiRegister(42, 0x00);
  writeSiRegister(43, 0x01);
  writeSiRegister(44, 0x00); // Top most byte
  writeSiRegister(45, 0x62); // Mid byte
  writeSiRegister(46, 0x00); // Low byte

  // 5. Configure CLK0 Control pin behavior (Register 16 = 0x4F)
  writeSiRegister(16, 0x4F);

  // 6. Reset both internal PLLs to synchronize (Register 177 = 0xAC)
  writeSiRegister(177, 0xAC);

  // 7. Enable CLK0 output pin (Register 3 = 0xFE)
  writeSiRegister(3, 0xFE);

}

#else
bool detectSi5351() {
    return false;
}
void Si5351_set_3_mhz() {
}
#endif