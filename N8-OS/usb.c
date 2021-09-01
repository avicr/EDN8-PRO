

#include "everdrive.h"

int app_usbListener();

void usbListener() {

    u8 bank = REG_APP_BANK;
    REG_APP_BANK = APP_USB;
    app_usbListener();
    REG_APP_BANK = bank;
    
}

#pragma codeseg ("BNK06")


#define USB_CMD_TEST            't'
#define USB_CMD_REBOOT          'r'
#define USB_CMD_HALT            'h'
#define USB_CMD_SEL_GAME        'n'
#define USB_CMD_RUN_GAME        's'

void usbSelectGame();

int app_usbListener() {

    u8 cmd;
    u8 resp;

    while (1) {

        if (bi_fifo_busy())return 0;
        bi_fifo_rd(&cmd, 1);
        if (cmd != '*')continue;

        bi_fifo_rd(&cmd, 1);

        // M8 connect message
        if (cmd == 'm')
        {
            ses_cfg->m8_connected = 1;
            sysVsync();
            gCleanScreen();
            //ppuOFF();
            gSetXY(10, 10);
            gAppendString("N8M8 Connected!         Press any button to continue.");
            gRepaint();            
            sysJoyWait();
            gCleanScreen();
            //ppuON();
            return 1;
        }

        // M8 disconnect message
        if (cmd == 'd')
        {
            ses_cfg->m8_connected = 0;
            sysVsync();
            gCleanScreen();
            //ppuOFF();
            gSetXY(8, 10);
            gAppendString("N8M8 Disconnected!       Press any button to continue.");
            gRepaint();  
            sysJoyWait();
            gCleanScreen();          
            return 2;
        }

        if (cmd == USB_CMD_TEST) {
            resp = 'k';
            bi_cmd_usb_wr(&resp, 1);
            return 0;
        }

        if (cmd == USB_CMD_REBOOT) {
            ppuOFF();
            bi_reboot_usb();
        }

        if (cmd == USB_CMD_HALT) {
            ppuOFF();
            bi_halt_usb(); //wait till usb access memory
            ppuON();
            return 0;
        }

        if (cmd == USB_CMD_SEL_GAME) {
            usbSelectGame();
        }

        if (cmd == USB_CMD_RUN_GAME) {
            resp = edStartGame(1);
            printError(resp);
        }

    }

    return 0;
}

void usbSelectGame() {

    u8 resp;
    u8 *path = malloc(MAX_PATH_SIZE);

    bi_rx_string(path);

    resp = edSelectGame(path, 0);
    free(MAX_PATH_SIZE);
    bi_cmd_usb_wr(&resp, 1);


    if (resp == 0) {
        bi_cmd_usb_wr(&registery->cur_game.rom_inf.mapper, 2);
    }
}


