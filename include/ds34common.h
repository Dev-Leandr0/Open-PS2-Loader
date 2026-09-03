#ifndef _DS34COMMON_H_
#define _DS34COMMON_H_
#include <stdint.h>

#define USB_CLASS_WIRELESS_CONTROLLER 0xE0
#define USB_SUBCLASS_RF_CONTROLLER    0x01
#define USB_PROTOCOL_BLUETOOTH_PROG   0x01

#define SONY_VID            0x12ba // Sony
#define DS34_VID            0x054C // Sony Corporation
#define DS3_PID             0x0268 // PS3 Controller
#define DS4_PID             0x05C4 // PS4 Controller
#define DS4_PID_SLIM        0x09CC // PS4 Slim Controller
#define GUITAR_HERO_PS3_PID 0x0100 // PS3 Guitar Hero Guitar
#define ROCK_BAND_PS3_PID   0x0200 // PS3 Rock Band Guitar

#define USB_JOY_VID         0x0079 // USB Joystick
#define USB_JOY_PID         0x0006 // USB Joystick

// NOTE: struct member prefixed with "n" means it's active-low (i.e. value of 0 indicates button is pressed, value 1 is released)
enum DS2ButtonBitNumber {
    DS2BtnBit_Select = 0,
    DS2BtnBit_L3 = 1,
    DS2BtnBit_R3 = 2,
    DS2BtnBit_Start = 3,
    DS2BtnBit_Up = 4,
    DS2BtnBit_Right = 5,
    DS2BtnBit_Down = 6,
    DS2BtnBit_Left = 7,
    DS2BtnBit_L2 = 8,
    DS2BtnBit_R2 = 9,
    DS2BtnBit_L1 = 10,
    DS2BtnBit_R1 = 11,
    DS2BtnBit_Triangle = 12,
    DS2BtnBit_Circle = 13,
    DS2BtnBit_Cross = 14,
    DS2BtnBit_Square = 15,
};


enum DS2Buttons {
    DS2ButtonSelect = (1 << 0),
    DS2ButtonL3 = (1 << 1),
    DS2ButtonR3 = (1 << 2),
    DS2ButtonStart = (1 << 3),
    DS2ButtonUp = (1 << 4),
    DS2ButtonRight = (1 << 5),
    DS2ButtonDown = (1 << 6),
    DS2ButtonLeft = (1 << 7),
    DS2ButtonL2 = (1 << 8),
    DS2ButtonR2 = (1 << 9),
    DS2ButtonL1 = (1 << 10),
    DS2ButtonR1 = (1 << 11),
    DS2ButtonTriangle = (1 << 12),
    DS2ButtonCircle = (1 << 13),
    DS2ButtonCross = (1 << 14),
    DS2ButtonSquare = (1 << 15),
};

struct ds2report
{
    union
    {
        uint16_t nButtonState;
        struct
        {
            uint8_t nButtonStateL; // Main buttons low byte (active-low)
            uint8_t nButtonStateH; // Main buttons high byte (active-low)
        };
        struct
        {
            uint16_t nSelect   : 1;
            uint16_t nL3       : 1;
            uint16_t nR3       : 1;
            uint16_t nStart    : 1;
            uint16_t nUp       : 1;
            uint16_t nRight    : 1;
            uint16_t nDown     : 1;
            uint16_t nLeft     : 1;
            uint16_t nL2       : 1;
            uint16_t nR2       : 1;
            uint16_t nL1       : 1;
            uint16_t nR1       : 1;
            uint16_t nTriangle : 1;
            uint16_t nCircle   : 1;
            uint16_t nCross    : 1;
            uint16_t nSquare   : 1;
        };
    };
    uint8_t RightStickX;
    uint8_t RightStickY;
    uint8_t LeftStickX;
    uint8_t LeftStickY;

    uint8_t PressureRight;
    uint8_t PressureLeft;
    uint8_t PressureUp;
    uint8_t PressureDown;

