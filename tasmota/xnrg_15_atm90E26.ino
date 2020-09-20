//{"NAME":"Wemos Mini ATM90E26 DIN","GPIO":[114,0,29,0,6,5,0,0,111,112,113,114,0],"FLAG":0,"BASE":18}
//{"NAME":"ATM90E26","GPIO":[0,0,29,0,0,0,0,0,111,112,113,114,0],"FLAG":0,"BASE":18}

#ifdef USE_SPI
#ifdef USE_ENERGY_SENSOR
#ifdef USE_ATM90E26

#define XNRG_15 15

// Support both?
//#include <energyic_UART.h>
#include <energyic_SPI.h>

//#ifdef ATM90E26_2 etc for more channels(devices)?
//in this case of the DIN monitor always will have 2x, only one in the featherwing, make this geenric for both

//TODO:Make better use of OLED display

ATM90E26_SPI *eic1;
ATM90E26_SPI *eic2;

// Wish this was in a header
// Do we make is the same as Energy struct or reduce mem usage - reduce?
// This is not a 3 phase chip
// At this stage not generic driver for ATM 3 phase chips with config to select which one
struct ATM90E26 {
  float voltage[3] = {0, 0, 0};              // 123.1 V
  float current[3] = {0, 0, 0};              // 123.123 A
  float active_power[3] = {0, 0, 0};         // 123.1 W
  float apparent_power[3] = {NAN, NAN, NAN}; // 123.1 VA
  float reactive_power[3] = {NAN, NAN, NAN}; // 123.1 VAr
  float power_factor[3] = {NAN, NAN, NAN};   // 0.12
  float frequency[3] = {NAN, NAN, NAN};      // 123.1 Hz
} atm90e26;

void AtmSnsInit(void) {

  // Calibration
  uint32_t lgain = 0x240b;
  uint32_t eic1_ugain = 0x6810;
  uint32_t eic1_igain = 0x7644;
  uint32_t eic2_ugain = 0x6720;
  uint32_t eic2_igain = 0x7644;

  // Initialise the ATM90E26 + SPI port
  // Should we test status for success... probably....

  // Is it better to init at the top / on declaration?
  // should grab the pin from definitions, setup when do DrvInit()
  eic1 = new ATM90E26_SPI(15);
  eic2 = new ATM90E26_SPI(0);

  eic1->SetLGain(lgain);
  eic1->SetUGain(eic1_ugain);
  eic1->SetIGain(eic1_igain);
  eic1->SetCRC1(eic1->CalcCheckSum(1));
  eic1->SetCRC2(eic1->CalcCheckSum(2));
  eic1->InitEnergyIC();
  eic2->SetLGain(lgain);
  eic2->SetUGain(eic2_ugain);
  eic2->SetIGain(eic2_igain);
  eic2->SetCRC1(eic2->CalcCheckSum(1));
  eic2->SetCRC2(eic2->CalcCheckSum(2));
  eic2->InitEnergyIC();

  AddLog_P2(LOG_LEVEL_INFO,
            PSTR("ATM90E26: "
                 "SysStaus1=%x "
                 "MeterStatus1=%x "
                 "SysStaus2=%x "
                 "MeterStatus2=%x "),
            eic1->GetSysStatus(),
            eic1->GetMeterStatus(),
            eic2->GetSysStatus(),
            eic2->GetMeterStatus()
            );

}


// own line? can do it in a column instad? not a sumple appen then
// neither will scale to 8x sensors, perhaps each refersh show a new one?
#ifdef USE_WEBSERVER 
//const char HTTP_ENERGY_ATM90E26[] PROGMEM =
//    "<tr><th colspan=\"2\" style=\"text-align:center\">" "Sensor 2" "</th></tr>"
 //   HTTP_SNS_VOLTAGE
 //   HTTP_SNS_CURRENT
//    HTTP_SNS_POWER
//    HTTP_ENERGY_SNS1
//    "{s}" D_FREQUENCY "{m}%s " D_UNIT_HERTZ "{e}";
const char HTTP_ENERGY_ATM90E26_HEADER[] PROGMEM =
  "<tr><th colspan=\"2\" style=\"text-align:center\">" "Sensor 2" "</th></tr>";
