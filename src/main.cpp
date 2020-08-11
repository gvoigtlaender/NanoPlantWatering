#include <Arduino.h>

#define VERSION "0.1.0.0"

#define HUMIDITY_IN A0
#define DISPLAY_SDA A4
#define DISPLAY_SCL A5

#define HWREV 2

#if HWREV == 1
#define PUMP_OUT PD6
#define BUZZER_OUT 13
#define WS2812_OUT 12
#define ALARM_IN 8
#else
#define PUMP_OUT 17
#define BUZZER_OUT 12
#define WS2812_OUT 13
#define ALARM_IN 15
#endif

#define BTN_MENU 4
#define BTN_PLUS 3
#define BTN_MINUS 2

// #define PUMP_INV 0

#include <U8x8lib.h>
#include <SPI.h>

#include "CTimer.hpp"
#include "CDisplayLine.hpp"
#include "CParameter.hpp"
#include "CAlarm.hpp"
#include "CButtonControl.hpp"

#include <FastLED.h>
CRGB ws2812;

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ 255, /* clock=*/ SCL, /* data=*/ SDA);

CAlarm * pAlarm = NULL;

CDisplayLine* pL1 = NULL;
CDisplayLine* pL2 = NULL;
CDisplayLine* pL3 = NULL;
CDisplayLine* pL4 = NULL;

int nParamIdx=0;

CParameter* pParameter = NULL;
CParameterUInt8* pPollingMin = NULL;
CParameterUInt8* pPumpingSec = NULL;
CParameterUInt8* pThr = NULL;
CParameterUInt8* pAlarmIn = NULL;
CParameterUInt8_RefInt* pRef1 = NULL;
CParameterUInt8_RefInt* pRef2 = NULL;



CButtonControl* pButtonControl = NULL;;

enum eWaterSensorType { eWS_EmptyHigh = 0, eWS_EmptyLow };

int nHumiditySensorP = 0;
int nHumiditySensorRaw = 0;

void switchPump(bool bOn)
{
  if ( bOn )
    digitalWrite(PUMP_OUT, HIGH);
  else
    digitalWrite(PUMP_OUT, LOW);
  Serial.print(F("switchPump("));
  Serial.print(bOn);
  Serial.print(F(")\n"));
}

void setup() {
  CTimer::Loop_CTimer();
  Serial.begin(57600);
  Serial.print(F("PLANT WATERING\n"));
  Serial.print(F("VERSION: "));
  Serial.print(F(VERSION));
  Serial.print(F("\n"));
#if LOG_DEBUG == 1
  Serial.print(F("setup()\n"));
#endif
  // put your setup code here, to run once:
  pinMode(PUMP_OUT, OUTPUT);
  pinMode(ALARM_IN, INPUT_PULLUP);
  pinMode(BUZZER_OUT, OUTPUT);

  switchPump(false);
  digitalWrite(BUZZER_OUT, LOW);

  u8x8.begin();
  u8x8.setPowerSave(0);

  u8x8.setContrast(0);

  pL1 = new CDisplayLine(u8x8, 0, u8x8_font_7x14B_1x2_r, 2, F("[PLANT WATERING]"));
  pL2 = new CDisplayLine(u8x8, 2, u8x8_font_7x14_1x2_r,  2, F("VERSION"));
  pL3 = new CDisplayLine(u8x8, 4, u8x8_font_7x14_1x2_r,  2, F(VERSION));
  pL4 = new CDisplayLine(u8x8, 6, u8x8_font_7x14_1x2_r,  2, F(""));
#if LOG_DEBUG == 1
  Serial.print(F("setup: display lines intialized\n"));
#endif

#if LOG_DEBUG == 1
  char cP = 'p';
#else
  char cP = '%';
#endif

  CParameter::_AddParameter(pPollingMin = new CParameterUInt8(F("Polling"), 'm', 1, 99, 20));
  CParameter::_AddParameter(pPumpingSec = new CParameterUInt8(F("Pumping"), 's', 1, 30, 10));
  CParameter::_AddParameter(pThr = new CParameterUInt8(F("HumThr"), cP, 1, 100, 65));
  CParameter::_AddParameter(pAlarmIn = new CParameterUInt8(F("AlarmIn"), ' ', 0, 1, 1));
  CParameter::_AddParameter(pRef1 = new CParameterUInt8_RefInt(F("Ref1"), cP, 0, 100, 40, nHumiditySensorRaw, 100));
  CParameter::_AddParameter(pRef2 = new CParameterUInt8_RefInt(F("Ref2"), cP, 0, 100, 100, nHumiditySensorRaw, 800));

  CParameter::_EEPROMread();

#if LOG_DEBUG == 1
  Serial.print(F("setup: parameters intialized\n"));
#endif
  pParameter = CParameter::_GetParameter(nParamIdx);
  CParameter::_logParameters(F("initialization"));
#if LOG_DEBUG == 1
  Serial.print(F("1st param to show: "));
  Serial.print(pParameter->GetName());
  Serial.print(F("\n"));
#endif

  pAlarm = new CAlarm();

  pButtonControl = new CButtonControl(BTN_MENU, BTN_PLUS, BTN_MINUS);

  FastLED.addLeds<NEOPIXEL,WS2812_OUT>(&ws2812, 1);
  FastLED.setBrightness(2);
  ws2812 = CRGB::Yellow;
  FastLED.show();

#if LOG_DEBUG == 1
  Serial.print(F("setup() done\n"));
#endif
}

