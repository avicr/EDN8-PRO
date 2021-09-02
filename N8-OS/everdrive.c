
#include "everdrive.h"



u8 edMapRoutLoad();
u8 edRegisteryLoad();
u8 edRegisteryReset();
u8 edLoadSysyInfo();
void bootloader(u8 *boot_flag);
u8 edVramBugHandler();
u8 edLoadFdsBios();
u8 edBramBackup();
u8 edBramRestore();

Registery *registery;
SysInfo *sys_inf;
SessionCFG *ses_cfg;

u8 *maprout;

u8 edInit(u8 sst_mode) {

    u8 resp;
    //if (REG_MAP_IDX != 0xff)bi_exit_game();
    std_init();
    sysInit();
    gInit();
    guiInit();

    registery = malloc(sizeof (Registery));
    sys_inf = malloc(sizeof (SysInfo));
    maprout = malloc(256);
    ses_cfg = malloc(sizeof (SessionCFG)); //this memory allocated in OS memory area. It resets to 0x00 only at cold boot.
    fmInitMemory();
    mem_set(sys_inf, 0, sizeof (SysInfo));    

    if (sst_mode)return 0; //all memory should be allocated before this point
    ses_cfg->ss_bank = 0;

    bootloader(&ses_cfg->boot_flag);

    resp = bi_init();
    if (resp)return resp;

    resp = edLoadSysyInfo();
    if (resp)return resp;

    resp = edMapRoutLoad();
    if (resp)return resp;

    resp = edRegisteryLoad();
    if (resp == ERR_REGI_CRC || resp == FAT_NO_FILE) {
        resp = edRegisteryReset();
        printError(ERR_REGI_CRC);
    }
    if (resp)return resp;

    resp = edVramBugHandler();
    if (resp)return resp;

    //gConsPrint("ok");
    //gRepaint();

    if (sys_inf->mcu.ram_rst) {
        bi_cmd_mem_set(RAM_NULL, ADDR_SRM, SIZE_SRM);
        registery->ram_backup_req = 0;
        resp = edRegisterySave();
        if (resp)return resp;
        rtcReset();
        printError(ERR_BAT_RDY);
    }

    if (ses_cfg->hot_start == 0) {
        resp = updateCheck();
        if (resp)return resp;
    }

    // gAppendHex8(registery->options.ss_mode);
    // gRepaint();
    // sysJoyWait();

    resp = edBramBackup();
    if (resp)return resp;

    if (ses_cfg->hot_start == 0) {
        ses_cfg->hot_start = 1; //should be in the end
        if (registery->options.autostart) {
            edStartGame(0);
        }
    }

    return 0;
}

u8 edLoadSysyInfo() {

    sys_inf->os_ver = OS_VER;
    sys_inf->os_bld_date = *(u16 *) 0xFFF0;
    sys_inf->os_bld_time = *(u16 *) 0xFFF2;
    sys_inf->os_dist_date = *(u16 *) 0xFFF4;
    sys_inf->os_dist_time = *(u16 *) 0xFFF6;
    bi_cmd_sys_inf(&sys_inf->mcu);

    //sys_inf->asm_date = mcu.asm_date; //0x4e66;
    //sys_inf->asm_time = mcu.asm_time; //0x1234;
    return 0;
}

void edRun() {

    u8 resp;
    
    while (1) {
        resp = fmanager("", 0);
        printError(resp);
        if (resp == FAT_DISK_ERR || resp == FAT_NOT_READY) {
            bi_cmd_disk_init();
        }
    }
}

u8 edMapRoutLoad() {

    u8 resp;
    resp = bi_cmd_file_open(PATH_MAPROUT, FA_READ);
    if (resp)return resp;
    resp = bi_cmd_file_read(maprout, 256);
    if (resp)return resp;
    resp = bi_cmd_file_close();
    if (resp)return resp;


    return 0;
}

