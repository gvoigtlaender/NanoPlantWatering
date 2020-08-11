#include <Arduino.h>
#include "CTimer.hpp"


class CButtonControl
{
public:
    typedef enum Button_e {
    Open = 0,
    Closed,
    
    Pressed,
    Held,
    Released,
    
    Clicked,
    DoubleClicked
    
  } Button;

  typedef enum Button_s {
    MENU = 1,
    PLUS = 2,
    MINUS = 4,
  } Button2;

  CButtonControl(int nMenu, int nPlus, int nMinus)
  :m_nPinMenu(nMenu)
  ,m_nPinPlus(nPlus)
  ,m_nPinMinus(nMinus)
  ,m_nStatePrev(0)
  ,m_nValue(0)
  ,m_pTimer(new CTimer("ENC"))
  ,m_Button(Open)
  ,m_bEditMode(false) {
    pinMode(m_nPinMenu, INPUT_PULLUP);
    pinMode(m_nPinPlus, INPUT_PULLUP);
    pinMode(m_nPinMinus, INPUT_PULLUP);
 
    m_nStatePrev = 0;
    if ( isActive_Menu() )
    {
      m_pTimer->Start();
      m_nStatePrev |= MENU;
    }  
    if ( isActive_Plus() )
      m_nStatePrev |= PLUS;
    if ( isActive_Minus() )
      m_nStatePrev |= MINUS;

    #if LOG_DEBUG == 1
    Serial.print(F("ClickEncoder::ClickEncoder() "));
    Serial.print(m_nStatePrev);
    Serial.print(F("\n"));
    #endif
  }

  void Loop() {
    int8_t nState = 0, nPrev = m_nStatePrev;
    if ( isActive_Menu() )
      nState |= MENU;
    if ( isActive_Plus() )
      nState |= PLUS;
    if ( isActive_Minus() )
      nState |= MINUS;

    //! menu pressed
    if ( (nPrev & MENU) == 0 && (nState & MENU) == MENU ) {
      m_pTimer->Start();
    #if LOG_DEBUG == 1
      Serial.print(F("Menu pressed\n"));
    #endif
      m_Button = Open;
    }
    //! menu released
    if ( (nPrev & MENU) == MENU && (nState & MENU) == 0 ) {
      m_Button = Clicked;
    #if LOG_DEBUG == 1
      Serial.print(F("Menu released after "));
      Serial.print(diff);
      Serial.print(F("ms\n"));
    #endif
    }

    //! plus & minus pressed


    //! plus pressed
    if ( (nState & PLUS) == PLUS && (nPrev & PLUS) == 0 ) {
      // m_nValue++;
      m_pTimer->Start();
    #if LOG_DEBUG == 1
      Serial.print(F("Plus pressed\n"));
    #endif
    }
    //! plus hold
    if ( (nState & PLUS) == PLUS && (nPrev & PLUS) == PLUS && m_bEditMode ) {
      if ( m_pTimer->GetDiffTime() > 1000 )
      {
        m_nValue += 5;
        m_pTimer->Start();
    #if LOG_DEBUG == 1
        Serial.print(F("Plus holding\n"));
    #endif
      }
    }
    //! plus released
    if ( (nPrev & PLUS) == PLUS && (nState & PLUS) == 0 ) {
      m_nValue++;
    #if LOG_DEBUG == 1
      Serial.print(F("Plus released\n"));
    #endif
    }

    //! minus pressed
    if ( (nState & MINUS) == MINUS && (nPrev & MINUS) == 0 ) {
      // m_nValue--;
      m_pTimer->Start();
    #if LOG_DEBUG == 1
      Serial.print(F("Minus pressed\n"));
    #endif
    }
    //! minus hold
    if ( (nState & MINUS) == MINUS && (nPrev & MINUS) == MINUS && m_bEditMode ) {
      if ( m_pTimer->GetDiffTime() > 1000 )
      {
        m_nValue -= 5;
        m_pTimer->Start();
    #if LOG_DEBUG == 1
        Serial.print(F("Minus holding\n"));
    #endif
      }
    }
    //! minus released
    if ( (nPrev & MINUS) == MINUS && (nState & MINUS) == 0 ) {
      m_nValue--;
    #if LOG_DEBUG == 1
      Serial.print(F("minus released\n"));
    #endif
    }
    m_nStatePrev = nState;
    
  }

  Button  getButton()
  {
    Button btn = m_Button;
    m_Button = Open;
    return btn;
  }

  int8_t getValue()
  {
    int8_t nVal = m_nValue;
    m_nValue = 0;
    return nVal;
  }

  void EditMode(bool bEnabled) {
    m_bEditMode = bEnabled;
  }

private:

  int m_nPinMenu;
  int m_nPinPlus;
  int m_nPinMinus;

  bool isActive_Menu() {
    return !digitalRead(m_nPinMenu);
  }
  bool isActive_Plus() {
    return !digitalRead(m_nPinPlus);
  }
  bool isActive_Minus() {
    return !digitalRead(m_nPinMinus);
  }
  uint8_t m_nStatePrev;
  int8_t m_nValue;
  CTimer* m_pTimer;
  Button m_Button;
  bool m_bEditMode;
};
