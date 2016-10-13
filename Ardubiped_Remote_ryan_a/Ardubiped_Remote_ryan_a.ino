#include <Adafruit_SSD1306.h>

#include <Adafruit_GFX.h>
#include <gfxfont.h>

 // ================================================================
// ===                      Library Includes                    ===
// ================================================================

#include <SPI.h>    // Library for SPI communications used by the nRF24L01 radio
#include <Wire.h>
#include <EEPROM.h>          // EEPROM library to store non volatile channel varaible
#include <RH_NRF24.h>        // Max we can send is 28 bytes of data 
RH_NRF24 nrf24(14, 10);      //CE, CSN

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
//#include <U8glib.h>
//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_NO_ACK|U8G_I2C_OPT_FAST);  // Fast I2C / TWI 

// ================================================================
// ===                      Robot Pin Defines                   ===
// ================================================================

#define PIN_A 2      // Right D pad up button
#define PIN_B 8      // Right D pad right button
#define PIN_C 9      // Right D pad down button
#define PIN_D 15     // Right D pad left button

#define PIN_UP 4     // Left D pad up button
#define PIN_RIGHT 7  // Left D pad right button
#define PIN_DOWN 5   // Left D pad down button
#define PIN_LEFT 6   // Left D pad left button

#define PIN_TOP_RIGHT 1 // Top Right function switch is on the TX pin
#define PIN_TOP_LEFT 0 // Top Left function switch is on the RX pin

#define PIN_F 1 // Top Right function switch is on the TX pin
#define PIN_E 0 // Top Left function switch is on the RX pin

#define Y_LED 3  // Yellow low batt indicator LED 
#define R_LED 17 // Red TX indicator LED 

#define HEAD_R_LED 23   // Red LED on head PCB Pin PB7 = D23
#define HEAD_G_LED 22   // Green LED on head PCB Pin PB6 = D22
#define HEAD_B_LED 16  // Blue LED on head PCB Pin PC2 = A2 = D16

#define PIN_HEAD_A 20 // *ANALOG ONLY PIN ADC6/A6/D20 - Must be read with analog read* A button on head PCB
#define PIN_HEAD_B 21 // *ANALOG ONLY PIN ADC7/A7/D21 - Must be read with analog read* B button on head PCB


#define A 0  // Right D pad up button
#define B 1  // Right D pad right button
#define C 2  // Right D pad down button
#define D 3  // Right D pad left button

#define UP 4    // Left D pad up button
#define RIGHT 5 // Left D pad right button
#define DOWN 6  // Left D pad down button
#define LEFT 7  // Left D pad left button

#define E 0 // Top Right function switch is on the TX pin
#define F 1 // Top Left function switch is on the RX pin

#define HEAD_A 2 // *ANALOG ONLY PIN ADC6/A6/D20 - Must be read with analog read* A button on head PCB
#define HEAD_B 3 // *ANALOG ONLY PIN ADC7/A7/D21 - Must be read with analog read* B button on head PCB

// ================================================================
// ===                  Variable Definitions                    ===
// ================================================================

uint8_t sendBuffer[2]; //array of unsigned 8-bit type - 28 is the max message length for the nrf24L01 radio

uint8_t receiveBuffer[7];
uint8_t len = sizeof(receiveBuffer);

unsigned char buttons = 0; // holds current value of all 8 buttons using bit values
unsigned char buttons2 = 0; // holds current value of all 8 buttons using bit values

unsigned char txChannel = 0; // Channel for the NRF24l01 transmitter 0-83 valid
int received; // the receive variable type must be the same as the type being received

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

/*
void draw(void) {
  // graphic commands to redraw the complete screen should be placed here  
  //u8g.setFont(u8g_font_unifont);
  u8g.setFont(u8g_font_helvB12); // Set font for print functions
  u8g.setFontPosTop(); // Sets the print and string commands to start below the reference point, vs above. 
  u8g.setPrintPos(0, 0); // Start back at the top of left corner of the screen
  u8g.print(("Buttons:  ")); //the F denotes that the string is stored in flash memory
  u8g.print(buttons);
  u8g.setPrintPos(0, 15);
  u8g.print("Buttons2: ");
  u8g.print(buttons2);
  u8g.setPrintPos(0, 30);
  u8g.print("Channel:  ");
  u8g.print(txChannel);
}
*/



void updateChannel(void)
{
  if (bitRead(buttons2, HEAD_A) == 1)   // If the button A on the head PCB is pushed 
  {
    txChannel--;                        // Decrement the txChannel
    if (txChannel > 83) txChannel = 83; // roll over from 0 to 83 to keep within valid range
    nrf24.setChannel(txChannel);       // Update the transmitter to use the new txChannel setting
    EEPROM.update(0, txChannel);       // Store the new value in EEPROM location 0 so it will be kept even after power off
  }
  else if (bitRead(buttons2, HEAD_B) == 1)// If the button B on the head PCB is pushed 
  {
    txChannel++;                       // Increment the txChannel
    if (txChannel > 83) txChannel = 0; // roll over from 83 to 0 to keep within valid range
    nrf24.setChannel(txChannel);       // Update the transmitter to use the new txChannel setting
    EEPROM.update(0, txChannel);       // Store the new value in EEPROM location 0 so it will be kept even after power off
  }

}
// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================

