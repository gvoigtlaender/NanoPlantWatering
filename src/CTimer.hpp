#include <Arduino.h>

#ifndef _CALARM_HPP_
#define _CALARM_HPP_
class CTimer  {
public:
  CTimer(const char* szName, bool bLog = true, bool bStart=false)
  : m_nEndTime(0)
  , m_bLog(bLog) {
    strncpy(m_szName, szName, sizeof(m_szName));
    strncpy(m_szTTR, "---", sizeof(m_szTTR));
    if ( bStart )
      StartMs(0);
  }

  void Start()  {
    m_nStartTime = CTimer::ms_ulMS;
    m_nEndTime = CTimer::ms_ulMS;
    strncpy(m_szTTR, "---", sizeof(m_szTTR));
    #if LOG_DEBUG == 1
    if ( m_bLog )
    {
      Serial.print(m_szName);
      Serial.print(F("->Start()\n"));
    }
    #endif
  }

  void Start(unsigned long nDelayS)  {
    m_nStartTime = CTimer::ms_ulMS;
    m_nEndTime = CTimer::ms_ulMS + nDelayS*1000;
    strncpy(m_szTTR, "---", sizeof(m_szTTR));
    #if LOG_DEBUG == 1
    if ( m_bLog )
    {
      Serial.print(m_szName);
      Serial.print(F("->Start("));
      Serial.print(nDelayS);
      Serial.print(F("s) => "));
      Serial.print(GetTimeToRun()/1000.0);
      Serial.print(F("s) => "));
      Serial.print(GetTimeToRunString());
      Serial.print(F("\n"));
    }
    #endif
  }
  void StartMs(unsigned long nDelayMS)  {
    m_nStartTime = CTimer::ms_ulMS;
    m_nEndTime = CTimer::ms_ulMS + nDelayMS;
    strncpy(m_szTTR, "---", sizeof(m_szTTR));
    #if LOG_DEBUG == 1
    if ( m_bLog )
    {
      Serial.print(m_szName);
      Serial.print(F("->StartMs("));
      Serial.print(nDelayMS);
      Serial.print(F("ms) => "));
      Serial.print(GetTimeToRun()/1000.0);
      Serial.print(F("s) => "));
      Serial.print(GetTimeToRunString());
      Serial.print(F("\n"));
    }
    #endif
  }
  bool IsExpired()  {
    if ( m_nEndTime <= CTimer::ms_ulMS )
    {
    #if LOG_DEBUG == 1
      if ( m_bLog )
      {
        Serial.print(m_szName);
        Serial.print(F("->IsExpired() = true\n"));
      }
    #endif
      return true;
    }
    return false;
  }
  unsigned long GetTimeToRun()  {
    return ( m_nEndTime - CTimer::ms_ulMS );
  }
  const char* GetTimeToRunString()  {
    double sec = GetTimeToRun() / 1000.0;
    int nMin = sec/60;
    int nSec = sec - nMin*60;
    snprintf(m_szTTR, sizeof(m_szTTR), "%02d:%02d", nMin, nSec);

    return m_szTTR;
  }

  unsigned long GetDiffTime()
  {
    return CTimer::ms_ulMS - m_nStartTime;
  }

  static void Loop_CTimer() {
    CTimer::ms_ulMS = millis();
  }

  static unsigned long GetMillis()
  {
    return CTimer::ms_ulMS;
  }

  unsigned long m_nStartTime;
  unsigned long m_nEndTime;
  bool m_bLog;
  char m_szName[4];
  char m_szTTR[8];
  static unsigned long ms_ulMS;
};
// static 
unsigned long CTimer::ms_ulMS = 0;

#endif //_CALARM_HPP_