    uint8_t PressureTriangle;
    uint8_t PressureCircle;
    uint8_t PressureCross;
    uint8_t PressureSquare;

    uint8_t PressureL1;
    uint8_t PressureR1;
    uint8_t PressureL2;
    uint8_t PressureR2;

} __attribute__((packed));

struct ds3report
{
    union
    {
        uint16_t ButtonState;
        struct
        {
            uint8_t ButtonStateL; // Main buttons low byte
            uint8_t ButtonStateH; // Main buttons high byte
        };
        struct
        {
            uint16_t Select   : 1;
            uint16_t L3       : 1;
            uint16_t R3       : 1;
            uint16_t Start    : 1;
            uint16_t Up       : 1;
            uint16_t Right    : 1;
            uint16_t Down     : 1;
            uint16_t Left     : 1;
            uint16_t L2       : 1;
            uint16_t R2       : 1;
            uint16_t L1       : 1;
            uint16_t R1       : 1;
            uint16_t Triangle : 1;
            uint16_t Circle   : 1;
            uint16_t Cross    : 1;
            uint16_t Square   : 1;
        };
    };
    uint8_t PSButton;         // PS button
    uint8_t Reserved1;        // Unknown
    uint8_t LeftStickX;       // left Joystick X axis 0 - 255, 128 is mid
    uint8_t LeftStickY;       // left Joystick Y axis 0 - 255, 128 is mid
    uint8_t RightStickX;      // right Joystick X axis 0 - 255, 128 is mid
    uint8_t RightStickY;      // right Joystick Y axis 0 - 255, 128 is mid
    uint8_t Reserved2[4];     // Unknown
    uint8_t PressureUp;       // digital Pad Up button Pressure 0 - 255
    uint8_t PressureRight;    // digital Pad Right button Pressure 0 - 255
    uint8_t PressureDown;     // digital Pad Down button Pressure 0 - 255
    uint8_t PressureLeft;     // digital Pad Left button Pressure 0 - 255
    uint8_t PressureL2;       // digital Pad L2 button Pressure 0 - 255
    uint8_t PressureR2;       // digital Pad R2 button Pressure 0 - 255
    uint8_t PressureL1;       // digital Pad L1 button Pressure 0 - 255
    uint8_t PressureR1;       // digital Pad R1 button Pressure 0 - 255
    uint8_t PressureTriangle; // digital Pad Triangle button Pressure 0 - 255
    uint8_t PressureCircle;   // digital Pad Circle button Pressure 0 - 255
    uint8_t PressureCross;    // digital Pad Cross button Pressure 0 - 255
    uint8_t PressureSquare;   // digital Pad Square button Pressure 0 - 255
    uint8_t Reserved3[3];     // Unknown
    uint8_t Charge;           // charging status ? 02 = charge, 03 = normal
    uint8_t Power;            // Battery status ? 05=full - 02=dying, 01=just before shutdown, EE=charging
    uint8_t Connection;       // Connection Type ? 14 when operating by bluetooth, 10 when operating by bluetooth with cable plugged in, 16 when bluetooh and rumble
    uint8_t Reserved4[9];     // Unknown
    int16_t AccelX;
    int16_t AccelY;
    int16_t AccelZ;
    int16_t GyroZ;

} __attribute__((packed));

