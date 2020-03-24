/*
 Display all the fast rendering fonts.

 This sketch uses the GLCD (font 1) and fonts 2, 4, 6, 7, 8

 Make sure all the display driver and pin comnenctions are correct by
 editting the User_Setup.h file in the TFT_eSPI library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
 #########################################################################
*/

// New background colour
#define TFT_BROWN 0x38E0

// Pause in milliseconds between screens, change to 0 to time font rendering
#define WAIT 500

#include <TTGO.h>

TTGOClass *ttgo;


unsigned long targetTime = 0; // Used for testing draw times

void setup(void)
{
    ttgo = TTGOClass::getWatch();
    ttgo->begin();
    ttgo->openBL();
}

void loop()
{
    targetTime = millis();

    // First we test them with a background colour set
    ttgo->eTFT->setTextSize(1);
    ttgo->eTFT->fillScreen(TFT_BLACK);
    ttgo->eTFT->setTextColor(TFT_GREEN, TFT_BLACK);

    ttgo->eTFT->drawString(" !\"#$%&'()*+,-./0123456", 0, 0, 2);
    ttgo->eTFT->drawString("789:;<=>?@ABCDEFGHIJKL", 0, 16, 2);
    ttgo->eTFT->drawString("MNOPQRSTUVWXYZ[\\]^_`", 0, 32, 2);
    ttgo->eTFT->drawString("abcdefghijklmnopqrstuvw", 0, 48, 2);
    int xpos = 0;
    xpos += ttgo->eTFT->drawString("xyz{|}~", 0, 64, 2);
    ttgo->eTFT->drawChar(127, xpos, 64, 2);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BLACK);
    ttgo->eTFT->setTextColor(TFT_GREEN, TFT_BLACK);

    ttgo->eTFT->drawString(" !\"#$%&'()*+,-.", 0, 0, 4);
    ttgo->eTFT->drawString("/0123456789:;", 0, 26, 4);
    ttgo->eTFT->drawString("<=>?@ABCDE", 0, 52, 4);
    ttgo->eTFT->drawString("FGHIJKLMNO", 0, 78, 4);
    ttgo->eTFT->drawString("PQRSTUVWX", 0, 104, 4);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BLACK);
    ttgo->eTFT->drawString("YZ[\\]^_`abc", 0, 0, 4);
    ttgo->eTFT->drawString("defghijklmno", 0, 26, 4);
    ttgo->eTFT->drawString("pqrstuvwxyz", 0, 52, 4);
    xpos = 0;
    xpos += ttgo->eTFT->drawString("{|}~", 0, 78, 4);
    ttgo->eTFT->drawChar(127, xpos, 78, 4);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BLACK);
    ttgo->eTFT->setTextColor(TFT_BLUE, TFT_BLACK);

    ttgo->eTFT->drawString("012345", 0, 0, 6);
    ttgo->eTFT->drawString("6789", 0, 40, 6);
    ttgo->eTFT->drawString("apm-:.", 0, 80, 6);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BLACK);
    ttgo->eTFT->setTextColor(TFT_RED, TFT_BLACK);

    ttgo->eTFT->drawString("0123", 0, 0, 7);
    ttgo->eTFT->drawString("4567", 0, 60, 7);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BLACK);
    ttgo->eTFT->drawString("890:.", 0, 0, 7);
    ttgo->eTFT->drawString("", 0, 60, 7);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BLACK);
    ttgo->eTFT->setTextColor(TFT_YELLOW, TFT_BLACK);

    ttgo->eTFT->drawString("01", 0, 0, 8);
    delay(WAIT);

    ttgo->eTFT->drawString("23", 0, 0, 8);
    delay(WAIT);

    ttgo->eTFT->drawString("45", 0, 0, 8);
    delay(WAIT);

    ttgo->eTFT->drawString("67", 0, 0, 8);
    delay(WAIT);

    ttgo->eTFT->drawString("89", 0, 0, 8);
    delay(WAIT);

    ttgo->eTFT->drawString("0:.", 0, 0, 8);
    delay(WAIT);

    ttgo->eTFT->setTextColor(TFT_MAGENTA);
    ttgo->eTFT->drawNumber(millis() - targetTime, 0, 100, 4);
    delay(4000);

    // Now test them with transparent background
    targetTime = millis();

    ttgo->eTFT->setTextSize(1);
    ttgo->eTFT->fillScreen(TFT_BROWN);
    ttgo->eTFT->setTextColor(TFT_GREEN);

    ttgo->eTFT->drawString(" !\"#$%&'()*+,-./0123456", 0, 0, 2);
    ttgo->eTFT->drawString("789:;<=>?@ABCDEFGHIJKL", 0, 16, 2);
    ttgo->eTFT->drawString("MNOPQRSTUVWXYZ[\\]^_`", 0, 32, 2);
    ttgo->eTFT->drawString("abcdefghijklmnopqrstuvw", 0, 48, 2);
    xpos = 0;
    xpos += ttgo->eTFT->drawString("xyz{|}~", 0, 64, 2);
    ttgo->eTFT->drawChar(127, xpos, 64, 2);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BROWN);
    ttgo->eTFT->setTextColor(TFT_GREEN);

    ttgo->eTFT->drawString(" !\"#$%&'()*+,-.", 0, 0, 4);
    ttgo->eTFT->drawString("/0123456789:;", 0, 26, 4);
    ttgo->eTFT->drawString("<=>?@ABCDE", 0, 52, 4);
    ttgo->eTFT->drawString("FGHIJKLMNO", 0, 78, 4);
    ttgo->eTFT->drawString("PQRSTUVWX", 0, 104, 4);

    delay(WAIT);
    ttgo->eTFT->fillScreen(TFT_BROWN);
    ttgo->eTFT->drawString("YZ[\\]^_`abc", 0, 0, 4);
    ttgo->eTFT->drawString("defghijklmno", 0, 26, 4);
    ttgo->eTFT->drawString("pqrstuvwxyz", 0, 52, 4);
    xpos = 0;
    xpos += ttgo->eTFT->drawString("{|}~", 0, 78, 4);
    ttgo->eTFT->drawChar(127, xpos, 78, 4);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BROWN);
    ttgo->eTFT->setTextColor(TFT_BLUE);

    ttgo->eTFT->drawString("012345", 0, 0, 6);
    ttgo->eTFT->drawString("6789", 0, 40, 6);
    ttgo->eTFT->drawString("apm-:.", 0, 80, 6);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BROWN);
    ttgo->eTFT->setTextColor(TFT_RED);

    ttgo->eTFT->drawString("0123", 0, 0, 7);
    ttgo->eTFT->drawString("4567", 0, 60, 7);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BROWN);
    ttgo->eTFT->drawString("890:.", 0, 0, 7);
    ttgo->eTFT->drawString("", 0, 60, 7);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BROWN);
    ttgo->eTFT->setTextColor(TFT_YELLOW);

    ttgo->eTFT->drawString("0123", 0, 0, 8);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BROWN);
    ttgo->eTFT->drawString("4567", 0, 0, 8);
    delay(WAIT);

    ttgo->eTFT->fillScreen(TFT_BROWN);
    ttgo->eTFT->drawString("890:.", 0, 0, 8);
    delay(WAIT);

    ttgo->eTFT->setTextColor(TFT_MAGENTA);

    ttgo->eTFT->drawNumber(millis() - targetTime, 0, 100, 4);
    delay(4000);;
}

