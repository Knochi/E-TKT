// MIT License

// Copyright (c) 2022 Andrei Speridião

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// for more information, please visit https://github.com/andreisperid/E-TKT
//

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// ~~~~~ IMPORTANT: do not forget to upload the files in "data" folder using SPIFFS ~~~~~
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// libraries
#include <Arduino.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include "SPIFFS.h"
#include <qrcode.h>
#include <U8g2lib.h>
#include <ESP32Tone.h>
#include <Preferences.h>

// extension files
#include "etktLogo.cpp" // etkt logo in binary format
#include "pitches.cpp"	// list of notes and their frequencies

// HARDWARE -----------------------------------------------------------------------

#define MICROSTEP_Feed 8
#define MICROSTEP_Char 16 // for more precision, maximum available microsteps for the character stepper

// home sensor
#define sensorPin 34 // hall sensor
int sensorState;
#define threshold 128 // between 0 and 1023
int currentCharPosition = -1;
int deltaPosition;

// wifi reset
#define wifiResetPin 13 // tact switch

// steppers
#define stepsPerRevolutionFeed 4076
const int stepsPerRevolutionChar = 200 * MICROSTEP_Char;
AccelStepper stepperFeed(MICROSTEP_Feed, 15, 2, 16, 4);
AccelStepper stepperChar(1, 32, 33);
#define enableCharStepper 25
float stepsPerChar;

// servo
Servo myServo;
#define servoPin 14
#define restAngle 50
#define targetAngle 22
int peakAngle;

// oled display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#define Lcd_X 128
#define Lcd_Y 64

// leds
#define ledFinish 5
#define ledChar 17

// buzzer
#define buzzerPin 26

// DEBUG --------------------------------------------------------------------------
// comment lines individually to isolate functions

#define do_cut
#define do_press
#define do_char
#define do_feed
#define do_sound
// #define do_wifi_debug
// #define do_display_debug
// #define do_serial

// DATA ---------------------------------------------------------------------------

// char
#define charQuantity 43 // the amount of teeth/characters in the carousel

// conversion table to prevent multichar error
// complex chars gets transformed into simple ones (not present in the carousel) to ease transmission and parsing
// ♡ ... <
// ☆ ... >
// ♪ ... ~
// € ... |

// list of characters in the caroulsel
// important: keep in mind the home is always on 21st character (J)
char charSet[charQuantity] = {
	'$', '-', '.', '2', '3', '4', '5', '6', '7', '8',
	'9', '*', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
	'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '<', '>',
	'~', '|', '@'};

String labelString;
char prevChar = 'J';
const int charHome = 21;
bool waitingLabel = false;

volatile bool busy = false; // volatile as the variable is read by both cores (0 and 1)

String parameter = "";
String value = "";
TaskHandle_t processorTaskHandle = NULL; // for dual core operation

// memory storage for align and force settings
Preferences preferences;
#define defaultAlign 5 // 1 to 9, 5 is the mid value
#define defaultForce 1 // 1 to 9
int alignFactor = defaultAlign;
int forceFactor = defaultForce;
int newAlign = defaultAlign;
int newForce = defaultForce;
String combinedSettings = "x";

// E-TKT musical signature
int etktNotes[8] = {
	44, 44, 16, 2, 31, 22, 31, 44};

// selected musical scale for playing tunes
int charNoteSet[charQuantity + 1] = {
	G4, G6, A4, D4, E4, F4, G5, A5, B5, C5, D5, 0, E5, F5, C6, D6, E6, F6, A6, B6, C4, C7, D7, E7, F7, G7, B4, A7, B7, C8, D8, C3, D3, E3, F3, G3, A3, B3, E2, F2, G2, A2, B2, 0};

// --------------------------------------------------------------------------------
// ASSEMBLY -----------------------------------------------------------------------
// adjust both values below for a rough starting point

// depending on the hall sensor positioning, the variable below makes sure the initial calibration is within tolerance
// use a value between -1.0 and 1.0 to make it roughly align during assembly
float assemblyCalibrationAlign = 0.5;

// depending on servo characteristics and P_press assembling process, the pressing angle might not be so precise and the value below compensates it
// use a value between 0 and 20 to make sure the press is barely touching the daisy wheel on test align
int assemblyCalibrationForce = 15;

// after that, proceed to fine tune on the E-TKT's app when the machine is fully assembled

// --------------------------------------------------------------------------------
// COMMUNICATION ------------------------------------------------------------------

