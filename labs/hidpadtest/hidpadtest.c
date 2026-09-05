// Host-side test harness for the OPL HID gamepad layer (include/hidpad.h).
//
// It exercises hid_pad_find(), the 0079:0006 decoder and translate_pad_hid()
// with plain C compiled for the host, without any PS2 hardware. Build and run
// with the accompanying Makefile (needs a C99 compiler, e.g. gcc):
//
//     make test
//
// Exit code is 0 only if every check passes.

typedef unsigned char u8;
typedef unsigned short u16;

#include <stdio.h>
#include <string.h>

#include "hidpad.h"

static int g_checks;
static int g_failures;

#define CHECK(expr) \
    do { \
        g_checks++; \
        if (!(expr)) { \
            g_failures++; \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        } \
    } while (0)

#define BIT(n) (1u << (n))

// Build a raw 0079:0006 report. face is the byte 5 high nibble (0x10..0x80),
// sys is byte 6 (bit0..7 = L1 R1 L2 R2 Select Start L3 R3), hat is the byte 5
// low nibble (0..7 direction, 8..15 released).
static void make_raw(
    u8 *raw, u8 face, u8 sys, u8 hat, u8 lx, u8 ly, u8 rx, u8 ry)
{
    raw[0] = lx;
    raw[1] = ly;
    raw[2] = 0;
    raw[3] = rx;
    raw[4] = ry;
    raw[5] = (u8)((face & 0xF0) | (hat & 0x0F));
    raw[6] = sys;
    raw[7] = 0;
}

static void decode_report(struct hid_pad_report *r, const u8 *raw)
{
    CHECK(hid_pad_decode_0079_0006(raw, 8, r) == 0);
}

// 1. PROFILE LOOKUP
static void test_profile(void)
{
    const struct hid_pad_device *joy = hid_pad_find(0x0079, 0x0006);
    const struct hid_pad_device *none = hid_pad_find(0xDEAD, 0xBEEF);

    CHECK(joy != NULL);
    CHECK(joy->vid == 0x0079);
    CHECK(joy->pid == 0x0006);
    CHECK(joy->report_len == 8);
    CHECK(joy->caps == 0);
    CHECK(joy->decode != NULL);
    CHECK(joy->decode == hid_pad_decode_0079_0006);
    CHECK(none == NULL);
}

// 2. NEUTRAL REPORT
static void test_neutral(void)
{
    const u8 raw[8] = {128, 128, 0, 128, 128, 0x0F, 0x00, 0x00};
    struct hid_pad_report r;
    struct ds2report d;

    decode_report(&r, raw);
    CHECK(r.lx == 128);
    CHECK(r.ly == 128);
    CHECK(r.rx == 128);
    CHECK(r.ry == 128);
    CHECK(r.hat == HIDP_HAT_RELEASED);
    CHECK(r.buttons == 0);

    translate_pad_hid(&r, &d);
    CHECK(d.LeftStickX == 128);
    CHECK(d.LeftStickY == 128);
    CHECK(d.RightStickX == 128);
    CHECK(d.RightStickY == 128);
    CHECK(d.nButtonState == 0xFFFF);
    CHECK(d.PressureUp == 255);
    CHECK(d.PressureRight == 255);
    CHECK(d.PressureDown == 255);
    CHECK(d.PressureLeft == 255);
    CHECK(d.PressureTriangle == 0);
    CHECK(d.PressureCircle == 0);
    CHECK(d.PressureCross == 0);
    CHECK(d.PressureSquare == 0);
    CHECK(d.PressureL1 == 0);
    CHECK(d.PressureR1 == 0);
    CHECK(d.PressureL2 == 0);
    CHECK(d.PressureR2 == 0);
}