static char szTmp[32];
CTimer tWatering = CTimer("WT", false);
static bool bPumpOn = false;
static bool bAlarm = false;
static uint8_t nThr = 0;
void Loop_Watering()  {
  enum _E_StatesWatering
  {
    eWStart = 0,
    eWCheck,
    eWWait,
    eWDelay
  };

  static _E_StatesWatering nStateWatering = eWStart;

  switch ( nStateWatering )
  {
    case eWStart:
        tWatering.Start(3);
        nStateWatering = eWDelay;
        break;

      case eWCheck:
        CParameter::_logParameters(F("Watering::WCheck"));
        Serial.print(F("Watering: WaterLevel: "));
        Serial.print(bAlarm ? "LOW" : "OK");
        Serial.print(F(", HumRaw: "));
        Serial.print(nHumiditySensorRaw);
        Serial.print(F(", Humidity: "));
        Serial.print(nHumiditySensorP);
        
        nThr = pThr->m_Value;
        if ( nHumiditySensorP <= nThr ) {
            Serial.print(F(" <= "));
            Serial.print(pThr->m_Value);
          if ( !bAlarm )  {
            Serial.print(F("-> Pump ON\n"));
            switchPump(true);
            bPumpOn = true;
          } else {
            Serial.print(F(" -> WATER EMPTY ALARM\n"));
            switchPump(false);
            tWatering.Start(pPollingMin->m_Value*60);
            nStateWatering = eWDelay;
            break;
          }
            tWatering.Start(pPumpingSec->m_Value);
            nStateWatering = eWWait;
        } else {
          Serial.print(F(" > "));
          Serial.print(pThr->m_Value);
          Serial.print(F(" -> Sleep\n"));
          tWatering.Start(pPollingMin->m_Value *60);
          nStateWatering = eWDelay;
        }
        break;

      case eWWait:
        if ( tWatering.IsExpired() )
        {
          Serial.print(F("Watering: Pump Off -> Sleep\n"));
          switchPump(false);
          bPumpOn = false;
          tWatering.Start(pPollingMin->m_Value*60-pPumpingSec->m_Value);
          nStateWatering = eWDelay;
        }
        else if ( bAlarm )
        {
          Serial.print(F("Watering: ALARM Pump Off -> Sleep\n"));
          switchPump(false);
          bPumpOn = false;
          tWatering.Start(pPollingMin->m_Value*60-pPumpingSec->m_Value);
          nStateWatering = eWDelay;
        }
        break;

      case eWDelay:
        if ( tWatering.IsExpired() )
        {
          Serial.print(F("Watering: Delay -> Check\n"));
          nStateWatering = eWCheck;
        }
        break;
  }

}

void Loop_Display()  {
  static CTimer tDisplayTimer = CTimer("DT", false);
  enum _E_StatesDisplay
  {
    eDStart = 0,
    eDUpdate,
    eDWait,
  };

  static _E_StatesDisplay nStateDisplay = eDStart;
  const char cPumpOn[4] = { '|', '/', '-', '\\' };
  const char* szTTR = NULL;
  static int nCycle = 0;
  switch ( nStateDisplay )
  {
    case eDStart:
      nStateDisplay = eDUpdate;
      break;

    case eDUpdate:

      snprintf(szTmp, sizeof(szTmp), "H:%d P[%c]", nHumiditySensorP, bAlarm?'A':bPumpOn?cPumpOn[++nCycle%4]:'-');
      while ( strlen(szTmp) < 11 )  {
        strcat(szTmp, " ");
      }
      szTTR = tWatering.GetTimeToRunString();
      strcat(szTmp, szTTR);
      pL2->drawString(szTmp);
      tDisplayTimer.StartMs(250);
      nStateDisplay = eDWait;
      break;

    case eDWait:
        if ( tDisplayTimer.IsExpired() )
        {
          nStateDisplay = eDUpdate;
        }
        break;
  }
}