// create AsyncWebServer object on port 80
AsyncWebServer server(80);
DNSServer dns;

String webProgress = " 0";

// qr code for accessing the webapp
const int QRcode_Version = 3; //  set the version (range 1->40)
const int QRcode_ECC = 2;	  //  set the Error Correction level (range 0-3) or symbolic (ECC_LOW, ECC_MEDIUM, ECC_QUARTILE and ECC_HIGH)
QRCode qrcode;				  //  create the QR code
String displaySSID = "";
String displayIP = "";

// --------------------------------------------------------------------------------
// LEDS ---------------------------------------------------------------------------

void lightFinished()
{
	// lights up when the label has finished printing

	bool state = true;
	for (int i = 0; i < 10; i++)
	{
		analogWrite(ledFinish, state * 128);
		state = !state;
		delay(100);
	}
	for (int i = 128; i >= 0; i--)
	{
		analogWrite(ledFinish, i);
		state = !state;
		delay(25);
	}

	webProgress = " 0";
}

void lightChar(float state)
{
	// lights up when the character is pressed against the tape
	int s = state * 128;

	analogWrite(ledChar, s);
}

// --------------------------------------------------------------------------------
// BUZZER -------------------------------------------------------------------------

void sound(int frequency = 2000, int duration = 1000)
{
	tone(buzzerPin, frequency, duration);
}

void labelMusic(String label)
{
	// plays a music according to the label letters

	int length = label.length();

	for (int i = 0; i < length; i++)
	{
		for (int j = 0; j < charQuantity; j++)
		{
			if (label[i] == charSet[j])
			{
#ifdef do_serial
				Serial.println(charNoteSet[j]);
#endif
				tone(buzzerPin, charNoteSet[j], 100);
			}
		}
		delay(50);
	}
}

void eggMusic(String notes, String durations)
{
	// ♪ By pressing down a special key ♪
	// ♪ It plays a little melody ♪

	int length = notes.length();

	for (int i = 0; i < length; i++)
	{
		for (int j = 0; j < charQuantity; j++)
		{
			if (notes[i] == charSet[j])
			{
				char charDuration = durations[i];
				float duration = 2000 / atoi(&charDuration);
				tone(buzzerPin, charNoteSet[j], duration);
			}
		}
	}
}

// --------------------------------------------------------------------------------
// DISPLAY ------------------------------------------------------------------------

void displayClear(int color = 0)
{
	// paints all pixels according to the desired target color

	for (uint8_t y = 0; y < 64; y++)
	{
		for (uint8_t x = 0; x < 128; x++)
		{
			u8g2.setDrawColor(color);
			u8g2.drawPixel(x, y);
		}
	}
	delay(100);

	u8g2.setDrawColor(color == 0 ? 1 : 0);
	u8g2.setFont(u8g2_font_6x13_te);
}

void displayInitialize()
{
	// starts and sets up the display

	u8g2.begin();
	u8g2.clearBuffer();
	u8g2.setContrast(8); // 0 > 255
	displayClear();
	u8g2.setDrawColor(1);
}

void displaySplash()
{
	// initial start screen

	displayInitialize();

	// invert colors
	displayClear(1);

	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.setDrawColor(0);
	u8g2.drawStr(40, 53, "andrei.cc");
	u8g2.sendBuffer();

	u8g2.setDrawColor(1);

	int n = 1;

	// animated splash
	for (int i = 128; i > 7; i = i - 18)
	{
		for (int j = 0; j < 18; j += 9)
		{
			u8g2.drawXBM(i - j - 11, 8, 128, 32, etktLogo);
			u8g2.sendBuffer();
		}
		if (charNoteSet[etktNotes[n]] != 44)
		{
#ifdef do_sound
			sound(charNoteSet[etktNotes[n]], 200);
#else
			delay(200);
#endif
		}
		n++;
	}

	// draw a box with subtractive color
	u8g2.setDrawColor(2);
	u8g2.drawBox(0, 0, 128, 64);
	u8g2.sendBuffer();

#ifdef do_sound
	sound(3000, 150);
#else
	delay(150);
#endif
}

void displayConfig()
{
	// screen for the wifi configuration mode

	displayClear();

	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(15, 12, "WI-FI SETUP");
	u8g2.drawStr(3, 32, "Please, connect to");
	u8g2.drawStr(3, 47, "the \"E-TKT\" network...");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(3, 12, 0x011a);

	u8g2.sendBuffer();
}