struct ds3guitarreport
{
    union
    {
        uint16_t ButtonState;
        struct
        {
            uint8_t ButtonStateL; // Main buttons low byte
            uint8_t ButtonStateH; // Main buttons high byte
        };
        struct
        {
            uint16_t Blue      : 1;
            uint16_t Green     : 1;
            uint16_t Red       : 1;
            uint16_t Yellow    : 1;
            uint16_t Orange    : 1;
            uint16_t StarPower : 1;
            uint16_t           : 1;
            uint16_t           : 1;
            uint16_t Select    : 1;
            uint16_t Start     : 1;
            uint16_t           : 1;
            uint16_t           : 1;
            uint16_t PSButton  : 1;
            uint16_t           : 1;
            uint16_t           : 1;
            uint16_t           : 1;
        };
    };
    uint8_t Dpad; // hat format, 0x08 is released, 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
    uint8_t : 8;
    uint8_t : 8;
    uint8_t Whammy; // Whammy axis 0 - 255, 128 is mid
    uint8_t : 8;
    uint8_t PressureRightYellow; // digital Pad Right + Yellow button Pressure 0 - 255 (if both are pressed, then they cancel eachother out)
    uint8_t PressureLeft;        // digital Pad Left button Pressure 0 - 255
    uint8_t PressureUpGreen;     // digital Pad Up + Green button Pressure 0 - 255 (if both are pressed, then they cancel eachother out)
    uint8_t PressureDownOrange;  // digital Pad Down + Orange button Pressure 0 - 255 (if both are pressed, then they cancel eachother out)
    uint8_t PressureBlue;        // digital Pad Blue button Pressure 0 - 255
    uint8_t PressureRed;         // digital Pad Red button Pressure 0 - 255
    uint8_t Reserved3[6];        // Unknown
    int16_t AccelX;
    int16_t AccelZ;
    int16_t AccelY;
    int16_t GyroZ;

} __attribute__((packed));

enum DS4DpadDirections {
    DS4DpadDirectionN = 0,
    DS4DpadDirectionNE,
    DS4DpadDirectionE,
    DS4DpadDirectionSE,
    DS4DpadDirectionS,
    DS4DpadDirectionSW,
    DS4DpadDirectionW,
    DS4DpadDirectionNW,
    DS4DpadDirectionReleased,
};

struct ds4report
{
    uint8_t ReportID;
    uint8_t LeftStickX;   // left Joystick X axis 0 - 255, 128 is mid
    uint8_t LeftStickY;   // left Joystick Y axis 0 - 255, 128 is mid
    uint8_t RightStickX;  // right Joystick X axis 0 - 255, 128 is mid
    uint8_t RightStickY;  // right Joystick Y axis 0 - 255, 128 is mid
    uint8_t Dpad     : 4; // hat format, 0x08 is released, 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
    uint8_t Square   : 1;
    uint8_t Cross    : 1;
    uint8_t Circle   : 1;
    uint8_t Triangle : 1;
    uint8_t L1       : 1;
    uint8_t R1       : 1;
    uint8_t L2       : 1;
    uint8_t R2       : 1;
    uint8_t Share    : 1;
    uint8_t Option   : 1;
    uint8_t L3       : 1;
    uint8_t R3       : 1;
    uint8_t PSButton : 1;
    uint8_t TPad     : 1;
    uint8_t Counter1 : 6; // counts up by 1 per report
    uint8_t PressureL2;   // digital Pad L2 button Pressure 0 - 255
    uint8_t PressureR2;   // digital Pad R2 button Pressure 0 - 255
    uint8_t Counter2;
    uint8_t Counter3;
    uint8_t Battery; // battery level from 0x00 to 0xff
    int16_t AccelX;
    int16_t AccelY;
    int16_t AccelZ;
    int16_t GyroZ;
    int16_t GyroY;
    int16_t GyroX;
    uint8_t Reserved1[5];    // Unknown
    uint8_t Power       : 4; // from 0x0 to 0xA - charging, 0xB - charged
    uint8_t Usb_plugged : 1;
    uint8_t Headphones  : 1;
    uint8_t Microphone  : 1;
    uint8_t Padding     : 1;
    uint8_t Reserved2[2];        // Unknown
    uint8_t TPpack;              // number of trackpad packets (0x00 to 0x04)
    uint8_t PackCounter;         // packet counter
    uint8_t Finger1ID      : 7;  // counter
    uint8_t nFinger1Active : 1;  // 0 - active, 1 - unactive
    uint16_t Finger1X      : 12; // finger 1 coordinates resolution 1920x943
    uint16_t Finger1Y      : 12;
    uint8_t Finger2ID      : 7;
    uint8_t nFinger2Active : 1;
    uint16_t Finger2X      : 12; // finger 2 coordinates resolution 1920x943
    uint16_t Finger2Y      : 12;

} __attribute__((packed));

