
#include "everdrive.h"

void app_inGameMenu();

void inGameMenu() {

    u8 bank = REG_APP_BANK;
    REG_APP_BANK = APP_SS;
    app_inGameMenu();
    REG_APP_BANK = bank;

}

void InGameMenuReturn();

#pragma codeseg ("BNK08")

void ss_return();
void ss_reset();

#define HELP_Y_LOC 24
void DoAlpha()
{
    u8 bFinished = 0;
    u8 FolderName[29];        
    u8 FolderNameIndex = 0;
    u8* CompletePath;

    AlphaBox box;

    FolderName[0] = 0;
    box.selector = 0;

    str_copy(ses_cfg->save_folder_name, FolderName);
    FolderNameIndex = str_lenght(FolderName);

    gSetPal(PAL_B2);
    gSetXY(3, HELP_Y_LOC-17);
    gConsPrint("                ");   // clear the line first
    gSetXY(3, HELP_Y_LOC-17);
    gConsPrint(FolderName);    
    
    // Draw the current character indicator
    gSetPal(PAL_B3);
    if (FolderNameIndex < 28)
    {
        gAppendChar('_');
    }

    gSetPal(PAL_B3);
    gSetXY(5, 20);
    gConsPrint("Move           D-Pad");
    if (registery->options.swap_ab)
    {
        gConsPrint("Enter          B");
        gConsPrint("Delete         A");
    }
    else 
    {
        gConsPrint("Enter          A");
        gConsPrint("Delete         B");
    }
    gConsPrint("Accept         Start");
    gConsPrint("Cancel         Select");

    while (1)
    {
        guiDrawAlphaBox(&box);
        
        if (box.act == ACT_EXIT)
        {            
            // Don't go out of bounds       
            if (FolderNameIndex > 0)
            {
                FolderNameIndex--;  
                FolderName[FolderNameIndex] = 0;          
            }                      
        }
        else if (box.act == JOY_STA)
        {
            bFinished = 1;
        }
        else if (box.act == ACT_OPEN)
        {            
            // if (box.selector == 37)  // backspace
            // {       
            //     // Don't go out of bounds       
            //     if (FolderNameIndex > 0)
            //     {
            //         FolderNameIndex--;  
            //         FolderName[FolderNameIndex] = 0;          
            //     }
            // }            
            // else             
            if (FolderNameIndex < 28) // Don't go over 16 characters
            {
                // Numbers
                if (box.selector < 10)
                {
                    FolderName[FolderNameIndex] = 0x30 + box.selector;
                }
                else if (box.selector == 36)
                {
                    // Space
                    FolderName[FolderNameIndex] = ' ';                    
                }
                else if (box.selector == 37)
                {
                    // Dash
                    FolderName[FolderNameIndex] = '-';
                }
                else if (box.selector == 38)
                {
                    // Period
                    FolderName[FolderNameIndex] = '.';
                }
                else if (box.selector == 39)
                {
                    // Exclaim
                    FolderName[FolderNameIndex] = '!';
                }
                else
                {
                    // Alphas
                    FolderName[FolderNameIndex] = 0x41 + (box.selector-10);                    
                }
                FolderNameIndex++; 
                FolderName[FolderNameIndex] = 0;
            }
        }

        if (bFinished)
        {
            
            if (FolderNameIndex && !str_is_empty(FolderName))
            {
                // Save the folder name to the session
                str_copy(FolderName, ses_cfg->save_folder_name);
                    
                // Create the folder
                CompletePath = malloc(MAX_PATH_SIZE);
                CompletePath[0] = 0;
                if (FolderName[0] != 0)
                {
                    str_append(CompletePath, PATH_SNAP_DIR);
                    str_append(CompletePath, "/");
                    str_append(CompletePath, str_extract_name(registery->cur_game.path));
                    CompletePath[str_lenght(CompletePath) - 4] = 0;      // Remove the ".nes"                    

                    // Now create the default save folder (this is ok even if it already exists)
                    str_append(CompletePath, "/");
                    str_append(CompletePath, FolderName);
                    bi_cmd_dir_make(CompletePath);
                    free(MAX_PATH_SIZE);
                }
            }

            // Close on end
            return;            
        }

        // Cancel on select
        if (box.act == JOY_SEL)
        {            
            return;
        }        

        gSetPal(PAL_B2);
        gSetXY(3, HELP_Y_LOC-17);
        gConsPrint("                                                            ");   // clear the line first
        gSetXY(3, HELP_Y_LOC-17);
        gConsPrint(FolderName);

        // Draw the current character indicator
        gSetPal(PAL_B3);
        if (FolderNameIndex < 28)
        {
            gAppendChar('_');
        }

        gSetPal(PAL_B3);
        gSetXY(5, 20);
        gConsPrint("Move           D-Pad");
        if (registery->options.swap_ab)
        {
            gConsPrint("Enter          B");
            gConsPrint("Delete         A");
        }
        else 
        {
            gConsPrint("Enter          A");
            gConsPrint("Delete         B");
        }
        gConsPrint("Accept         Start");
        gConsPrint("Cancel         Select");
    }
}