void displayReset()
{
	// screen for the wifi configuration reset confirmation

	displayClear();

	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(15, 12, "WI-FI RESET");
	u8g2.drawStr(3, 32, "Connection cleared!");
	u8g2.drawStr(3, 47, "Release the button.");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(3, 12, 0x00cd);

	u8g2.sendBuffer();
}

void displayQRCode()
{
	// main screen with qr code, network and attributed ip

	displayClear();
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);

	uint8_t qrcodeData[qrcode_getBufferSize(QRcode_Version)];

	if (displayIP != "")
	{
		u8g2.setDrawColor(1);

		u8g2.drawStr(14, 15, "E-TKT");
		u8g2.setDrawColor(2);
		u8g2.drawFrame(3, 3, 50, 15);
		u8g2.setDrawColor(1);

		u8g2.drawStr(14, 31, "ready");

		String resizeSSID;
		if (displaySSID.length() > 8)
		{
			resizeSSID = displaySSID.substring(0, 7) + "...";
		}
		else
		{
			resizeSSID = displaySSID;
		}
		const char *d = resizeSSID.c_str();
		u8g2.drawStr(14, 46, d);

		const char *b = displayIP.c_str();
		u8g2.drawStr(3, 61, b);

		u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
		u8g2.drawGlyph(3, 46, 0x00f8);
		u8g2.drawGlyph(3, 31, 0x0073);

		String ipFull = "http://" + displayIP;
		const char *c = ipFull.c_str();
		qrcode_initText(&qrcode, qrcodeData, QRcode_Version, QRcode_ECC, c);

		// qr code background
		for (uint8_t y = 0; y < 64; y++)
		{
			for (uint8_t x = 0; x < 64; x++)
			{
				u8g2.setDrawColor(0);
				u8g2.drawPixel(x + 128 - 64, y);
			}
		}

		// setup the top right corner of the QRcode
		uint8_t x0 = 128 - 64 + 6;
		uint8_t y0 = 3;

		// display QRcode
		for (uint8_t y = 0; y < qrcode.size; y++)
		{
			for (uint8_t x = 0; x < qrcode.size; x++)
			{

				int newX = x0 + (x * 2);
				int newY = y0 + (y * 2);

				if (qrcode_getModule(&qrcode, x, y))
				{
					u8g2.setDrawColor(1);
					u8g2.drawBox(newX, newY, 2, 2);
				}
				else
				{
					u8g2.setDrawColor(0);
					u8g2.drawBox(newX, newY, 2, 2);
				}
			}
		}
	}
	u8g2.sendBuffer();
	delay(1000);
}

void displayProgress(float total, float actual, String label)
{
	// screen with the label being printed and its progress

	displayClear();

	u8g2.setDrawColor(1);

	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(15, 12, "PRINTING");
	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(3, 12, 0x0081);

	u8g2.setFont(u8g2_font_6x13_te);
	const char *c = label.c_str();
	u8g2.drawStr(0, 36, c);

	u8g2.setDrawColor(2);
	u8g2.drawBox(0, 21, (actual)*6, 21);

	float progress = actual / total;
	progress = progress * 100;

	// String progressString = String(progress * 95, 0);
	webProgress = String(progress, 0);

	if (progress > 0)
	{
		progress -= 1; // avoid 100% progress while still finishing
	}
	String progressString = String(progress, 0);

	progressString.concat("%");
	const char *p = progressString.c_str();
	u8g2.setDrawColor(1);
	u8g2.drawStr(6, 60, p);

	u8g2.sendBuffer();
}

void displayFinished()
{
	// screen with finish confirmation

	displayClear(1);
	u8g2.setDrawColor(0);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	webProgress = "finished";
	u8g2.drawStr(42, 37, "FINISHED!");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(27, 37, 0x0073);
	u8g2.drawGlyph(90, 37, 0x0073);

	u8g2.sendBuffer();
}

void displayCut()
{
	// screen for manual cut mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(44, 37, "CUTTING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, 37, 0x00f2);
	u8g2.drawGlyph(90, 37, 0x00f2);

	u8g2.sendBuffer();
}

void displayFeed()
{
	// screen for manual feed mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(44, 37, "FEEDING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, 37, 0x006e);
	u8g2.drawGlyph(90, 37, 0x006e);

	u8g2.sendBuffer();
}