void Loop_Menu()  {
  enum _E_StatesMenu
  {
    eMStart = 0,
    eMShow,
    eMShowIdle,
    eMEdit,
    eMSave
  };
  static _E_StatesMenu nStateMenu = eMStart;

  int16_t value = 0;
  static int16_t value_prev = 0;
  CButtonControl::Button b = CButtonControl::Open;
  static char cSave = 'y';
  switch ( nStateMenu )
  {
    case eMStart:
      nStateMenu = eMShow;
      break;

    case eMShow:
#if LOG_MENU == 1
      CParameter::_logParameters(F("Menu::MShow"));
      Serial.print(F("Menu.Show -> Idle: "));
      Serial.print(pParameter->GetName());
      Serial.print("\n");
#endif
      snprintf(szTmp, sizeof(szTmp), "%s", pParameter->GetName());
      pL3->drawString(szTmp);
      pParameter->PrintValueLine(szTmp, sizeof(szTmp));
      pL4->drawString(szTmp);
      nStateMenu = eMShowIdle;
      break;

    case eMShowIdle:
      b = pButtonControl->getButton();
      if ( b == CButtonControl::Clicked )
      {
#if LOG_MENU == 1
        Serial.print(F("Menu.Idle -> clicked -> Edit: "));
        Serial.print(pParameter->GetName());
        Serial.print("\n");
#endif
        snprintf(szTmp, sizeof(szTmp), "%s *", pParameter->GetName());
        pL3->drawString(szTmp);
        pButtonControl->EditMode(true);
        pParameter->Store();
        nStateMenu = eMEdit;
        break;
      }
      value = pButtonControl->getValue();
      if ( value > 0 )  {
         nParamIdx++;
         if ( nParamIdx == CParameter::_GetNoOfParams() )
          nParamIdx=0;
      } else if ( value < 0 ) {
        nParamIdx--;
        if ( nParamIdx < 0 )
          nParamIdx = CParameter::_GetNoOfParams()-1;
      }

      if ( value != value_prev )
      {
#if LOG_MENU == 1
        Serial.print(F("value="));
        Serial.print(value);
        Serial.print(F(", pIdx="));
        Serial.print(nParamIdx);
#endif
      }
      if ( value == 0 )
      {
        if ( value != value_prev )
        {
#if LOG_MENU == 1
          Serial.print(F("\n"));
#endif
        }
        value_prev = value;
        break;
      }
      CParameter::_logParameters(F("Menu::Select"));
      pParameter = CParameter::_GetParameter(nParamIdx);
#if LOG_MENU == 1
      Serial.print(F(" => "));
      Serial.print(pParameter->GetName());
      Serial.print(F("\n"));
#endif
      nStateMenu = eMShow;
      break;

    case eMEdit:
      b = pButtonControl->getButton();
      if ( b == CButtonControl::Clicked )
      {
#if LOG_MENU == 1
        Serial.print(F("Menu.Edit -> clicked -> Save\n"));
#endif
        cSave = 'y';
        nStateMenu = eMSave;
        snprintf(szTmp, sizeof(szTmp), "Save: %c", cSave);
        pL4->drawString(szTmp);
        pButtonControl->EditMode(false);
        break;
      }
      value = pButtonControl->getValue();
      if ( value != 0 )
      {
        pParameter->ChangeValue(value);
        pParameter->PrintValueLine(szTmp, sizeof(szTmp));
        pL4->drawString(szTmp);

      }
      break;

    case eMSave:
      if ( !pParameter->HasChanged() ) {
#if LOG_MENU == 1
        Serial.print(F("Menu.Save -> unchanged -> Show\n"));
#endif
        nStateMenu = eMShow;
        break;
      }

      b = pButtonControl->getButton();
      if ( b == CButtonControl::Clicked )
      {
        if ( cSave=='y' )
        {
          CParameter::_logParameters(F("Menu::Save y"));
#if LOG_MENU == 1
          Serial.print(F("Menu.Save -> clicked -> y -> save\n"));
#endif
          pParameter->EEPROMupdate();
        }
        else
        {
          CParameter::_logParameters(F("Menu::Save n"));
#if LOG_MENU == 1
          Serial.print(F("Menu.Save -> clicked -> n -> restore\n"));
#endif
          pParameter->Restore();
        }
        nStateMenu = eMShow;
        break;
      }

      value = pButtonControl->getValue();
      if ( value != 0 )
      {
        cSave = (cSave=='y') ? 'n' : 'y';
#if LOG_MENU == 1
        Serial.print(F("Menu.Save -> "));
        Serial.print(cSave);
        Serial.print(F("\n"));
#endif
        snprintf(szTmp, sizeof(szTmp), "Save: %c", cSave);
        pL4->drawString(szTmp);
      }

      break;
  }

}