const char HTTP_SNS_FREQUENCY[] PROGMEM =
    "{s}" D_FREQUENCY "{m}%s " D_UNIT_HERTZ "{e}";

#endif

//constant char JSON_ENERGY_ATM90E26[] PROGMEM =

// TODO:Add status parser to driver then send that instead
// TODO:Add status strings to i18n
void AtmShow(bool json) {

    //here only supporting single phase for now
    char voltage_chr[FLOATSZ];
    dtostrfd(atm90e26.voltage[0], Settings.flag2.voltage_resolution,
             voltage_chr);
    char current_chr[FLOATSZ];
    dtostrfd(atm90e26.current[0], Settings.flag2.current_resolution,
             current_chr);
    char active_power_chr[FLOATSZ];
    dtostrfd(atm90e26.active_power[0], Settings.flag2.wattage_resolution,
             active_power_chr);
    char apparent_power_chr[FLOATSZ];
    dtostrfd(atm90e26.apparent_power[0], Settings.flag2.wattage_resolution,
             apparent_power_chr);
    char reactive_power_chr[FLOATSZ];
    dtostrfd(atm90e26.reactive_power[0], Settings.flag2.wattage_resolution,
             reactive_power_chr);
    char power_factor_chr[FLOATSZ];
    dtostrfd(atm90e26.power_factor[0], 2, power_factor_chr);
    char frequency_chr[FLOATSZ];
    dtostrfd(atm90e26.frequency[0], Settings.flag2.frequency_resolution,
             frequency_chr);

  if (json) {

    ResponseAppend_P(PSTR(",\"" "SysStatus" "\":\"%x\""),eic1->GetSysStatus());
    ResponseAppend_P(PSTR(",\"" "MeterStatus" "\":\"%x\""),eic1->GetMeterStatus());
    // All should be on the one sensor line
    // https://github.com/arendst/Tasmota/issues/339
    // With that, currently add ENERGYn, is there a best practic way that's better?
    if (Settings.flag4.atm90e26_channel2){
      ResponseAppend_P(PSTR("},\"" D_JSON_ENERGY "%d\":{"),2);
      ResponseAppend_P(PSTR("\"" D_JSON_POWERUSAGE "\":%s"), active_power_chr);
      ResponseAppend_P(PSTR(",\"" D_JSON_APPARENT_POWERUSAGE "\":%s,\"" D_JSON_REACTIVE_POWERUSAGE "\":%s,\"" D_JSON_POWERFACTOR "\":%s"), apparent_power_chr, reactive_power_chr, power_factor_chr);
      ResponseAppend_P(PSTR(",\"" D_JSON_FREQUENCY "\":%s"), frequency_chr);
      ResponseAppend_P(PSTR(",\"" D_JSON_VOLTAGE "\":%s"), voltage_chr);
      ResponseAppend_P(PSTR(",\"" D_JSON_CURRENT "\":%s"), current_chr);
      ResponseAppend_P(PSTR(",\"" "SysStatus" "\":\"%x\""),eic2->GetSysStatus());
      ResponseAppend_P(PSTR(",\"" "MeterStatus" "\":\"%x\""),eic2->GetMeterStatus());
    }
 
    //need to add support to others, do they work with multiple of the same type coming from the one sensor?
    //#ifdef USE_DOMOTICZ
    //DomoticzSensor()
    //DZ_VOLTAGE, DZ_CURRENT, DZ_POWER_ENERGY
    //#ifdef USE_KNX
    //KnxSensor()
    //KNX_ENERGY_VOLTAGE, KNX_ENERGY_CURRENT, KNX_ENERGY_POWER, KNX_ENERGY_POWERFACTOR, KNX_ENERGY_DAILY, KNX_ENERGY_START, KNX_ENERGY_TOTAL

#ifdef USE_WEBSERVER 
  } else {
    //What does EnergyFormatIndex do? GetTextIndexed? -> probably single string/row
    //we can probaly ignore EnergyFormat, really for multiple phases but might be good for 3ph support later
    //not used in JSON version so can see both usages
    if (Settings.flag4.atm90e26_channel2){
      char value_chr[FLOATSZ *3];
      char value2_chr[FLOATSZ *3];
      char value3_chr[FLOATSZ *3];
      WSContentSend_PD(HTTP_ENERGY_ATM90E26_HEADER);
      WSContentSend_PD(HTTP_SNS_VOLTAGE, EnergyFormat(value_chr, voltage_chr, json, Energy.voltage_common));
      WSContentSend_PD(HTTP_SNS_CURRENT, EnergyFormat(value_chr, current_chr, json));
      WSContentSend_PD(HTTP_SNS_POWER, EnergyFormat(value_chr, active_power_chr, json));
      WSContentSend_PD(HTTP_ENERGY_SNS1, EnergyFormat(value_chr, apparent_power_chr, json),
                                         EnergyFormat(value2_chr, reactive_power_chr, json),
                                         EnergyFormat(value3_chr, power_factor_chr, json));
      WSContentSend_PD(HTTP_SNS_FREQUENCY, frequency_chr);
    }
#endif // USE_WEBSERVER
  }
}