u8 edRegisteryLoad() {

    u8 resp;
    u16 crc;
    //mem_set(registery, 0, sizeof (Registery));

    resp = bi_cmd_file_open(PATH_REGISTERY, FA_READ);
    if (resp)return resp;

    resp = bi_cmd_file_read(registery, sizeof (Registery));
    if (resp)return resp;

    resp = bi_cmd_file_close();
    if (resp)return resp;

    crc = crcFast(registery, sizeof (Registery) - 2);
    if (crc != registery->crc)return ERR_REGI_CRC;
    
    // Apply the custom palette
    sysUpdateCustomPal();
    
    return 0;
}

u8 edRegisteryReset() {

    mem_set(registery, 0, sizeof (Registery));

    registery->options.cheats = 1;
    registery->options.sort_files = 1;
    registery->options.ss_key_menu = JOY_STA | JOY_D;
    registery->options.ss_key_save = SS_COMBO_OFF;
    registery->options.ss_key_load = SS_COMBO_OFF;
    registery->options.ss_mode = 1;
    registery->options.fds_auto_swp = 1;

    registery->options.pal_custom[0] = 0x0F;
    registery->options.pal_custom[1] = 0x2D;
    registery->options.pal_custom[2] = 0x10;
    registery->options.pal_custom[3] = 0x20;
    registery->options.pal_custom[4] = 0x27;
    registery->options.pal_custom[5] = 0x1A;
    registery->options.pal_custom[6] = 0x0;

    volSetDefaults();

    str_copy(PATH_DEF_GAME, registery->cur_game.path);
    registery->cur_game.rom_inf.supported = 1;

    return edRegisterySave();
}

u8 edRegisterySave() {

    u8 resp;

    resp = bi_cmd_file_open(PATH_REGISTERY, FA_WRITE | FA_OPEN_ALWAYS);
    if (resp)return resp;

    registery->crc = crcFast(registery, sizeof (Registery) - 2);

    resp = bi_cmd_file_write(registery, sizeof (Registery));
    if (resp)return resp;

    resp = bi_cmd_file_close();
    if (resp)return resp;

    return 0;
}

u8 edSelectGame(u8 *path, u8 recent_add) 
{
    u8 resp;    
    ppuOFF();

    //in case if file to ram been used before change the game
    resp = edBramBackup();
    if (resp)return resp;
    //resp = srmBackup();
    //if (resp)return resp;


    mem_set(&registery->cur_game, 0, sizeof (Game));

    resp = getRomInfo(&registery->cur_game.rom_inf, path);
    if (resp)return resp;

    if (registery->cur_game.rom_inf.usb_game) {
        path += 4; //skip "USB:" identificator
    }

    str_copy(path, registery->cur_game.path);

    resp = edRegisterySave();
    if (resp)return resp;

    //resp = srmRestore();
    //if (resp)return resp;

    if (recent_add) {
        resp = recentAdd(path);
        if (resp)return resp;
    }

    if (recent_add) {
        bi_cmd_game_ctr(); //not count usb games and recently played
        sys_inf->mcu.game_ctr++;
    }

    if (!registery->cur_game.rom_inf.usb_game) {
        gCleanScreen();
        gRepaint();
    }
       
    return 0;
}

void edCreateDefaultSaveFolder()
{
    u8* GameSaveFolder;    
    
    // Allocate a big ass chunk of memory
    GameSaveFolder = malloc(MAX_PATH_SIZE);
    GameSaveFolder[0] = 0;

    // First create a folder with the ROM name inside of EDN8/SNAP
    str_append(GameSaveFolder, PATH_SNAP_DIR);
    str_append(GameSaveFolder, "/");
    str_append(GameSaveFolder, str_extract_name(registery->cur_game.path));
    GameSaveFolder[str_lenght(GameSaveFolder) - 4] = 0;      // Remove the ".nes"
    bi_cmd_dir_make(GameSaveFolder);

    // Now create the default save folder (this is ok even if it already exists)
    str_append(GameSaveFolder, "/");
    str_append(GameSaveFolder, "DEFAULT");
    str_copy("DEFAULT", ses_cfg->save_folder_name);
    bi_cmd_dir_make(GameSaveFolder);
    gAppendString(ses_cfg->save_folder_name);

    // Give the everdrive a little time to complete the dir make commands before freeing the memory    
    sysVsync();
    sysVsync();
    free(MAX_PATH_SIZE);
}