float Tranform_Linear_X_to_Y(float x, float x1, float y1, float x2, float y2)
{
	if ((x2 - x1) != 0)
  {
    float d1 = x - x1;
    float d2 = y2 - y1;
    float d3 = x2 - x1;
    return ((d1)*(d2))/(d3) + y1;
  }  
  else
    return 0.0;
}

void Loop_HumiditySensor()
{
  static CTimer tHumiditySensor("HS", false);
  const int nPeriod = 10;
  const int nDelay = 1000;

  if ( tHumiditySensor.IsExpired() )
  {
      nHumiditySensorRaw = analogRead(HUMIDITY_IN);

      float dRaw = nHumiditySensorRaw;
      float d = Tranform_Linear_X_to_Y(dRaw, pRef1->m_Raw, pRef1->m_Value, pRef2->m_Raw, pRef2->m_Value);

      static float dHumidity = d;
      dHumidity = dHumidity*(nPeriod-1)/nPeriod + d/nPeriod;
      nHumiditySensorP = dHumidity;

      Serial.print("Raw=");
      Serial.print(dRaw);
      Serial.print(", Hum:");
      Serial.print(d);
      Serial.print(", Avg:");
      Serial.print(nHumiditySensorP);
      Serial.print("\n");

      tHumiditySensor.StartMs(nDelay);
  }
}

static CTimer tWs2812("WS", false);
void Loop_Ws2812()
{
  static uint8_t nToggle = 0;

  if ( tWs2812.IsExpired() )
  {
    if ( bAlarm )
      ws2812 = (nToggle%2 == 0 ) ? CRGB::Red : CRGB::Black;
    else if ( bPumpOn )
      ws2812 = CRGB::Green;
    else
      ws2812 = CRGB::Blue;
    nToggle++;
    FastLED.show();
    tWs2812.StartMs(500);
  }
}

void Loop_AlarmSensor()
{
  static CTimer tAlarmSensor("AS", false);
  static bool bAlarm0 = false;
  if ( tAlarmSensor.IsExpired() )
  {
    if ( pAlarmIn->m_Value == eWS_EmptyHigh )
      bAlarm = digitalRead(ALARM_IN);
    else if ( pAlarmIn->m_Value == eWS_EmptyLow )
      bAlarm = !digitalRead(ALARM_IN);
    if ( bAlarm && !bAlarm0 )
      tWs2812.Start();
    bAlarm0 = bAlarm;
    tAlarmSensor.StartMs(5);
  }
}

void Loop_AlarmBuzzer()
{
  static CTimer tTimer("AB", false);

  enum _E_States
  {
    eStart = 0,
    ePlay,
    eDelay
  };

  static _E_States eState = eStart;

  switch ( eState )
  {
    case eStart:
      if ( bAlarm )
        eState = ePlay;
      break;
    
    case ePlay:
      if ( pAlarm->PlayAlarm() == true )
      {
        tTimer.Start(5);
        eState = eDelay;
      }
      break;

    case eDelay:
      if ( tTimer.IsExpired() )
      {
        eState = eStart;
      }
      break;
  }
}

void loop()  {
  // put your main code here, to run repeatedly:
  CTimer::Loop_CTimer();

  Loop_HumiditySensor();

  pButtonControl->Loop();

  Loop_Watering();

  Loop_Display();

  Loop_Menu();

  Loop_AlarmSensor();

  Loop_Ws2812();

  Loop_AlarmBuzzer();
}
