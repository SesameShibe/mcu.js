
#include <TTGO.h>

TTGOClass *ttgo;
bool irq = false;

void setup()
{
    Serial.begin(115200);
    ttgo = TTGOClass::getWatch();
    ttgo->begin();
    ttgo->openBL();

    ttgo->eTFT->fillScreen(TFT_BLACK);
    ttgo->eTFT->drawString("T-Watch AXP202",  25, 50, 4);
    ttgo->eTFT->setTextFont(4);
    ttgo->eTFT->setTextColor(TFT_WHITE, TFT_BLACK);

    pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] {
        irq = true;
    }, FALLING);

    //!Clear IRQ unprocessed  first
    ttgo->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ | AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_IRQ, true);
    ttgo->power->clearIRQ();
}

void loop()
{
    if (irq) {
        irq = false;
        ttgo->power->readIRQ();
        if (ttgo->power->isVbusPlugInIRQ()) {
            ttgo->eTFT->fillRect(20, 100, 200, 85, TFT_BLACK);
            ttgo->eTFT->drawString("Power Plug In", 25, 100);
        }
        if (ttgo->power->isVbusRemoveIRQ()) {
            ttgo->eTFT->fillRect(20, 100, 200, 85, TFT_BLACK);
            ttgo->eTFT->drawString("Power Remove", 25, 100);
        }
        if (ttgo->power->isPEKShortPressIRQ()) {
            ttgo->eTFT->fillRect(20, 100, 200, 85, TFT_BLACK);
            ttgo->eTFT->drawString("PowerKey Press", 25, 100);
        }
        ttgo->power->clearIRQ();
    }
    delay(1000);
}