void edGetMapConfig(RomInfo *inf, MapConfig *cfg) {

    mem_set(cfg, 0, sizeof (MapConfig));

    cfg->prg_msk |= bi_get_rom_mask(inf->prg_size);
    cfg->prg_msk |= bi_get_srm_mask(inf->srm_size) << 4;
    cfg->chr_msk |= bi_get_rom_mask(inf->chr_size) & 15;

    cfg->chr_msk |= (inf->mapper & 0xf00) >> 4;
    cfg->map_idx = inf->mapper & 0xff;


    cfg->ss_key_menu = SS_COMBO_OFF;
    cfg->ss_key_save = SS_COMBO_OFF;
    cfg->ss_key_load = SS_COMBO_OFF;
    cfg->map_ctrl = MAP_CTRL_UNLOCK;
    cfg->master_vol = volGetMasterVol(cfg->map_idx);
    if (sys_inf->mcu.cart_form) {
        cfg->map_ctrl |= MAP_CTRL_FAMI;
    }

    cfg->map_cfg |= inf->submap << 4;
    if (inf->srm_size == 0)cfg->map_cfg |= MCFG_SRM_OFF;
    if (inf->chr_ram)cfg->map_cfg |= MCFG_CHR_RAM;

    if (inf->mir_mode == MIR_HOR)cfg->map_cfg |= MCFG_MIR_H;
    if (inf->mir_mode == MIR_VER)cfg->map_cfg |= MCFG_MIR_V;
    if (inf->mir_mode == MIR_4SC)cfg->map_cfg |= MCFG_MIR_4;
    if (inf->mir_mode == MIR_1SC)cfg->map_cfg |= MCFG_MIR_1;

    //forcing simple incremental swap method instead smart swap. Smart method may not work for some games

    if (inf->prg_size > SIZE_FDS_DISK * 2 && inf->rom_type == ROM_TYPE_FDS) {
        //during disk swap use increment disk mode instead of auto detecion for multi disk games.
        //seems like muulti disk gams does not actualy have correct disk number in file request header
        cfg->map_cfg |= MCFG_FDS_ASW;
    }

}

u8 edApplyOptions(MapConfig *cfg) {

    u8 resp;
    Options *opt = &registery->options;

    if (opt->cheats) {
        resp = ggLoadCodes(&cfg->gg, registery->cur_game.path);
        if (resp)return resp;
        cfg->map_ctrl |= MAP_CTRL_GG_ON;
    }

    if (opt->rst_delay) {
        cfg->map_ctrl |= MAP_CTRL_RDELAY;
    }

    if (opt->ss_mode) {


        cfg->ss_key_save = registery->options.ss_key_save;
        cfg->ss_key_load = registery->options.ss_key_load;
        cfg->ss_key_menu = registery->options.ss_key_menu;

        if (cfg->map_idx != MAP_IDX_FDS)cfg->map_ctrl |= MAP_CTRL_SS_BTN;
        cfg->map_ctrl |= MAP_CTRL_SS_ON;

        /*
        if (opt->ss_mode == SS_MOD_QSS) {
            cfg->ss_key_load = registery->options.ss_key_load;
        }
         */
    }

    if (opt->fds_auto_swp && registery->cur_game.rom_inf.rom_type == ROM_TYPE_FDS) {
        cfg->map_cfg |= MCFG_FDS_ASW;
    }

    return 0;
}