void displayReel()
{
	// screen for reeling mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(44, 37, "REELING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, 37, 0x00d5);
	u8g2.drawGlyph(90, 37, 0x00d5);

	u8g2.sendBuffer();
}

void displayTest(int a, int f)
{
	// screen for settings test mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(44, 37, "TESTING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, 37, 0x0073);
	u8g2.drawGlyph(90, 37, 0x0073);

	u8g2.sendBuffer();
}

void displaySettings(int a, int f)
{
	// screen for settings save mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(47, 17, "SAVED!");

	String alignString = "ALIGN: ";
	alignString.concat(a);
	const char *alignChar = alignString.c_str();
	u8g2.drawStr(44, 37, alignChar);

	String forceString = "FORCE: ";
	forceString.concat(f);
	const char *forceChar = forceString.c_str();
	u8g2.drawStr(42, 57, forceChar);

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(33, 16, 0x0073);
	u8g2.drawGlyph(83, 16, 0x0073);

	u8g2.sendBuffer();
	delay(3000);
}

void displayReboot()
{
	// screen for imminent reboot

	displayClear(0);

	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(38, 37, "REBOOTING...");

	u8g2.sendBuffer();
	delay(2000);
}

void debugDisplay()
{
	// displaySplash();
	// delay(2000);

	// displayReset();
	// delay(2000);

	// displayConfig();
	// delay(2000);

	// displayIP = "192.168.69.69";
	// displaySSID = "Network";
	// displayQRCode();
	// delay(2000);

	// displayProgress(7, 5, " E-TKT ");
	// delay(5000);

	// displayFinished();
	// delay(2000);

	// displayCut();
	// delay(2000);

	// displayFeed();
	// delay(2000);

	// displayReel();
	// delay(2000);

	// displaySettings(5, 2);
	// delay(2000);

	// displayReboot();
	// delay(2000);
}

// --------------------------------------------------------------------------------
// MECHANICS ----------------------------------------------------------------------

void setHome(int align = alignFactor)
{
	// runs the char stepper clockwise until triggering the hall sensor, then call it home at char 21

	float a = (align - 5.0f) / 10.0f;

#ifdef do_serial
	Serial.print("align: ");
	Serial.println(align);

	Serial.print("homing with a = ");
	Serial.println(a);
#endif

	sensorState = analogRead(sensorPin);

	if (sensorState < threshold)
	{
		stepperChar.runToNewPosition(-stepsPerChar * 4);
		stepperChar.run();
	}
	sensorState = analogRead(sensorPin);

	stepperChar.move(-stepsPerRevolutionChar * 1.5f);
	while (sensorState > threshold)
	{
		sensorState = analogRead(sensorPin);
		stepperChar.run();
		delayMicroseconds(100); // TODO: less intrusive way to avoid triggering watchdog?
	}
	stepperChar.setCurrentPosition(0);
	sensorState = analogRead(sensorPin);

	stepperChar.runToNewPosition(-stepsPerChar + (stepsPerChar * a) + (assemblyCalibrationAlign * stepsPerChar));
	stepperChar.run();
	stepperChar.setCurrentPosition(0);
	currentCharPosition = charHome;

	delay(100);
}

void feedLabel(int repeat = 1)
{
	// runs the feed stepper by a specific amount to push the tape forward

#ifdef do_serial
	Serial.print("feed ");
	Serial.print(repeat);
	Serial.println("x");
#endif

	stepperFeed.enableOutputs();
	delay(10);

	for (int i = 0; i < repeat; i++)
	{
		stepperFeed.runToNewPosition((stepperFeed.currentPosition() - stepsPerRevolutionFeed / 8) * 1);
		delay(10);
	}

	stepperFeed.disableOutputs();

	delay(20);
}

void pressLabel(bool strong = false, int force = forceFactor, bool slow = false)
{
	// press the label

#ifdef do_serial
	Serial.println("press");
#endif

	int delayFactor = 0;

	peakAngle = targetAngle + 9 - assemblyCalibrationForce - force;

	if (strong)
	{
		delayFactor = 4;
	}
	else
	{
		delayFactor = slow ? 100 : 0;
	}
	lightChar(1.0f); // lights up the char led

	for (int pos = restAngle; pos >= peakAngle; pos--)
	{
		myServo.write(pos);
		delay(delayFactor);
	}
	for (int i = 0; i < 5; i++) // to make sure the servo has reached the peak position
	{
		myServo.write(peakAngle);
		delay(50);
	}

	for (int pos = peakAngle; pos <= restAngle; pos++)
	{
		myServo.write(pos);
		delay(delayFactor);
	}
	for (int i = 0; i < 5; i++) // to make sure the servo has reached the rest position
	{
		myServo.write(restAngle);
		delay(50);
	}

	lightChar(0.2f); // dims the char led
}

