#ifndef _HIDPAD_H_
#define _HIDPAD_H_

#include "ds34common.h"

#define HIDP_HAT_RELEASED 0xFF

// Buttons are reported as a semantic bitmask (HIDP_BTN_*) independent of the
// physical device layout. See hidp_btn for the bit position of each button.
enum hidp_btn {
    HIDP_BTN_SQUARE = 0,
    HIDP_BTN_CROSS,
    HIDP_BTN_CIRCLE,
    HIDP_BTN_TRIANGLE,
    HIDP_BTN_L1,
    HIDP_BTN_R1,
    HIDP_BTN_L2,
    HIDP_BTN_R2,
    HIDP_BTN_SELECT,
    HIDP_BTN_START,
    HIDP_BTN_L3,
    HIDP_BTN_R3,
    HIDP_BTN_COUNT
};

// Normalized gamepad state produced by the device decoders. Axes stay in the
// raw 0..255 range with 128 as center. hat is 0..7 for the eight directions
// and HIDP_HAT_RELEASED when released.
struct hid_pad_report {
    u8 lx;   // left stick X axis 0..255, 128 is mid
    u8 ly;   // left stick Y axis 0..255, 128 is mid
    u8 rx;   // right stick X axis 0..255, 128 is mid
    u8 ry;   // right stick Y axis 0..255, 128 is mid
    u8 hat;  // 0..7 direction, HIDP_HAT_RELEASED when released
    u16 buttons;
};

// Device decoder returning 0 for a structurally valid report and non-zero
// otherwise. It must never output a hat value outside 0..7 / HIDP_HAT_RELEASED.
typedef int (*hid_pad_decode_fn)(const u8 *report, u8 len, struct hid_pad_report *out);

// 0079:0006 USB HID input report, 8 bytes:
//   byte 0: left X, byte 1: left Y, byte 3: right X, byte 4: right Y
//   byte 5 low nibble: hat (0..7 direction, 8..15 released)
//   byte 5 high nibble: triangle/circle/cross/square
//   byte 6: L1 R1 L2 R2 Select Start L3 R3
//   bytes 2, 7: unused
// Returns 0 on success, non-zero for a structurally invalid report.
static inline int hid_pad_decode_0079_0006(
    const u8 *report,
    u8 len,
    struct hid_pad_report *out)
{
    u8 hat;

    if (len < 8)
        return -1;

    out->lx = report[0];
    out->ly = report[1];
    out->rx = report[3];
    out->ry = report[4];

    hat = report[5] & 0x0F;
    out->hat = (hat < 8) ? hat : HIDP_HAT_RELEASED;

    out->buttons = 0;
    if (report[5] & 0x10)
        out->buttons |= (1 << HIDP_BTN_TRIANGLE);
    if (report[5] & 0x20)
        out->buttons |= (1 << HIDP_BTN_CIRCLE);
    if (report[5] & 0x40)
        out->buttons |= (1 << HIDP_BTN_CROSS);
    if (report[5] & 0x80)
        out->buttons |= (1 << HIDP_BTN_SQUARE);

    if (report[6] & 0x01)
        out->buttons |= (1 << HIDP_BTN_L1);
    if (report[6] & 0x02)
        out->buttons |= (1 << HIDP_BTN_R1);
    if (report[6] & 0x04)
        out->buttons |= (1 << HIDP_BTN_L2);
    if (report[6] & 0x08)
        out->buttons |= (1 << HIDP_BTN_R2);
    if (report[6] & 0x10)
        out->buttons |= (1 << HIDP_BTN_SELECT);
    if (report[6] & 0x20)
        out->buttons |= (1 << HIDP_BTN_START);
    if (report[6] & 0x40)
        out->buttons |= (1 << HIDP_BTN_L3);
    if (report[6] & 0x80)
        out->buttons |= (1 << HIDP_BTN_R3);

    return 0;
}

// Device capabilities, reserved for future gamepads (e.g. rumble, LED).
// Not enabled for the 0079:0006 joystick in this phase.
#define HIDP_CAP_RUMBLE (1 << 0)
#define HIDP_CAP_LED    (1 << 1)

struct hid_pad_device {
    u16 vid;
    u16 pid;
    u8 report_len;
    u8 caps;
    hid_pad_decode_fn decode;
};

static const struct hid_pad_device hid_pad_devices[] = {
    {
        .vid = 0x0079,
        .pid = 0x0006,
        .report_len = 8,
        .caps = 0,
        .decode = hid_pad_decode_0079_0006,
    },
};