u8 edStartGame(u8 usb_mode) {

    u8 ext_bios = 0;
    u8 resp;
    u16 i;
    u8* GameSaveFolder;   
    MapConfig *cfg = &ses_cfg->cfg;
    RomInfo *cur_game = &registery->cur_game.rom_inf;

    //if (registery->cur_game.path[0] == 0)return ERR_GAME_NOT_SEL;
    if (registery->cur_game.rom_inf.supported == 0 && !usb_mode)return ERR_MAP_NOT_SUPP;
    if (cur_game->usb_game && !usb_mode)return ERR_USB_GAME;

    // Allocate a big ass chunk of memory
    GameSaveFolder = malloc(MAX_PATH_SIZE);
    GameSaveFolder[0] = 0;
    // First create a folder with the ROM name inside of EDN8/SNAP
    str_append(GameSaveFolder, PATH_SNAP_DIR);
    str_append(GameSaveFolder, "/");
    str_append(GameSaveFolder, str_extract_name(registery->cur_game.path));
    GameSaveFolder[str_lenght(GameSaveFolder) - 4] = 0;      // Remove the ".nes"
    bi_cmd_dir_make(GameSaveFolder);

    // Now create the default save folder (this is ok even if it already exists)
    str_append(GameSaveFolder, "/");
    str_append(GameSaveFolder, "DEFAULT");
    str_copy("DEFAULT", ses_cfg->save_folder_name);
    bi_cmd_dir_make(GameSaveFolder);
    free(MAX_PATH_SIZE);

    ppuOFF();
    resp = edBramRestore();
    if (resp)return resp;

    if (cur_game->rom_type == ROM_TYPE_FDS) {//load fds bios if exists
        resp = edLoadFdsBios();
        if (resp == 0)ext_bios = 1;
    }

    if (usb_mode) {
        //do nothing
    } else if (cur_game->rom_type == ROM_TYPE_FDS) {

        resp = srmRestoreFDS();
        if (resp)return resp;

    } else {

        resp = bi_cmd_file_open(registery->cur_game.path, FA_READ);
        if (resp)return resp;

        resp = bi_cmd_file_set_ptr(cur_game->dat_base);
        if (resp)return resp;

        resp = bi_cmd_file_read_mem(ADDR_PRG, cur_game->prg_size);
        if (resp)return resp;

        if (!cur_game->chr_ram) {
            resp = bi_cmd_file_read_mem(ADDR_CHR, cur_game->chr_size);
            if (resp)return resp;
        }

        resp = bi_cmd_file_close();
        if (resp)return resp;

        if (cur_game->prg_save) {
            ses_cfg->save_prg = 1;
        }

    }

    edGetMapConfig(cur_game, cfg);
    resp = edApplyOptions(cfg);
    if (resp)return resp;

    if (ext_bios)cfg->map_cfg |= MCFG_FDS_EBI;

    PPU_CTRL = 0x00;
    //PPU_ADDR = 0x3f;
    //PPU_ADDR = 0x00;
    //PPU_DATA = 0x30;

    //apu initialize
    for (i = 0; i < 0x13; i++) {
        ((u8 *) 0x4000)[i] = 0;
    }

    *(u8 *) 0x4015 = 0x00;
    *(u8 *) 0x4017 = 0x40;


    if (usb_mode) {
        bi_cmd_fpg_init_usb();
    } else {
        u8 map_path[32];
        edGetMapPath(cur_game->map_pack, map_path);
        resp = bi_cmd_fpg_init_sdc(map_path); //reconfigure fpga
        if (resp)return resp;
    }

    // Really should move these M8 commands to a function instead of copying this if everywhere...
    if (ses_cfg->m8_connected)
    {
        bi_cmd_usb_wr("!G", 2);
        bi_cmd_usb_wr(registery->cur_game.path, 513);    
    }
    
    // Give the USB write a little time to finish
    //sysVsync();    

    //mem_copy(&cfg, &ses_cfg->cfg, sizeof (MapConfig));
    // Read anything off the usb
    usbListener();
    bi_start_app(cfg);    
    
    return 0;
}

void edRebootGame() {

    u16 i;

    gCleanScreen();
    gRepaint();
    ppuOFF();
    PPU_CTRL = 0x00;

    //apu initialize
    for (i = 0; i < 0x13; i++) {
        ((u8 *) 0x4000)[i] = 0;
    }

    *(u8 *) 0x4015 = 0x00;
    *(u8 *) 0x4017 = 0x40;

    REG_SST_ADDR = 0;
    for (i = 0; i < 256; i++) {
        REG_SST_DATA = 0;
    }

    bi_cmd_mem_set(0, ADDR_CFG, sizeof (MapConfig));
    bi_start_app(&ses_cfg->cfg);
}

