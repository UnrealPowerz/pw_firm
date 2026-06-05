/*
 * menu_consts.h - Main-menu item indices, shared across ui/home.c
 * (button shortcuts) and ui/main_menu.c (the menu itself).
 *
 * Order matches the main-menu sprite/icon layout in EEPROM at
 * 0x280 + g.ui_menuCursor * 0x40. Cost lookups go through MENU_ITEM_COSTS[].
 */

#ifndef MENU_CONSTS_H
#define MENU_CONSTS_H

enum main_menu_item {
    MENU_POKERADAR     = 0,
    MENU_DOWSING       = 1,
    MENU_CONNECTION    = 2,
    MENU_TRAINER_CARD  = 3,
    MENU_INVENTORY     = 4,
    MENU_SETTINGS      = 5
};

#endif /* MENU_CONSTS_H */