// 3. STICKS: 0, 128 and 255 on every axis, without transformation.
static void test_sticks(void)
{
    const u8 values[3] = {0, 128, 255};
    /* offsets in the raw report: 0=lx, 1=ly, 3=rx, 4=ry */
    static const int axis_offsets[4] = {0, 1, 3, 4};
    int v, axis;
    u8 raw[8];
    struct hid_pad_report r;
    struct ds2report d;

    for (v = 0; v < 3; v++) {
        make_raw(raw, 0x00, 0x00, 0x0F, values[v], values[v], values[v], values[v]);
        decode_report(&r, raw);
        CHECK(r.lx == values[v]);
        CHECK(r.ly == values[v]);
        CHECK(r.rx == values[v]);
        CHECK(r.ry == values[v]);
        translate_pad_hid(&r, &d);
        CHECK(d.LeftStickX == values[v]);
        CHECK(d.LeftStickY == values[v]);
        CHECK(d.RightStickX == values[v]);
        CHECK(d.RightStickY == values[v]);
    }

    /* one axis at a time: the others must stay at 128 */
    for (v = 0; v < 3; v++) {
        for (axis = 0; axis < 4; axis++) {
            make_raw(raw, 0x00, 0x00, 0x0F, 128, 128, 128, 128);
            raw[axis_offsets[axis]] = values[v];
            decode_report(&r, raw);
            CHECK(r.lx == ((axis_offsets[axis] == 0) ? values[v] : 128));
            CHECK(r.ly == ((axis_offsets[axis] == 1) ? values[v] : 128));
            CHECK(r.rx == ((axis_offsets[axis] == 3) ? values[v] : 128));
            CHECK(r.ry == ((axis_offsets[axis] == 4) ? values[v] : 128));
        }
    }
}

// 4. FACE BUTTONS, one at a time and all at once.
static void test_face_buttons(void)
{
    int i;
    static const struct {
        int hidp_bit;
        u8 raw_mask;
        int ds2_bit;
    } faces[4] = {
        {HIDP_BTN_SQUARE, 0x80, DS2BtnBit_Square},
        {HIDP_BTN_CROSS, 0x40, DS2BtnBit_Cross},
        {HIDP_BTN_CIRCLE, 0x20, DS2BtnBit_Circle},
        {HIDP_BTN_TRIANGLE, 0x10, DS2BtnBit_Triangle},
    };
    u8 raw[8];
    struct hid_pad_report r;
    struct ds2report d;

    for (i = 0; i < 4; i++) {
        make_raw(raw, faces[i].raw_mask, 0x00, 0x0F, 128, 128, 128, 128);
        decode_report(&r, raw);
        CHECK(r.hat == HIDP_HAT_RELEASED);
        CHECK(r.buttons == BIT(faces[i].hidp_bit));

        translate_pad_hid(&r, &d);
        CHECK((d.nButtonState & BIT(faces[i].ds2_bit)) == 0);
        CHECK((d.nButtonState & BIT(DS2BtnBit_Up)) != 0);   /* D-pad released */
        CHECK((d.nButtonState & BIT(DS2BtnBit_L1)) != 0);   /* shoulder released */
        CHECK(d.PressureTriangle == (faces[i].ds2_bit == DS2BtnBit_Triangle ? 255 : 0));
        CHECK(d.PressureCircle == (faces[i].ds2_bit == DS2BtnBit_Circle ? 255 : 0));
        CHECK(d.PressureCross == (faces[i].ds2_bit == DS2BtnBit_Cross ? 255 : 0));
        CHECK(d.PressureSquare == (faces[i].ds2_bit == DS2BtnBit_Square ? 255 : 0));
    }

    /* all face buttons at once, D-pad released (hat = 0x0F) */
    make_raw(raw, 0xF0, 0x00, 0x0F, 128, 128, 128, 128);
    decode_report(&r, raw);
    CHECK(r.hat == HIDP_HAT_RELEASED);
    CHECK(r.buttons == (BIT(HIDP_BTN_SQUARE) | BIT(HIDP_BTN_CROSS) | BIT(HIDP_BTN_CIRCLE) | BIT(HIDP_BTN_TRIANGLE)));

    translate_pad_hid(&r, &d);
    CHECK(d.nButtonState == 0x0FFF);                       /* only face bits pressed */
    CHECK(d.PressureTriangle == 255);
    CHECK(d.PressureCircle == 255);
    CHECK(d.PressureCross == 255);
    CHECK(d.PressureSquare == 255);
    CHECK(d.PressureUp == 255);
    CHECK(d.PressureRight == 255);
    CHECK(d.PressureDown == 255);
    CHECK(d.PressureLeft == 255);
}