void edGetMapPath(u8 map_pack, u8 *path) {

    path[0] = 0;
    str_append(path, PATH_MAP);
    str_append(path, "/");
    if (map_pack < 128)str_append(path, "0");
    if (map_pack < 10)str_append(path, "0");
    str_append_num(path, map_pack);
    str_append(path, ".RBF");
}

u8 edVramBugHandler() {

    u8 resp;
    u8 i;

    u8 * msg[] = {
        //"00000000000000000000000000000000",
        "",
        "",
        "!WARNING!",
        "PPU VRAM bug detected.",
        "",
        "",
        "Such bug presents only in ",
        "poorly made console clones.",
        "",
        "Due this bug some advanced",
        "mappers can not work properly",
        "For example mappers with",
        "4-screen mirroring or advanced",
        "graphics features in MMC5.",
        "",
        "",
        "Also this bug effects N8 menu.",
        "In safe mode menu working slow",
        "and with another color scheme.",
        0
        //"00000000000000000000000000000000",
    };


    if (sysVramBug() != 0 && registery->vram_bug_msg == 0) {

        registery->vram_bug_msg = 1;
        resp = edRegisterySave();
        if (resp)return resp;

        gCleanScreen();

        for (i = 0; msg[i] != 0; i++) {
            gConsPrintCX(msg[i]);
        }
        gRepaint();
        for (i = 0; i < 60; i++)sysVsync();
        sysJoyWait();

    }

    if (sysVramBug() == 0 && registery->vram_bug_msg != 0) {
        registery->vram_bug_msg = 0;
        resp = edRegisterySave();
        if (resp)return resp;
    }

    return 0;
}

u8 edLoadFdsBios() {

    u8 resp;
    u16 i;
    u16 addr;
    u8 len;
    static const u8 fix[] = {
        0x05, 0x01, 0xCE,
        0x8D, 0x24, 0x40, 0xAE, 0x31,
        0x06, 0x05, 0x7A,
        0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
        0x0A, 0x06, 0xCA,
        0xA5, 0x07, 0x20, 0x94, 0xE7, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
        0x03, 0x07, 0x06,
        0xEA, 0xEA, 0xEA,
        0x1A, 0x07, 0x1B,
        0xEA, 0xEA, 0xEA, 0xA2, 0x27, 0xAD, 0x30, 0x40,
        0x29, 0x10, 0xD0, 0x5A, 0xF0, 0x23, 0xEA, 0xEA,
        0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
        0x00
    };

    resp = bi_cmd_file_open(PATH_FDS_BIOS, FA_READ);
    if (resp)return resp;

    resp = bi_cmd_file_read_mem(ADDR_FDS_BIOS, 8192);
    if (resp)return resp;

    resp = bi_cmd_file_close();
    if (resp)return resp;

    for (i = 0; fix[i] != 0; i += len + 3) {
        len = fix[i];
        addr = (fix[i + 1] << 8) | fix[i + 2];
        bi_cmd_mem_wr(ADDR_FDS_BIOS + addr, &fix[i + 3], len);
    }


    return 0;
}

u8 edBramBackup() {

    u8 resp;
    if (!registery->ram_backup_req)return 0;

    //for loaded via usb games skip save types which going to write back to the rom file
    if (registery->cur_game.rom_inf.usb_game == 0) {

        resp = srmBackupFDS();
        if (resp)return resp;

        if (ses_cfg->save_prg) {
            ses_cfg->save_prg = 0;
            resp = srmBackupPRG();
            if (resp)return resp;
        }
    }

    resp = srmBackup();
    if (resp)return resp;

    registery->ram_backup_req = 0;
    return edRegisterySave();
}

u8 edBramRestore() {

    u8 resp;
    if (registery->ram_backup_req)return 0;

    resp = srmRestore();
    if (resp)return resp;

    registery->ram_backup_req = 1;
    return edRegisterySave();
}