void goToCharacter(char c, int override = alignFactor)
{
	// reaches out for a specific character

#ifdef do_serial
	Serial.println("char");
#endif

	// calls home everytime to avoid accumulating errors
	setHome(override);

	// optimizes O and 0, I and 1
	if (c == '0')
	{
		c = 'O';
	}
	else if (c == '1')
	{
		c = 'I';
	}

	// matches the character to the list and gets delta steps from home
	for (int i = 0; i < charQuantity; i++)
	{
		if (c == charSet[i])
		{
			deltaPosition = i - currentCharPosition;
			currentCharPosition = i;
		}
	}

	if (deltaPosition < 0)
	{
		deltaPosition += 43;
	}

	// runs char stepper clockwise to reach the target position
	stepperChar.runToNewPosition(-stepsPerChar * deltaPosition);

	delay(25);
}

void cutLabel()
{
	// moves to a specific char (*) then presses label three times (more vigorously)

	goToCharacter('*');

	for (int i = 0; i < 3; i++)
	{
		pressLabel(true);
	}
}

void writeLabel(String label)
{
	// manages entirely a particular label printing process, from start to end (task kill)

	// enables servo
	myServo.write(restAngle);
	delay(500);

#ifdef do_serial
	Serial.print("print ");
	Serial.println(label);
#endif

	// all possible characters: $-.23456789*abcdefghijklmnopqrstuvwxyz♡☆♪€@

	int labelLength = label.length();

	lightChar(0.2f);

	// the webapp uses underscores instead of spaces, here they are changed back to spaces
	for (int i = 0; i < labelLength; i++)
	{
		if (label[i] == '_')
		{
			label[i] = ' ';
		}
	}

	displayInitialize();
	displayProgress(labelLength, 0, label);

#ifdef do_sound
	if (label == " TASCHENRECHNER " || label == " POCKET CALCULATOR " || label == " DENTAKU " || label == " CALCULADORA " || label == " MINI CALCULATEUR ")
	{
		eggMusic("*4599845887*459984588764599845887*4599845887", "88843888484888438884848884388848488843888484"); // ♪ I'm the operator with my pocket calculator ♪
	}
	else
	{
		labelMusic(label);
	}
#else
	delay(200);
#endif

	// enable and home char stepper
	stepperChar.enableOutputs();
	setHome();

#ifdef do_feed
	feedLabel();
#else
	delay(500);
#endif

	//
	for (int i = 0; i < labelLength; i++)
	{
		if (label[i] != ' ' && label[i] != '_' && label[i] != prevChar) // skip char seeking on repeated characters or on spaces
		{
#ifdef do_char
			goToCharacter(label[i]);
#else
			delay(500);
#endif
			prevChar = label[i];
		}

		if (label[i] != ' ' && label[i] != '_') // skip pressing on label spaces
		{

#ifdef do_press
			pressLabel();
#else
			delay(500);
#endif
		}

#ifdef do_feed
		feedLabel();
#else
		delay(500);
#endif
		delay(500);

		displayProgress(labelLength, i + 1, label);
	}

	if (labelLength < 6 && labelLength != 1) // minimum label length to make sure the user can grab it
	{
		int spaceDelta = 6 - labelLength;
		for (int i = 0; i < spaceDelta; i++)
		{
#ifdef do_feed
			feedLabel();
#else
			delay(500);
#endif
		}
	}

#ifdef do_cut
	cutLabel();
#else
	delay(500);
#endif

	lightChar(0.0f);

	// reset server parameters
	parameter = "";
	value = "";
	myServo.write(restAngle);

	stepperChar.disableOutputs();

	displayFinished();
	lightFinished();
	busy = false;
	displayQRCode();
	vTaskDelete(processorTaskHandle); // delete task
}

// --------------------------------------------------------------------------------
// DATA ---------------------------------------------------------------------------