// 5. SHOULDERS AND SYSTEM BUTTONS, one at a time and all at once.
static void test_shoulders_system(void)
{
    int i;
    static const struct {
        int hidp_bit;
        u8 raw_mask;
        int ds2_bit;
        int has_pressure;
    } sys[8] = {
        {HIDP_BTN_L1, 0x01, DS2BtnBit_L1, 1},
        {HIDP_BTN_R1, 0x02, DS2BtnBit_R1, 1},
        {HIDP_BTN_L2, 0x04, DS2BtnBit_L2, 1},
        {HIDP_BTN_R2, 0x08, DS2BtnBit_R2, 1},
        {HIDP_BTN_SELECT, 0x10, DS2BtnBit_Select, 0},
        {HIDP_BTN_START, 0x20, DS2BtnBit_Start, 0},
        {HIDP_BTN_L3, 0x40, DS2BtnBit_L3, 0},
        {HIDP_BTN_R3, 0x80, DS2BtnBit_R3, 0},
    };
    u8 raw[8];
    struct hid_pad_report r;
    struct ds2report d;

    for (i = 0; i < 8; i++) {
        make_raw(raw, 0x00, sys[i].raw_mask, 0x0F, 128, 128, 128, 128);
        decode_report(&r, raw);
        CHECK(r.hat == HIDP_HAT_RELEASED);
        CHECK(r.buttons == BIT(sys[i].hidp_bit));

        translate_pad_hid(&r, &d);
        CHECK((d.nButtonState & BIT(sys[i].ds2_bit)) == 0);
        CHECK((d.nButtonState & BIT(DS2BtnBit_Up)) != 0);   /* D-pad released */
        CHECK((d.nButtonState & BIT(DS2BtnBit_Square)) != 0); /* face released */
        CHECK(d.PressureL1 == (sys[i].ds2_bit == DS2BtnBit_L1 ? 255 : 0));
        CHECK(d.PressureR1 == (sys[i].ds2_bit == DS2BtnBit_R1 ? 255 : 0));
        CHECK(d.PressureL2 == (sys[i].ds2_bit == DS2BtnBit_L2 ? 255 : 0));
        CHECK(d.PressureR2 == (sys[i].ds2_bit == DS2BtnBit_R2 ? 255 : 0));
    }

    /* all shoulders/system at once, D-pad and face released */
    make_raw(raw, 0x00, 0xFF, 0x0F, 128, 128, 128, 128);
    decode_report(&r, raw);
    CHECK(r.buttons == (BIT(HIDP_BTN_L1) | BIT(HIDP_BTN_R1) | BIT(HIDP_BTN_L2) | BIT(HIDP_BTN_R2) |
                        BIT(HIDP_BTN_SELECT) | BIT(HIDP_BTN_START) | BIT(HIDP_BTN_L3) | BIT(HIDP_BTN_R3)));

    translate_pad_hid(&r, &d);
    CHECK(d.nButtonStateL == 0xF0);   /* Select/L3/R3/Start pressed, D-pad released */
    CHECK(d.nButtonStateH == 0xF0);   /* L2/R2/L1/R1 pressed, face released */
    CHECK(d.PressureL1 == 255);
    CHECK(d.PressureR1 == 255);
    CHECK(d.PressureL2 == 255);
    CHECK(d.PressureR2 == 255);
    CHECK(d.PressureTriangle == 0);
    CHECK(d.PressureUp == 255);
}

// 6. HAT: directions 0..7, all of 8..15 handled as released without any
// access outside ds2_dpad_hat[8].
static void test_hat_directions(void)
{
    int i;
    u8 raw[8];
    struct hid_pad_report r;
    struct ds2report d;

    for (i = 0; i < 8; i++) {
        make_raw(raw, 0x00, 0x00, (u8)i, 128, 128, 128, 128);
        decode_report(&r, raw);
        CHECK(r.hat == i);
        CHECK(r.buttons == 0);

        translate_pad_hid(&r, &d); {
            u8 expect = (u8)(~ds2_dpad_hat[i] & 0xFF);
            CHECK(d.nButtonStateL == expect);
        }
        CHECK(d.nButtonStateH == 0xFF);
    }

    make_raw(raw, 0x00, 0x00, 0x0F, 128, 128, 128, 128);
    decode_report(&r, raw);
    translate_pad_hid(&r, &d);
    CHECK(d.PressureUp == 255);
    CHECK(d.PressureDown == 255);
}

static void test_hat_released(void)
{
    int i;
    u8 raw[8];
    struct hid_pad_report r;
    struct ds2report d;

    for (i = 8; i < 16; i++) {
        make_raw(raw, 0x00, 0x00, (u8)i, 128, 128, 128, 128);
        decode_report(&r, raw);
        CHECK(r.hat == HIDP_HAT_RELEASED);
        CHECK(r.buttons == 0);

        translate_pad_hid(&r, &d);
        CHECK(d.nButtonStateL == 0xFF);
        CHECK(d.nButtonStateH == 0xFF);
        CHECK(d.PressureUp == 255);
        CHECK(d.PressureRight == 255);
        CHECK(d.PressureDown == 255);
        CHECK(d.PressureLeft == 255);
    }
}

