

#include "everdrive.h"

void ppuSetAddr(u16 addr);
void ppuSetPal(u8 *pal);
u8 vram_bug;


// Custom purple palette
// static u8 pal_std[] = {
//     0x0F, 0x2d, 0x0F, 0x20,
//     0x0F, 0x2d, 0x0F, 0x23,
//     0x0F, 0x2d, 0x0F, 0x3C,
//     0x0F, 0x2d, 0x0F, 0x1A,
// };

static u8 pal_std[] = {
    0x0F, 0x2d, 0x0F, 0x10,
    0x0F, 0x2d, 0x0F, 0x20,
    0x0F, 0x2d, 0x0F, 0x27,
    0x0F, 0x2d, 0x0F, 0x1A,
};

static u8 pal_safe[] = {
    0x0F, 0x1c, 0x0F, 0x20,
    0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F,
};

static u8 pal_black[] = {
    0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F,
};

static u8 pal_purple[] = {
    0x13, 0x13, 0x13, 0x13,
    0x13, 0x13, 0x13, 0x13,
    0x13, 0x13, 0x13, 0x13,
    0x13, 0x13, 0x13, 0x13,
};

void sysInit() {

    u8 i;
    u8 *str = "EDN8";

    sysVsync();
    PPU_CTRL = 0;
    PPU_MASK = 0;

    //check if ppu vram can be switched off
    REG_VRM_ATTR = VRM_MODE_TST;
    ppuSetAddr(0x2000);
    for (i = 0; str[i] != 0; i++) {
        PPU_DATA = str[i];
    }

    vram_bug = 1;
    ppuSetAddr(0x2000);
    i = PPU_DATA;
    for (i = 0; str[i] != 0; i++) {
        if (PPU_DATA != str[i])vram_bug = 0;
    }

    //vram_bug = 1;
    sysPalInit(0);



    ppuSetScroll(0, 0);
    ppuON();
}

void ppuSetPal(u8 *pal) {

    u8 i;
    
    sysVsync();
    ppuSetAddr(0x3F00);    
    for (i = 0; i < 16; i++) {                
        PPU_DATA = pal[i];
    }
        
    //ppuSetAddr(0x2000);    
    
}

void ppuSetAddr(u16 addr) {
    PPU_ADDR = addr >> 8;
    PPU_ADDR = addr & 0xff;
}

void ppuSetScroll(u8 x, u8 y) {

    PPU_ADDR = 0;
    PPU_ADDR = 0;
    PPU_SCROLL = x;
    PPU_SCROLL = y - 6;
}

void ppuOFF() {
    sysVsync();
    PPU_MASK = 0;
}

void ppuON() {
    sysVsync();
    PPU_MASK = (registery->options.pal_custom[6] << 5)  | 0x0A;
}

void sysPalInit(u8 fade_to_black) {

    u8 i;
    if (fade_to_black == 2)
    {
        ppuSetPal(pal_purple);
    }
    else if (fade_to_black ) {

        ppuSetPal(pal_black);

    } else if (vram_bug) {

        REG_VRM_ATTR = VRM_MODE_SAF;
        ppuSetPal(pal_safe);
        //clean attribute area
        ppuSetAddr(0x2300);
        for (i = 0; i < 128; i++) {
            PPU_DATA = 0x00;
            PPU_DATA = 0x00;
        }

    } else {        
        REG_VRM_ATTR = VRM_MODE_STD;
        ppuSetPal(pal_std);
    }
}

u8 sysJoyRead() {

    u8 joy;
    u8 tmp;

    joy = sysJoyRead_raw();

    if (registery->options.swap_ab) {

        tmp = joy;
        joy &= ~(JOY_B | JOY_A);
        if ((tmp & JOY_B))joy |= JOY_A;
        if ((tmp & JOY_A))joy |= JOY_B;
    }

    return joy;
}

u8 sysJoyRead_raw() {

    u16 delay;
    u8 joy = 0;
    u8 i;
    
    delay = bi_get_ticks();
    while (bi_get_ticks() - delay < 10); //antiglitch

    JOY_PORT1 = 0x01;
    JOY_PORT1 = 0x00;

    for (i = 0; i < 8; i++) {
        joy <<= 1;
        if ((JOY_PORT1 | JOY_PORT2) & 3)joy |= 1;
    }    

    
    usbListener();
    
    return joy;
}

u8 sysJoyWait() {

    u8 joy;
    static u16 time;

    if (time == 0)time = bi_get_ticks();

    while (1) {

        joy = sysJoyRead();
        if (joy == 0)break;

        if ((bi_get_ticks() - time) > JOY_DELAY) {

            time += JOY_SPEED;
            if ((joy & (JOY_B | JOY_A)) == 0)return joy;
        }


    }

    time = 0;

    while (joy == 0) {

        joy = sysJoyRead();
    }

    return joy;
}

void sysVsync() {

    volatile u8 tmp = PPU_STAT;
    while (PPU_STAT < 128);
}

u8 sysVramBug() {
    return vram_bug;
}

void sysUpdateCustomPal()
{
    int i = 0;
    
    // background 1 color
    for (i = 0; i < 16; i += 4)
    {
        pal_std[i] = registery->options.pal_custom[0];
        pal_std[i+2] = registery->options.pal_custom[0];
    }

    // background 2 color
    for (i = 1; i < 16; i += 4)
    {
        pal_std[i] = registery->options.pal_custom[1];        
    }
    
    // text colors
    pal_std[3] = registery->options.pal_custom[2];
    pal_std[7] = registery->options.pal_custom[3];
    pal_std[11] = registery->options.pal_custom[4];
    pal_std[15] = registery->options.pal_custom[5];
    
    //gCleanScreen();
    //gRepaint();
        
    sysPalInit(0);
    ppuSetScroll(0, 0);
    ppuON();
}

void sysRestoreDefaultPal()
{
    registery->options.pal_custom[0] = 0x0F;
    registery->options.pal_custom[1] = 0x2D;
    registery->options.pal_custom[2] = 0x10;
    registery->options.pal_custom[3] = 0x20;
    registery->options.pal_custom[4] = 0x27;
    registery->options.pal_custom[5] = 0x1A;
    registery->options.pal_custom[6] = 0x0;
}