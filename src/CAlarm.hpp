#include <Arduino.h>
#include "CTimer.hpp"

struct _S_ALARM_LINE
{
  unsigned int nFreq;
  unsigned int nPeriod;
};

_S_ALARM_LINE aAlarm[6] =
{
  { 1000, 500 },
  { 150, 500 },
  { 1000, 500 },
  { 150, 500 },
  { 1000, 500 },
  { 150, 500 },
};



class CAlarm
{
  enum _E_STATE
  {
    eStart = 0,
    eAlarm,
    eWait,
  };
public:
  CAlarm()
  :m_pTimer(new CTimer("AT", false))
  ,m_nState(eStart)
  ,m_nLine(0)
  {

  }

  bool PlayAlarm()
  {
    switch ( m_nState )
    {
      case eStart:
    #if LOG_DEBUG == 1
        Serial.print(F("Alarm start\n"));
    #endif
        m_nLine = 0;
        m_nState = eAlarm;
        return false;
      
      case eAlarm:
        tone(BUZZER_OUT, aAlarm[m_nLine].nFreq);
        m_pTimer->StartMs(aAlarm[m_nLine].nPeriod);
        m_nState = eWait;
        return false;

      case eWait:
        if ( m_pTimer->IsExpired() )
        {
          if ( ++m_nLine >= sizeof(aAlarm)/sizeof(_S_ALARM_LINE) )
          {
    #if LOG_DEBUG == 1
            Serial.print(F("Alarm done\n"));
    #endif
            noTone(BUZZER_OUT);
            m_nState = eStart;
            return true;
          }
          else
          {
            m_nState = eAlarm;
            return false;
          }
          
        }
        else
        {
          return false;
        }
        break;

      default:
        m_nState = eStart;
        return false;
    }
  }

protected:
  CTimer* m_pTimer;
  _E_STATE m_nState;
  unsigned int m_nLine;
};