void loadSettings()
{
	// load settings from internal memory

	preferences.begin("calibration", false);

	// check if are there any align and force values stored in memory
	int a = preferences.getUInt("align", 0);
	if (a == 0)
	{
		preferences.putUInt("align", defaultAlign);
	}
	int f = preferences.getUInt("force", 0);
	if (f == 0)
	{
		preferences.putUInt("force", defaultForce);
	}

	alignFactor = preferences.getUInt("align", 0);
	forceFactor = preferences.getUInt("force", 0);
	preferences.end();

	combinedSettings = alignFactor;
	combinedSettings.concat(forceFactor);

#ifdef do_serial
	Serial.print("combined settings (align/force) ");
	Serial.println(combinedSettings);
#endif
}

void testSettings(int tempAlign, int tempForce, bool full = true)
{
	// test settings (align or align + force) without writing to the memory

#ifdef do_serial
	Serial.print("testing align ");
	Serial.print(tempAlign);
	Serial.print(" and force ");
	Serial.println(tempForce);
#endif

	stepperChar.enableOutputs();

	if (full)
	{
		// feedLabel(4);
		feedLabel(2);
		goToCharacter('E', tempAlign);
		pressLabel(false, tempForce, false);
		feedLabel();
		goToCharacter('-', tempAlign);
		pressLabel(false, tempForce, false);
		feedLabel();
		goToCharacter('T', tempAlign);
		pressLabel(false, tempForce, false);
		feedLabel();
		goToCharacter('K', tempAlign);
		pressLabel(false, tempForce, false);
		feedLabel();
		goToCharacter('T', tempAlign);
		pressLabel(false, tempForce, false);
		feedLabel(2);
		goToCharacter('*', tempAlign);
		pressLabel(true, tempForce, false);
		pressLabel(true, tempForce, false);
		pressLabel(true, tempForce, false);
	}
	else
	{
		goToCharacter('M', tempAlign);
		pressLabel(false, 1, true);
	}

	stepperChar.disableOutputs();
}

void saveSettings(int tempAlign, int tempForce)
{
	// save settings to memory

	preferences.begin("calibration", false);

	preferences.putUInt("align", tempAlign);
	preferences.putUInt("force", tempForce);

	alignFactor = tempAlign;
	forceFactor = tempForce;

#ifdef do_serial
	Serial.print("saved align ");
	Serial.print(alignFactor);
	Serial.print(" and force ");
	Serial.println(forceFactor);
#endif

	preferences.end();
}

void interpretSettings()
{
	// interpret strings coming from the web app
	String a = value.substring(0, 1);
	String f = value.substring(2, 3);
	newAlign = (int)a.toInt();
	newForce = (int)f.toInt();

#ifdef do_serial
	Serial.print("align: ");
	Serial.print(newAlign);
	Serial.print(", force: ");
	Serial.println(newForce);
#endif
}

