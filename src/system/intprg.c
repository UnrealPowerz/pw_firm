#include "all_headers.h"

#pragma section IntPRG

// ROM: 0x005e  100.0%
__interrupt(vect=7) void INT_NMI(void) {}
// ROM: 0x0060  100.0%
__interrupt(vect=8) void INT_TRAP0(void) {}
// ROM: 0x0062  100.0%
__interrupt(vect=9) void INT_TRAP1(void) {}
// ROM: 0x0064  100.0%
__interrupt(vect=10) void INT_TRAP2(void) {}
// ROM: 0x0066  100.0%
__interrupt(vect=11) void INT_TRAP3(void) {}

// ROM: 0x0068  100.0%
__interrupt(vect=13) void INT_SLEEP(void) {}

// ROM: 0xa300  83.9%  saves: r0
__interrupt(vect=16) void irq0(void) {
    statusFlags_BIT.button_event = 1;
    wakeupFlagMaybe = 1;
    CKSTPR1 |= 0x04;
    IRR1 &= ~0x01;
}

// ROM: 0xa31c  97.5%
__interrupt(vect=17) void irq1(void) { IRR1 &= ~0x02; }

// ROM: 0xa322  97.5%
__interrupt(vect=18) void irq_aec(void) { IRR1 &= ~0x04; }

// ROM: 0x006a  100.0%
__interrupt(vect=21) void INT_COMP0(void) {}
// ROM: 0x006c  100.0%
__interrupt(vect=22) void INT_COMP1(void) {}

// ROM: 0xa65e  80.6%  saves: r0
__interrupt(vect=23) void drv_rtc_handle_quarter_sec(void) {
    statusFlags_BIT.tick = 1;
    RTCFLG &= ~0x01;
}

// ROM: 0xa674  74.8%  saves: r0
__interrupt(vect=24) void drv_rtc_handle_half_sec(void) { RTCFLG &= ~0x02; }

// ROM: 0xa682  78.0%  saves: er0
__interrupt(vect=25) void drv_rtc_handle_sec(void) {
    uint8_t sec = RSECDR;
    if (!(sec & 0x80)) {
        DAT_f7a4 = sec;
    }
    DAT_f788++;
    if (DAT_f7a2 < 0xE10) {
        DAT_f7a2++;
    } else {
        DAT_f7a2 = 0xE10;
    }
    if (activityTimer != 0) {
        activityTimer--;
    }
    if (stepTimer != 0) {
        stepTimer--;
    }
    RTCFLG &= ~0x04;
}

// ROM: 0xa6e2  86.7%  saves: r0
__interrupt(vect=26) void drv_rtc_handle_min(void) {
    uint8_t min = RMINDR;
    if (!(min & 0x80)) {
        DAT_f7a5 = min;
    }
    DAT_f7a7 |= 0x01;
    RTCFLG &= ~0x08;
}

// ROM: 0xa704  87.8%  saves: r0
__interrupt(vect=27) void drv_rtc_handle_hour(void) {
    uint8_t hr = RHRDR;
    if (!(hr & 0x80)) {
        DAT_f7a6 = RHRDR;
    }
    DAT_f7a7 |= 0x02;
    RTCFLG &= ~0x10;
}

// ROM: 0x006e  100.0%
__interrupt(vect=29) void INT_RTC_WEEK(void) {}
// ROM: 0x0070  100.0%
__interrupt(vect=30) void INT_RTC_FREE(void) {}
// ROM: 0x0072  100.0%
__interrupt(vect=31) void INT_RTC_WDT(void) {}
// ROM: 0x0074  100.0%
__interrupt(vect=32) void INT_RTC_AEC_OVERFLOW(void) {}

// ROM: 0x06fa  97.5%
__interrupt(vect=33) void timer_b1_overflow(void) {
    IRR2 &= ~0x04;
}

// ROM: 0x0076  100.0%
__interrupt(vect=34) void INT_SSU_I2C(void) {}

// ROM: 0x3a4a  68.6%  saves: er1,er0
__interrupt(vect=35) void drv_timerw_isr(void) {
    drv_sound_update();
    TSRW &= ~0x01;
}

// ROM: 0x075a  100.0%
__interrupt(vect=37) void drv_ir_isr_sci3(void) {}

// ROM: 0xa328  97.5%
__interrupt(vect=38) void vector_adc(void) { IRR2 &= ~0x40; }