void app_inGameMenu() {    
    enum {      
        SS_SELECT_FOLDER,
        SS_NEW_FOLDER,
        SS_MASTER_SAVE,
        SS_MASTER_LOAD,          
        SS_SAVE,
        SS_LOAD,
        SS_BANK,
        SS_RESET,
        SS_EXIT,        
        SS_SWAP_DISK,                
        SS_SIZE
    };

    u8 resp;
    FileInfo inf = {0};
    ListBox box;
    u8 * items[SS_SIZE + 1];
    u8 buff[16];        
    u8* RootSaveFolder;
    u8 ss_src;    
    u8 ss_bank_hex;    
    u8 update_info = 0;
    u8 Done = 0;

    edInit(1);
    
    REG_SST_ADDR = 0xff; //ss hit byte
    ss_src = REG_SST_DATA;

    //mem_set(&box, 0, sizeof (ListBox));
    box.hdr = 0;
    box.items = items;
    box.selector = 0;    
    items[SS_SELECT_FOLDER] = "Select Folder";
    items[SS_NEW_FOLDER] = "New Folder";
    items[SS_MASTER_SAVE] = "Save Master";
    items[SS_MASTER_LOAD] = "Load Master";
    items[SS_SAVE] = "Save State";
    items[SS_LOAD] = "Load State";
    items[SS_RESET] = "Reset Game";
    items[SS_EXIT] = "Exit Game";
    

    items[SS_SWAP_DISK] = 0; //"Swap Disk";
    items[SS_SIZE] = 0;    
    ss_bank_hex = decToBcd(ses_cfg->ss_bank);

    // Read anything off the USB
    usbListener();
    
    //quick ss section
    if (ss_src != 0xff && ss_src == registery->options.ss_key_load) {
        ppuOFF();        
        resp = srmRestoreSS(ss_bank_hex, 0);        
        if (resp)printError(resp);
        InGameMenuReturn();
    }

    if (ss_src != 0xff && ss_src == registery->options.ss_key_save) {
        ppuOFF();
        resp = srmBackupSS(ss_bank_hex, 0);
        if (resp)printError(resp);
        InGameMenuReturn();
    }

    // Really should move these M8 commands to a function instead of copying this if everywhere...
    if (ses_cfg->m8_connected)
    {
         // Game operation paused
        bi_cmd_usb_wr("!P", 2);               
    }

    resp = srmGetInfoSS(&inf, ss_bank_hex, 0);
    sysUpdateCustomPal();
    box.hdr = "ses_cfg->save_folder_name";

    while (1) {

        if (update_info) {
            resp = srmGetInfoSS(&inf, ss_bank_hex, 0);

            // Really should move these M8 commands to a function instead of copying this if everywhere...
            if (ses_cfg->m8_connected)
            {                
                // Bank select message
                bi_cmd_usb_wr("!B", 2);
                bi_cmd_usb_wr(ses_cfg->save_folder_name, 513);    
                bi_cmd_usb_wr(&ses_cfg->ss_bank, 1);

                // Give the everdrive time to send all that
                sysVsync();
                sysVsync();                
            }

            update_info = 0;
        }

        gSetPal(PAL_G2);
        if (resp == 0) {
            gDrawFooter("Save Time: ", 1, 0);
            gAppendDate(inf.date);
            gAppendString(" ");
            gAppendTime(inf.time);
        } else {
            gDrawFooter("Empty Slot", 1, G_CENTER);
        }

        buff[0] = 0;
        str_append(buff, "Slot: ");
        str_append_hex8(buff, ss_bank_hex);
        items[SS_BANK] = buff;        
        ss_bank_hex = decToBcd(ses_cfg->ss_bank);
        
        box.hdr = ses_cfg->save_folder_name;
        box.selector |= SEL_DPD;        
        guiDrawListBox(&box);
        

        if (box.act == ACT_EXIT) {            
            InGameMenuReturn();
        }


        if (/*box.selector == SS_BANK &&*/ box.act == JOY_L) {
            ses_cfg->ss_bank = dec_mod(ses_cfg->ss_bank, MAX_SS_SLOTS);
            ss_bank_hex = decToBcd(ses_cfg->ss_bank);
            update_info = 1;
        }

        if (/*box.selector == SS_BANK &&*/ box.act == JOY_R) {
            ses_cfg->ss_bank = inc_mod(ses_cfg->ss_bank, MAX_SS_SLOTS);
            ss_bank_hex = decToBcd(ses_cfg->ss_bank);
            update_info = 1;
        }

        // if (box.selector == SS_MASTER_SLOT && box.act == JOY_L) {
        //     ses_cfg->ss_master_slot = dec_mod(ses_cfg->ss_master_slot, MAX_SS_SLOTS);
        //     ss_master_slot_hex = decToBcd(ses_cfg->ss_master_slot);
        //     update_info = 1;
        // }

        // if (box.selector == SS_MASTER_SLOT && box.act == JOY_R) {
        //     ses_cfg->ss_master_slot = inc_mod(ses_cfg->ss_master_slot, MAX_SS_SLOTS);
        //     ss_master_slot_hex = decToBcd(ses_cfg->ss_master_slot);
        //     update_info = 1;
        // }


        //gCleanScreen();
        if (box.selector == SS_BANK)continue;

        if (box.act == ACT_OPEN)
        {
            // Present the file browser and select 
            if (box.selector == SS_SELECT_FOLDER)
            {
                // gCleanScreen();
                // DoAlpha();     
                // gCleanScreen();           
                // box.hdr = "ses_cfg->save_folder_name";
                RootSaveFolder = malloc(MAX_PATH_SIZE + 1);
                RootSaveFolder[0] = '/';
                RootSaveFolder[1] = 0;
                str_append(RootSaveFolder, PATH_SNAP_DIR);
                
                str_append(RootSaveFolder, "/");
                str_append(RootSaveFolder, str_extract_name(registery->cur_game.path));
                RootSaveFolder[str_lenght(RootSaveFolder) - 4] = 0;      // Remove the ".nes"                
                
                resp = fmanager(RootSaveFolder, 1);          
                
                // Debug yo
                // gSetXY(0, 0);
                // gConsPrint(RootSaveFolder);
                update_info = 1;
                free(MAX_PATH_SIZE + 1);
                gCleanScreen();                                      
            }
            else if (box.selector == SS_NEW_FOLDER)
            {
                // Do the folder name input
                gCleanScreen();
                DoAlpha();     
                gCleanScreen();                           
                update_info = 1;
            }
            else
            {
                break;
            }
        }
    }

    ppuOFF();

    if (box.selector == SS_SWAP_DISK) {
        REG_FDS_SWAP = 1;
        InGameMenuReturn();
    }

    if (box.selector == SS_MASTER_SAVE) 
    {        
        resp = srmBackupSS(ss_bank_hex, 1);

        if (resp)
        {
            printError(resp);
        }        
        ppuON();
        sysPalInit(2);        
        gRepaint();                
        InGameMenuReturn();
    }

    if (box.selector == SS_MASTER_LOAD) 
    {
        resp = srmRestoreSS(ss_bank_hex, 1);

        if (resp)
        {
            printError(resp);
        }
        ppuON();
        sysPalInit(2);        
        gRepaint();             
        InGameMenuReturn();
    }

    if (box.selector == SS_SAVE) {
        resp = srmBackupSS(ss_bank_hex, 0);
        
        if (resp)
        {
            // if we've failed, try to create the directory
            bi_cmd_dir_make("EDN8/SNAP");

            // try again
            resp = srmBackupSS(ss_bank_hex, 0);

            if (resp)
            {
                printError(resp);
            }
        }
        InGameMenuReturn();
    }

    if (box.selector == SS_LOAD) {
        resp = srmRestoreSS(ss_bank_hex, 0);
        if (resp)printError(resp);
        InGameMenuReturn();
    }

    if (box.selector == SS_RESET) {
        
        edRebootGame();
    }

    if (box.selector == SS_EXIT) {

        bi_exit_game();
    }

}

void InGameMenuReturn()
{
    if (ses_cfg->m8_connected)
    {
         // Game operation unpaused
        bi_cmd_usb_wr("!U", 2);               
    }

    ss_return();
}