// 7. INVALID / SHORT REPORTS must be rejected without reading past the
// provided length.
static void test_short_reports(void)
{
    u8 len;
    const u8 raw[8] = {0x00};
    struct hid_pad_report r;

    for (len = 0; len < 8; len++) {
        memset(&r, 0xAA, sizeof(r));
        CHECK(hid_pad_decode_0079_0006(raw, len, &r) != 0);
        /* the decoder must not have touched the output on failure */
        CHECK(r.lx == 0xAA && r.buttons == 0xAAAA);
    }
}

// A valid report must always decode to the same, stable result.
static void test_valid_stability(void)
{
    const u8 raw[8] = {10, 20, 34, 30, 40, 0xF0, 0xFF, 90};
    struct hid_pad_report a;
    struct hid_pad_report b;

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));

    CHECK(hid_pad_decode_0079_0006(raw, 8, &a) == 0);
    CHECK(hid_pad_decode_0079_0006(raw, 8, &b) == 0);
    CHECK(memcmp(&a, &b, sizeof(a)) == 0);
    CHECK(a.lx == 10 && a.ly == 20 && a.rx == 30 && a.ry == 40);
    CHECK(a.hat == 0);
    CHECK(a.buttons == 0x0FFF);
}

// 8. DS2 TRANSLATION semantics.
static void test_translate_buttons(void)
{
    const u8 raw[8] = {128, 128, 0, 128, 128, 0x10, 0x01, 0x00};
    struct hid_pad_report r;
    struct ds2report d;

    decode_report(&r, raw);
    CHECK(r.hat == 0);   /* byte 5 low nibble 0 -> North */
    CHECK((r.buttons & BIT(HIDP_BTN_TRIANGLE)) != 0);
    CHECK((r.buttons & BIT(HIDP_BTN_L1)) != 0);
    CHECK((r.buttons & BIT(HIDP_BTN_CIRCLE)) == 0);

    translate_pad_hid(&r, &d);
    CHECK(d.nTriangle == 0);
    CHECK(d.nL1 == 0);
    CHECK(d.nCircle == 1);
    CHECK(d.nUp == 0);         /* hat North -> D-pad Up pressed */
    CHECK(d.PressureTriangle == 255);
    CHECK(d.PressureL1 == 255);
    CHECK(d.PressureCircle == 0);
    CHECK(d.PressureUp == 0);
}

static void test_translate_no_residue(void)
{
    u8 raw[8];
    struct hid_pad_report r;
    struct ds2report d;

    /* everything pressed first, then a fully neutral report into the same struct */
    make_raw(raw, 0xF0, 0xFF, 0x00, 0, 255, 0, 255);
    decode_report(&r, raw);
    translate_pad_hid(&r, &d);

    make_raw(raw, 0x00, 0x00, 0x0F, 128, 128, 128, 128);
    decode_report(&r, raw);
    translate_pad_hid(&r, &d);

    CHECK(d.nButtonState == 0xFFFF);
    CHECK(d.LeftStickX == 128);
    CHECK(d.LeftStickY == 128);
    CHECK(d.RightStickX == 128);
    CHECK(d.RightStickY == 128);
    CHECK(d.PressureUp == 255 && d.PressureRight == 255);
    CHECK(d.PressureDown == 255 && d.PressureLeft == 255);
    CHECK(d.PressureTriangle == 0 && d.PressureCircle == 0);
    CHECK(d.PressureCross == 0 && d.PressureSquare == 0);
    CHECK(d.PressureL1 == 0 && d.PressureR1 == 0);
    CHECK(d.PressureL2 == 0 && d.PressureR2 == 0);
}

