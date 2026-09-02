#include "ds34common.h"

#include "loadcore.h"
#include "sysclib.h"

void translate_pad_guitar(const struct ds3guitarreport *in, struct ds2report *out, uint8_t guitar_hero_format)
{
    out->RightStickX = 0x7F;
    out->RightStickY = 0x7F;
    out->LeftStickX = 0x7F;
    out->LeftStickY = -(in->Whammy);

    static const u8 dpad_mapping[] = {
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

    u8 dpad = in->Dpad > DS4DpadDirectionReleased ? DS4DpadDirectionReleased : in->Dpad;

    out->nButtonStateL = ~(in->Select | in->Start << 3 | dpad_mapping[dpad]);

    out->nLeft = 0;
    out->nL2 = 1;

    if (guitar_hero_format) {
        // GH PS3 Guitars swap Yellow and Blue
        // Interestingly, it is only GH PS3 Guitars that do this, all the other instruments including GH Drums don't have this swapped.
        out->nButtonStateH = ~(in->Green << 1 | in->Blue << 4 | in->Red << 5 | in->Yellow << 6 | in->Orange << 7);
        if (in->AccelX > 512 || in->AccelX < 432) {
            out->nL2 = 0;
        }
    } else {
        out->nButtonStateH = ~(in->StarPower | in->Green << 1 | in->Yellow << 4 | in->Red << 5 | in->Blue << 6 | in->Orange << 7);
    }
}

void translate_pad_ds3(const struct ds3report *in, struct ds2report *out, u8 pressure_emu)
{
    out->nButtonStateL = ~in->ButtonStateL;
    out->nButtonStateH = ~in->ButtonStateH;

    out->RightStickX = in->RightStickX;
    out->RightStickY = in->RightStickY;
    out->LeftStickX = in->LeftStickX;
    out->LeftStickY = in->LeftStickY;

    if (pressure_emu) { // needs emulating pressure buttons
        out->PressureRight = in->Right * 255;
        out->PressureLeft = in->Left * 255;
        out->PressureUp = in->Up * 255;
        out->PressureDown = in->Down * 255;

        out->PressureTriangle = in->Triangle * 255;
        out->PressureCircle = in->Circle * 255;
        out->PressureCross = in->Cross * 255;
        out->PressureSquare = in->Square * 255;

        out->PressureL1 = in->L1 * 255;
        out->PressureR1 = in->R1 * 255;
        out->PressureL2 = in->L2 * 255;
        out->PressureR2 = in->R2 * 255;
    } else {
        out->PressureRight = in->PressureRight;
        out->PressureLeft = in->PressureLeft;
        out->PressureUp = in->PressureUp;
        out->PressureDown = in->PressureDown;

        out->PressureTriangle = in->PressureTriangle;
        out->PressureCircle = in->PressureCircle;
        out->PressureCross = in->PressureCross;
        out->PressureSquare = in->PressureSquare;

        out->PressureL1 = in->PressureL1;
        out->PressureR1 = in->PressureR1;
        out->PressureL2 = in->PressureL2;
        out->PressureR2 = in->PressureR2;
    }
}

void translate_pad_ds4(const struct ds4report *in, struct ds2report *out, u8 have_touchpad)
{
    static const u8 dpad_mapping[] = {
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

    u8
        dpad = in->Dpad > DS4DpadDirectionReleased ? DS4DpadDirectionReleased : in->Dpad, // Just in case an unexpected value appears
        select = in->Share,
        start = in->Option;

    if (have_touchpad && in->TPad) {
        if (!in->nFinger1Active) {
            if (in->Finger1X < 960)
                select = 1;
            else
                start = 1;
        }

        if (!in->nFinger2Active) {
            if (in->Finger2X < 960)
                select = 1;
            else
                start = 1;
        }
    }

    out->nButtonStateL = ~(select | in->L3 << 1 | in->R3 << 2 | start << 3 | dpad_mapping[dpad]);
    out->nButtonStateH = ~(in->L2 | in->R2 << 1 | in->L1 << 2 | in->R1 << 3 | in->Triangle << 4 | in->Circle << 5 | in->Cross << 6 | in->Square << 7);

    out->RightStickX = in->RightStickX;
    out->RightStickY = in->RightStickY;
    out->LeftStickX = in->LeftStickX;
    out->LeftStickY = in->LeftStickY;

    out->PressureRight = out->nRight ? 0 : 255;
    out->PressureLeft = out->nLeft ? 0 : 255;
    out->PressureUp = out->nUp ? 0 : 255;
    out->PressureDown = out->nDown ? 0 : 255;

    out->PressureTriangle = in->Triangle * 255;
    out->PressureCircle = in->Circle * 255;
    out->PressureCross = in->Cross * 255;
    out->PressureSquare = in->Square * 255;

    out->PressureL1 = in->L1 * 255;
    out->PressureR1 = in->R1 * 255;
    out->PressureL2 = in->PressureL2;
    out->PressureR2 = in->PressureR2;
}

void translate_pad_joystick(const u8 *report, struct ds2report *out)
{
    static const u8 dpad_mapping[] = {
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

    u8 hat = report[5] & 0x0F,  // 15 = neutral, 0 = up, 2 = right, 4 = down, 6 = left
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
