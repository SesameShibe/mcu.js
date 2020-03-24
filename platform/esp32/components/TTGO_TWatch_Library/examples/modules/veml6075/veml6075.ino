#pragma mark - Depend SparkFun_VEML6075_Arduino_Library
/*
cd ~/Arduino/libraries
git clone https://github.com/sparkfun/SparkFun_VEML6075_Arduino_Library.git
*/

#include <TTGO.h>
#include <Wire.h>
#include <SparkFun_VEML6075_Arduino_Library.h>

TTGOClass *ttgo;
TFT_eSPI *tft ;
VEML6075 uv;

void setup(void)
{
    Serial.begin(115200);

    ttgo = TTGOClass::getWatch();

    ttgo->begin();

    tft = ttgo->eTFT;

    tft->fillScreen(TFT_BLACK);

    ttgo->openBL();

    Wire1.begin(25, 26);
    if (uv.begin(Wire1) == false) {
        Serial.println("Unable to communicate with VEML6075.");
        while (1) ;
    }

    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextFont(4);
}


void loop(void)
{
    tft->fillRect(30, 90, 120, 80, TFT_BLACK);
    tft->setCursor(30, 90);

    tft->println("UVA:" + String(uv.uva()));

    tft->setCursor(30, tft->getCursorY());
    tft->println("UVB:" + String(uv.uvb()));

    tft->setCursor(30, tft->getCursorY());
    tft->println("UV Index:" + String(uv.index()));

    delay(1000);
}