void processor(void *parameters)
{
	// receives and treats commands from the webapp

	for (;;)
	{
		if (parameter == "feed")
		{
			// feeds the tape by 1 character length
			busy = true;

			displayInitialize();
			displayFeed();

			analogWrite(ledFinish, 32);
			myServo.write(restAngle);
			delay(500);

#ifdef do_feed
			feedLabel();
#else
			delay(500);
#endif

			parameter = "";
			value = "";
			stepperChar.disableOutputs();
			myServo.write(restAngle);

			busy = false;
			webProgress = "finished";
			delay(500);
			webProgress = " 0";
			analogWrite(ledFinish, 0);
			lightChar(0.0f);
			displayQRCode();
			vTaskDelete(processorTaskHandle); // delete task
		}
		else if (parameter == "reel")
		{
			// feeds the tape in a fixed length from the cog to the press
			busy = true;

			displayInitialize();
			displayReel();
			analogWrite(ledFinish, 32);
			myServo.write(restAngle);
			delay(500);

			for (int i = 0; i < 16; i++)
			{
#ifdef do_feed
				feedLabel();
#else
				delay(500);
#endif
			}

			parameter = "";
			value = "";
			stepperChar.disableOutputs();
			busy = false;
			webProgress = "finished";
			delay(500);
			webProgress = " 0";
			analogWrite(ledFinish, 0);
			lightChar(0.0f);
			displayQRCode();
			vTaskDelete(processorTaskHandle); // delete task
		}
		else if (parameter == "cut")
		{
			// manually cuts the tape in the current tape position
			busy = true;

			displayInitialize();
			displayCut();

			lightChar(0.2f);
			stepperChar.enableOutputs();
			myServo.write(restAngle);
			delay(500);

#ifdef do_cut
			cutLabel();
#else
			delay(500);
#endif

			parameter = "";
			value = "";
			stepperChar.disableOutputs();
			busy = false;
			webProgress = "finished";
			delay(500);
			webProgress = " 0";
			lightChar(0.0f);
			displayQRCode();
			vTaskDelete(processorTaskHandle); // delete task
		}
		else if (parameter == "tag" && value != "")
		{
			// prints a tag

			busy = true;
			String label = value;
			label.toUpperCase();
			writeLabel(label);
			busy = false;
		}
		else if ((parameter == "testalign" || parameter == "testfull") && value != "")
		{
			// tests the settings chosen in the web app

			busy = true;
			interpretSettings();

			displayInitialize();
			displayTest(newAlign, newForce);
			analogWrite(ledFinish, 0);

			bool testFull = false;
			if (parameter == "testfull")
			{
				testFull = true;
#ifdef do_serial
				Serial.print("testing full temporary settings / align: ");
				Serial.print(newAlign);
				Serial.print(" / force: ");
				Serial.println(newForce);
#endif
			}
			else if (parameter == "testalign")
			{
				testFull = false;
#ifdef do_serial
				Serial.print("testing align temporary settings / align: ");
				Serial.print(newAlign);
				Serial.print(" / force: ");
				Serial.println(newForce);
#endif
			}

			testSettings(newAlign, newForce, testFull);

			newAlign = 0;
			newForce = 0;
			value = "";

			webProgress = "finished";
			delay(500);
			webProgress = " 0";

			analogWrite(ledFinish, 0);
			lightChar(0.0f);
			displayQRCode();

			busy = false;
			vTaskDelete(processorTaskHandle); // delete task
		}
		else if (parameter == "save" && value != "")
		{
			// save the settings from the web app
			busy = true;
			interpretSettings();

#ifdef do_serial
			Serial.println("saving settings");
#endif

			displayInitialize();
			displaySettings(newAlign, newForce);
			analogWrite(ledFinish, 0);

			saveSettings(newAlign, newForce);

			newAlign = 0;
			newForce = 0;
			value = "";

			displayReboot();

			webProgress = "finished";
			delay(500);
			webProgress = " 0";

			analogWrite(ledFinish, 0);
			lightChar(0.0f);
			busy = false;
			delay(500);
			ESP.restart(); // instead of deleting task, reboots
		}
	}
}

// --------------------------------------------------------------------------------
// COMMUNICATION ------------------------------------------------------------------

void notFound(AsyncWebServerRequest *request)
{
	request->send(404, "text/plain", "Not found");
}