void setup()
{

// ================================================================
// ===                     Robot Pin Setup                      ===
// ================================================================

  pinMode(R_LED, OUTPUT);
  pinMode(Y_LED, OUTPUT);
  pinMode(HEAD_R_LED, OUTPUT);
  pinMode(HEAD_G_LED, OUTPUT);
  pinMode(HEAD_B_LED, OUTPUT);


  
  digitalWrite(R_LED, HIGH); // Turn on the Red LED - Active HIGH
  digitalWrite(Y_LED, HIGH); // Turn on the Red LED - Active HIGH
  digitalWrite(HEAD_R_LED, LOW); // Turn on the Red LED - Active LOW
  digitalWrite(HEAD_G_LED, LOW); // Turn on the Green LED
  digitalWrite(HEAD_B_LED, LOW); // Turn on the Blue LED
  
  pinMode(PIN_A, INPUT_PULLUP); // Set pushbutton pins to inputs
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_C, INPUT_PULLUP);
  pinMode(PIN_D, INPUT_PULLUP);
  
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP);

  pinMode(PIN_F, INPUT_PULLUP);
  pinMode(PIN_E, INPUT_PULLUP);

  pinMode(PIN_HEAD_A, INPUT_PULLUP);
  pinMode(PIN_HEAD_B, INPUT_PULLUP);
  
  
// ===================++===========================================
// ===             nrF34L01 Transceiver Setup                   ===
// ================================================================

  nrf24.init();
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  txChannel = EEPROM.read(0);
  nrf24.setChannel(txChannel); // Set the desired Transceiver channel valid values are 0-127, in the US only channels 0-83 are within legal bands
  nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm);    

// ===================++===========================================
// ===                     OLED Display Setup                   ===
// ================================================================

/*
  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }
*/

Serial.begin(9600);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  /*display.display();
  delay(2000);*/

  // Clear the buffer.
  display.clearDisplay();

// Turn off all LEDs  
  digitalWrite(R_LED, LOW); // Turn off the Red LED
  digitalWrite(Y_LED, LOW); // Turn off the Yellow LED
  digitalWrite(HEAD_R_LED, HIGH); // Turn off the Red LED - Active LOW
  digitalWrite(HEAD_G_LED, HIGH); // Turn off the Green LED
  digitalWrite(HEAD_B_LED, HIGH); // Turn off the Blue LED

  display.clearDisplay();
  
} // End setup function

// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================

void loop(){
/*  
// picture loop
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
  */
  updateChannel();
  
// ================================================================
// ===                       Read Buttons                       ===
// ================================================================

// We use the "bitWrite" function to change each bit individually in the "buttons" variable depending on the state of each button.  
// This allows us to store all 8 button values in one 8 bit variable which can then be easily sent to the robot for decoding. 
  bitWrite(buttons, A, !(digitalRead(PIN_A))); //So here the bit zero bit (right most bit) in "buttons" will be set to the current state of the button named "A"
  bitWrite(buttons, B, !(digitalRead(PIN_B)));
  bitWrite(buttons, C, !(digitalRead(PIN_C)));
  bitWrite(buttons, D, !(digitalRead(PIN_D)));
  bitWrite(buttons, UP, !(digitalRead(PIN_UP)));
  bitWrite(buttons, RIGHT, !(digitalRead(PIN_RIGHT))); 
  bitWrite(buttons, DOWN, !(digitalRead(PIN_DOWN)));
  bitWrite(buttons, LEFT, !(digitalRead(PIN_LEFT)));

  bitWrite(buttons2, F, !(digitalRead(PIN_F)));
  bitWrite(buttons2, E, !(digitalRead(PIN_E)));

  if (analogRead(PIN_HEAD_A) < 128) bitWrite(buttons2, HEAD_A, HIGH);
  else bitWrite(buttons2, HEAD_A, LOW);
  
  if (analogRead(PIN_HEAD_B) < 128) bitWrite(buttons2, HEAD_B, HIGH);
  else bitWrite(buttons2, HEAD_B, LOW);

  if ((buttons != 0)||(buttons2 != 0)) // If any buttons are being pushed
  {
    digitalWrite(R_LED, HIGH); // Turn on the Red LED
  }
  else
  {
    digitalWrite(R_LED, LOW); // Turn off the Red LED
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Buttons:  ");
  display.println(buttons);
  display.print("Buttons2:  ");
  display.println(buttons2);
  display.print("Channel:  ");
  display.println(txChannel);
  display.display();
  
// ================================================================
// ===                    Send Data to Robot                    ===
// ================================================================

  sendBuffer[0] = buttons; // Set the sendBuffer to the button state
  sendBuffer[1] = buttons2; // Set the sendBuffer to the button state
  nrf24.send(sendBuffer, sizeof(sendBuffer)); // Send the buffer
  nrf24.waitPacketSent();   // Wait for the radio to finish transmitting
  delay(20);

} // End main loop