/**
 * Translate DS3 pad data into DS2 pad data.
 * @param in DS3 report
 * @param out DS2 report
 * @param pressure_emu set to 1 to extrapolate digital buttons into button pressure
 * NOTE: if set to 0, ds3report must be large enough for that data to be read!
 */
void translate_pad_ds3(const struct ds3report *in, struct ds2report *out, uint8_t pressure_emu);

/**
 * Translate PS3 Guitar pad data into DS2 Guitar pad data.
 * @param in PS3 Guitar report
 * @param out PS2 Guitar report
 * @param guitar_hero_format set to 1 if this is a guitar hero guitar, set to 0 if this is a rock band guitar
 */
void translate_pad_guitar(const struct ds3guitarreport *in, struct ds2report *out, uint8_t guitar_hero_format);

/**
 * Translate DS3 pad data into DS2 pad data.
 * @param in DS4 report
 * @param out DS2 report
 * @param have_touchpad set to 1 if input report has touchpad data
 * NOTE: if set to 1, ds4report must be large enough for that data to be read!
 */
void translate_pad_ds4(const struct ds4report *in, struct ds2report *out, uint8_t have_touchpad);

/**
 * Translate a generic USB joystick report into DS2 pad data.
 * @param report 8-byte joystick report
 * @param out DS2 report
 */
static inline void translate_pad_joystick(const uint8_t *report, struct ds2report *out)
{
    static const uint8_t dpad_mapping[] = {
        (DS2ButtonUp),
        (DS2ButtonUp | DS2ButtonRight),
        (DS2ButtonRight),
        (DS2ButtonDown | DS2ButtonRight),
        (DS2ButtonDown),
        (DS2ButtonDown | DS2ButtonLeft),
        (DS2ButtonLeft),
        (DS2ButtonUp | DS2ButtonLeft),
        0,
    };

    uint8_t hat = report[5] & 0x0F,  // 15 = neutral, 0 = up, 2 = right, 4 = down, 6 = left
        l1 = report[6] & 0x01,
        r1 = (report[6] >> 1) & 0x01,
        l2 = (report[6] >> 2) & 0x01,
        r2 = (report[6] >> 3) & 0x01,
        select = (report[6] >> 4) & 0x01,
        start = (report[6] >> 5) & 0x01,
        l3 = (report[6] >> 6) & 0x01,
        r3 = (report[6] >> 7) & 0x01,
        triangle = (report[5] >> 4) & 0x01,
        circle = (report[5] >> 5) & 0x01,
        cross = (report[5] >> 6) & 0x01,
        square = (report[5] >> 7) & 0x01,
        dpad = (hat == 15) ? 8 : hat;

    out->LeftStickX = report[0];
    out->LeftStickY = report[1];
    out->RightStickX = report[3];
    out->RightStickY = report[4];

    out->nButtonStateL = ~(select | l3 << 1 | r3 << 2 | start << 3 | dpad_mapping[dpad]);
    out->nButtonStateH = ~(l2 | r2 << 1 | l1 << 2 | r1 << 3 | triangle << 4 | circle << 5 | cross << 6 | square << 7);

    out->PressureRight = out->nRight ? 0 : 255;
    out->PressureLeft = out->nLeft ? 0 : 255;
    out->PressureUp = out->nUp ? 0 : 255;
    out->PressureDown = out->nDown ? 0 : 255;

    out->PressureTriangle = triangle * 255;
    out->PressureCircle = circle * 255;
    out->PressureCross = cross * 255;
    out->PressureSquare = square * 255;

    out->PressureL1 = l1 * 255;
    out->PressureR1 = r1 * 255;
    out->PressureL2 = l2 * 255;
    out->PressureR2 = r2 * 255;
}

#endif