void AtmDrvInit(void) {
  //esp8266 template doesn't say hardware SPI?
  //if (PinUsed(GPIO_SPI_CS) && PinUsed(GPIO_SPI_MOSI) && PinUsed(GPIO_SPI_CLK)) {
  //possibly should add own pin definitions so unique
  if (PinUsed(GPIO_SSPI_CS) && PinUsed(GPIO_SSPI_MOSI) && PinUsed(GPIO_SSPI_MISO) && PinUsed(GPIO_SSPI_SCLK)) {
    //SSPI_CS isn't a flag, if 0 then can assume that using secondary CS (and first is always GPIO15?
    //if Pin(GPIO_SSPI_CS) == 0 use_second_device = true?
    AddLog_P2(LOG_LEVEL_INFO, PSTR("ATM90E26 device init %d"), Pin(GPIO_SSPI_CS));
    energy_flg = XNRG_15;
  }
}

//Currently update sensors every 200ms, thats the finest resoltuon we can get is actively querying
void AtmEvery200ms(void) {


  // Yes many of these these can be calculated, i'm lazy, measure all the things
  // could (ab)use the three phases but not sure if may confuse consumers of data so won't
  // also not scalable above 3 channels
  Energy.active_power[0] = eic1->GetActivePower();
  Energy.voltage[0] = eic1->GetLineVoltage();
  Energy.current[0] = eic1->GetLineCurrent();
  Energy.apparent_power[0] = eic1->GetApparentPower();
  Energy.reactive_power[0] = eic1->GetReactivePower();
  Energy.power_factor[0] = eic1->GetPowerFactor();
  Energy.frequency[0] = eic1->GetFrequency();

  atm90e26.active_power[0] = eic2->GetActivePower();
  atm90e26.voltage[0] = eic2->GetLineVoltage();
  atm90e26.current[0] = eic2->GetLineCurrent();
  atm90e26.apparent_power[0] = eic2->GetApparentPower();
  atm90e26.reactive_power[0] = eic2->GetReactivePower();
  atm90e26.power_factor[0] = eic2->GetPowerFactor();
  atm90e26.frequency[0] = eic2->GetFrequency();
  // DNE
  // Energy.phase_angle[0] = eic1->GetPhaseAngle();

  // Need to duplicate function if want total for channel#n
  EnergyUpdateTotal(Energy.active_power[0], false);
}

void AtmEverySecond(void) {
}

// What commands do we want to be able to send?
// Calibration at least
bool AtmCommand(void) {
  return true;
}

bool Xnrg15(uint8_t function) {
  bool result = false;
  switch (function) {
  // If/when have totals for other channels
  // case FUNC_ENERGY_RESET:
  // xdrv_03.energy.ino only calls per 200ms, 250ms and then every second
  //(50, 100, 200, 250)
  case FUNC_EVERY_200_MSECOND:
    AtmEvery200ms();
    break;
  case FUNC_ENERGY_EVERY_SECOND:
    AtmEverySecond();
    break;
  case FUNC_JSON_APPEND:
    AtmShow(true);
    break;
  case FUNC_WEB_SENSOR:
    AtmShow(false);
    break;
  case FUNC_COMMAND:
    result = AtmCommand();
    break;
  case FUNC_INIT:
    AtmSnsInit();
    break;
  case FUNC_PRE_INIT:
    AtmDrvInit();
    break;
  }
  return result;
}

#endif // USE_ATM90E26
#endif // USE_ENERGY_SENSOR
#endif // USE_SPI