// 9. COMBINED REPORTS must keep every field independent.
static void test_combined(void)
{
    u8 raw[8];
    struct hid_pad_report r;
    struct ds2report d;

    /* stick + D-pad */
    make_raw(raw, 0x00, 0x00, 0x01, 0, 128, 128, 128);
    decode_report(&r, raw);
    CHECK(r.lx == 0);
    CHECK(r.hat == 1);
    CHECK(r.buttons == 0);
    translate_pad_hid(&r, &d);
    CHECK(d.LeftStickX == 0);
    CHECK(d.nButtonStateL == (u8)~(DS2ButtonUp | DS2ButtonRight));

    /* stick + buttons */
    make_raw(raw, 0x00, 0x10, 0x0F, 255, 128, 128, 128);
    decode_report(&r, raw);
    CHECK(r.lx == 255);
    CHECK((r.buttons & BIT(HIDP_BTN_SELECT)) != 0);
    translate_pad_hid(&r, &d);
    CHECK(d.LeftStickX == 255);
    CHECK((d.nButtonState & BIT(DS2BtnBit_Select)) == 0);

    /* D-pad + buttons */
    make_raw(raw, 0x20, 0x00, 0x04, 128, 128, 128, 128);
    decode_report(&r, raw);
    CHECK(r.hat == 4);
    CHECK((r.buttons & BIT(HIDP_BTN_CIRCLE)) != 0);
    translate_pad_hid(&r, &d);
    CHECK((d.nButtonState & BIT(DS2BtnBit_Down)) == 0);
    CHECK(d.PressureCircle == 255);
    CHECK(d.PressureDown == 0);

    /* all buttons + all sticks at their extrema */
    make_raw(raw, 0xF0, 0xFF, 0x0F, 0, 255, 1, 254);
    decode_report(&r, raw);
    CHECK(r.lx == 0 && r.ly == 255 && r.rx == 1 && r.ry == 254);
    CHECK(r.hat == HIDP_HAT_RELEASED);
    CHECK(r.buttons == 0x0FFF);
    translate_pad_hid(&r, &d);
    CHECK(d.LeftStickX == 0 && d.LeftStickY == 255);
    CHECK(d.RightStickX == 1 && d.RightStickY == 254);
    CHECK(d.nButtonStateL == 0xF0);   /* system buttons pressed, D-pad released */
    CHECK(d.nButtonStateH == 0x00);   /* shoulders and face pressed */
    CHECK(d.PressureUp == 255 && d.PressureDown == 255);  /* D-pad released */
}

// 10. Full chain: raw 0079:0006 report -> profile -> decode -> translate.
static void test_full_chain(void)
{
    /* square + Select + Start + hat South-East, sticks off-center.
       bytes 2 and 7 are reserved and must never leak into the state. */
    const u8 raw[8] = {7, 8, 99, 9, 10, 0x83, 0x30, 77};
    const struct hid_pad_device *profile;
    struct hid_pad_report r;
    struct ds2report d;

    profile = hid_pad_find(0x0079, 0x0006);
    CHECK(profile != NULL);

    memset(&r, 0, sizeof(r));
    CHECK(profile->decode(raw, 8, &r) == 0);

    CHECK(r.lx == 7 && r.ly == 8 && r.rx == 9 && r.ry == 10);
    CHECK(r.hat == 3);   /* byte 5 low nibble 0x03 = South-East (Down|Right) */
    CHECK(r.buttons == (BIT(HIDP_BTN_SQUARE) | BIT(HIDP_BTN_SELECT) | BIT(HIDP_BTN_START)));

    translate_pad_hid(&r, &d);
    CHECK(d.LeftStickX == 7 && d.LeftStickY == 8);
    CHECK(d.RightStickX == 9 && d.RightStickY == 10);
    CHECK((d.nButtonState & BIT(DS2BtnBit_Square)) == 0);
    CHECK((d.nButtonState & BIT(DS2BtnBit_Select)) == 0);
    CHECK((d.nButtonState & BIT(DS2BtnBit_Start)) == 0);
    CHECK((d.nButtonState & BIT(DS2BtnBit_Up)) != 0);      /* hat SE -> Up released */
    CHECK((d.nButtonState & BIT(DS2BtnBit_Down)) == 0);    /* hat SE -> Down pressed */
    CHECK((d.nButtonState & BIT(DS2BtnBit_Right)) == 0);   /* hat SE -> Right pressed */
    CHECK((d.nButtonState & BIT(DS2BtnBit_Left)) != 0);    /* hat SE -> Left released */
    CHECK((d.nButtonState & BIT(DS2BtnBit_Triangle)) != 0);/* face released */
    CHECK(d.PressureSquare == 255);
    CHECK(d.PressureTriangle == 0);
    CHECK(d.PressureDown == 0);
    CHECK(d.PressureLeft == 255);
}

int main(void)
{
    test_profile();
    test_neutral();
    test_sticks();
    test_face_buttons();
    test_shoulders_system();
    test_hat_directions();
    test_hat_released();
    test_short_reports();
    test_valid_stability();
    test_translate_buttons();
    test_translate_no_residue();
    test_combined();
    test_full_chain();

    if (g_failures) {
        printf("%d/%d checks FAILED\n", g_failures, g_checks);
        return 1;
    }

    printf("all %d checks passed\n", g_checks);
    return 0;
}