static inline const struct hid_pad_device *hid_pad_find(u16 vid, u16 pid)
{
    unsigned int i;

    for (i = 0; i < sizeof(hid_pad_devices) / sizeof(hid_pad_devices[0]); i++) {
        if (hid_pad_devices[i].vid == vid && hid_pad_devices[i].pid == pid)
            return &hid_pad_devices[i];
    }

    return NULL;
}

// Hat direction to DS2 D-pad button masks (active bits, see DS2Button*).
// Indexed exclusively with 0..7; never with HIDP_HAT_RELEASED.
static const u8 ds2_dpad_hat[8] = {
    DS2ButtonUp,                     // 0 = North
    DS2ButtonUp | DS2ButtonRight,    // 1 = North-East
    DS2ButtonRight,                  // 2 = East
    DS2ButtonDown | DS2ButtonRight,  // 3 = South-East
    DS2ButtonDown,                   // 4 = South
    DS2ButtonDown | DS2ButtonLeft,   // 5 = South-West
    DS2ButtonLeft,                   // 6 = West
    DS2ButtonUp | DS2ButtonLeft,     // 7 = North-West
};

// Generic HID to DS2 translation. It only knows struct hid_pad_report and the
// DS2 pad contract; all device specific knowledge lives in the decoders.
// hat is indexed into ds2_dpad_hat only for values below 8, keeping the hat
// look-up free of out-of-bounds accesses.
static inline void translate_pad_hid(
    const struct hid_pad_report *in,
    struct ds2report *out)
{
    u16 pressed = 0;

    if (in->buttons & (1 << HIDP_BTN_SQUARE))
        pressed |= (1 << DS2BtnBit_Square);
    if (in->buttons & (1 << HIDP_BTN_CROSS))
        pressed |= (1 << DS2BtnBit_Cross);
    if (in->buttons & (1 << HIDP_BTN_CIRCLE))
        pressed |= (1 << DS2BtnBit_Circle);
    if (in->buttons & (1 << HIDP_BTN_TRIANGLE))
        pressed |= (1 << DS2BtnBit_Triangle);
    if (in->buttons & (1 << HIDP_BTN_L1))
        pressed |= (1 << DS2BtnBit_L1);
    if (in->buttons & (1 << HIDP_BTN_R1))
        pressed |= (1 << DS2BtnBit_R1);
    if (in->buttons & (1 << HIDP_BTN_L2))
        pressed |= (1 << DS2BtnBit_L2);
    if (in->buttons & (1 << HIDP_BTN_R2))
        pressed |= (1 << DS2BtnBit_R2);
    if (in->buttons & (1 << HIDP_BTN_SELECT))
        pressed |= (1 << DS2BtnBit_Select);
    if (in->buttons & (1 << HIDP_BTN_START))
        pressed |= (1 << DS2BtnBit_Start);
    if (in->buttons & (1 << HIDP_BTN_L3))
        pressed |= (1 << DS2BtnBit_L3);
    if (in->buttons & (1 << HIDP_BTN_R3))
        pressed |= (1 << DS2BtnBit_R3);

    if (in->hat < 8)
        pressed |= ds2_dpad_hat[in->hat];

    out->LeftStickX = in->lx;
    out->LeftStickY = in->ly;
    out->RightStickX = in->rx;
    out->RightStickY = in->ry;

    out->nButtonState = (u16)~pressed;

    out->PressureUp = (pressed & (1 << DS2BtnBit_Up)) ? 0 : 255;
    out->PressureRight = (pressed & (1 << DS2BtnBit_Right)) ? 0 : 255;
    out->PressureDown = (pressed & (1 << DS2BtnBit_Down)) ? 0 : 255;
    out->PressureLeft = (pressed & (1 << DS2BtnBit_Left)) ? 0 : 255;
    out->PressureTriangle = (pressed & (1 << DS2BtnBit_Triangle)) ? 255 : 0;
    out->PressureCircle = (pressed & (1 << DS2BtnBit_Circle)) ? 255 : 0;
    out->PressureCross = (pressed & (1 << DS2BtnBit_Cross)) ? 255 : 0;
    out->PressureSquare = (pressed & (1 << DS2BtnBit_Square)) ? 255 : 0;
    out->PressureL1 = (pressed & (1 << DS2BtnBit_L1)) ? 255 : 0;
    out->PressureR1 = (pressed & (1 << DS2BtnBit_R1)) ? 255 : 0;
    out->PressureL2 = (pressed & (1 << DS2BtnBit_L2)) ? 255 : 0;
    out->PressureR2 = (pressed & (1 << DS2BtnBit_R2)) ? 255 : 0;
}

#endif