void initialize()
{
	// Initialize SPIFFS
	if (!SPIFFS.begin())
	{
#ifdef do_serial
		Serial.println("An Error has occurred while mounting SPIFFS");
#endif
		return;
	}

	// route for web app
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/index.html", String(), false); });

	server.on("/&", HTTP_GET, [](AsyncWebServerRequest *request)
			  {
				  int paramsNr = request->params();

				  for (int i = 0; i < paramsNr; i++)
				  {
					  AsyncWebParameter *p = request->getParam(i);
					  parameter = p->name();
					  value = p->value();

					  // if not currently busy, creates a task for a single label print on core 0, that will be killed upon completion
					  // the wifi and web app keeps running on core 1
					  if (!busy)
					  {
						  xTaskCreatePinnedToCore(
							  processor,			// the processor() function that processes the inputs
							  "processorTask",		// name of the task 
							  10000,				// number of words to be allocated to use on task  
							  NULL,					// parameter to be input on the task (can be NULL) 
							  1,					// priority for the task (0 to N) 
							  &processorTaskHandle, // reference to the task (can be NULL) 
							  0);					// core 0
					  }
// 					  else
// 					  {
// #ifdef do_serial
// 						Serial.println("<< DENYING, BUSY >>");
// #endif
// 					  }
				  }
				  request->send(SPIFFS, "/index.html", String(), false); });

	// check printing status
	server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(200, "text/plane", webProgress); });

	// provide stored settings
	server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(200, "text/plane", combinedSettings);  Serial.print("giving combinedSettings: ");  Serial.print(combinedSettings); });

	// ----------------------------------------------------------------------------
	// asset serving --------------------------------------------------------------

	// route to load style.css file
	server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/style.css", "text/css"); });

	// route to load script.js file
	server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/script.js", "text/javascript"); });

	// route to load fonts
	server.on("/fontwhite.ttf", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/fontwhite.ttf", "font"); });

	// route to favicon
	server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/favicon.ico", "image"); });

	// route to icon image
	server.on("/icon.png", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/icon.png", "image"); });

	// route to splash icon
	server.on("/splash.png", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/splash.png", "image"); });

	// route to manifest file
	server.on("/manifest.json", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/manifest.json", "image"); });

	// route to settings image
	server.on("/iso.png", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/iso.png", "image"); });

	// start server
	server.begin();
}

void configModeCallback(AsyncWiFiManager *myWiFiManager)
{
	// captive portal to configure SSID and password

	displayConfig();

#ifdef do_serial
	Serial.println(myWiFiManager->getConfigPortalSSID());
#endif
}

void clearWifiCredentials()
{
	// load the flash-saved configs
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg); // initiate and allocate wifi resources
	delay(2000);		 // wait a bit

	// clear credentials if button is pressed
	if (esp_wifi_restore() != ESP_OK)
	{
#ifdef do_serial
		Serial.println("WiFi is not initialized by esp_wifi_init ");
#endif
	}
	else
	{
#ifdef do_serial
		Serial.println("WiFi Configurations Cleared!");
#endif
	}
	displayReset();
	delay(1500);
	esp_restart();
}

void wifiManager()
{
	// local intialization. once its business is done, there is no need to keep it around
	AsyncWiFiManager wifiManager(&server, &dns);

	bool wifiReset = digitalRead(wifiResetPin);
	if (!wifiReset)
	{
		clearWifiCredentials();
	}

	// set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
	wifiManager.setAPCallback(configModeCallback);

#ifdef do_wifi_debug
	wifiManager.setDebugOutput(true);
#else
	wifiManager.setDebugOutput(false);
#endif

	// fetches ssid and pass and tries to connect
	// if it does not connect it starts an access point with the specified name
	// here  "AutoConnectAP"
	// and goes into a blocking loop awaiting configuration
	if (!wifiManager.autoConnect("E-TKT"))
	{
		// Serial.println("failed to connect and hit timeout");
		// reset and try again, or maybe put it to deep sleep
		ESP.restart();
		delay(1000);
	}

	// TODO: pending idea to access the device from a dns name, but Android doesn't support that yet
	// if (!MDNS.begin("e-tkt"))
	// {
	// 	// Serial.println("Error starting mDNS");
	// 	return;
	// }

	// if you get here you have connected to the WiFi
	displayIP = WiFi.localIP().toString();
	displaySSID = WiFi.SSID();

#ifdef do_wifi_debug
	Serial.print("connected at ");
	Serial.print(displayIP);
	Serial.print(" with password ");
	Serial.println(displaySSID);
#endif

	displayQRCode();

	initialize();
}

// --------------------------------------------------------------------------------
// CORE ---------------------------------------------------------------------------
void setup()
{
#ifdef do_serial
	Serial.begin(115200);
	Serial.println("setup");
#endif

	// turns of the leds
	analogWrite(ledChar, 0);
	analogWrite(ledFinish, 0);
	
	loadSettings();

	// set  display
	displayInitialize();
	displayClear();

	// set pins
	pinMode(sensorPin, INPUT_PULLUP);
	pinMode(wifiResetPin, INPUT);
	pinMode(ledChar, OUTPUT);
	pinMode(ledFinish, OUTPUT);
	pinMode(enableCharStepper, OUTPUT);


	// set feed stepper
	stepperFeed.setMaxSpeed(1000000);
	stepperFeed.setAcceleration(1000); // 6000?

	// set char stepper
	digitalWrite(enableCharStepper, HIGH);
	stepperChar.setMaxSpeed(20000 * MICROSTEP_Char);
	stepperChar.setAcceleration(1000 * MICROSTEP_Char);
	stepsPerChar = (float)stepsPerRevolutionChar / charQuantity;
	stepperChar.setPinsInverted(true, false, true);
	stepperChar.setEnablePin(enableCharStepper);
	setHome(); // initial home for reference
	stepperChar.disableOutputs();

	// set  servo
	myServo.attach(servoPin);
	myServo.write(restAngle);
	delay(100);

	// splash
	displaySplash();

	// start wifi > comment both to avoid entering the main loop
	wifiManager();
	delay(2000); // time for the task to start
}

void loop()
{
	// the loop should be empty

#ifdef do_display_debug
	debugDisplay();
#endif
}