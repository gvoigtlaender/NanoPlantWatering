#include <Arduino.h>
#include <EEPROM.h>

unsigned int FSHlength(const __FlashStringHelper * FSHinput) {
  PGM_P FSHinputPointer = reinterpret_cast<PGM_P>(FSHinput);
  unsigned int stringLength = 0;
  while (pgm_read_byte(FSHinputPointer++)) {
    stringLength++;
  }
  return stringLength;
}

class CParameter {
public:
    CParameter(const __FlashStringHelper* szName, const char cUnit)
    :m_pszName(NULL)
    ,m_nEepromOffset(ms_nEEPromOffset)
    ,m_cUnit(cUnit) {
    int nLen = FSHlength(szName);
    m_pszName = new char[nLen+1];
    memcpy_P(m_pszName, szName, nLen);
    m_pszName[nLen] = 0x00;
    for ( uint8_t n=0; n<nLen; n++ ) {
        m_cCheckSum ^= m_pszName[n];
        m_cCheckSum ^= m_nEepromOffset;
    }

#if LOG_DEBUG == 1
    Serial.print(F("CParameterUInt8("));
    Serial.print(szName);
    Serial.print(F(","));
    Serial.print(FSHlength(szName));
    Serial.print(F(")"));
    Serial.print(F(" CS="));
    Serial.print((int)m_cCheckSum);
    Serial.print(F(" done\n"));
#endif
    }
    virtual void EEPROMread() = 0;
    virtual void EEPROMupdate() = 0;
    virtual void Store() = 0;
    virtual void Restore() = 0;
    virtual bool HasChanged() = 0;

    virtual void PrintValueLine(char* szTmp, size_t nSize) = 0;
    virtual void ChangeValue(int16_t value) = 0;

    virtual const char* GetName() const {
    return m_pszName;
    }

    virtual const char GetUnit() const {
    return m_cUnit;
    }

    virtual void logParameters() = 0;
    char* m_pszName;
    uint8_t m_nEepromOffset;
    char m_cUnit;

protected:
  static uint8_t ms_nEEPromOffset;
  static char m_cCheckSum;
  static CParameter** ms_vParameterList;
  static uint8_t ms_ParameterListSize;

public:
    static void _logParameters(const __FlashStringHelper* szLog) {
        Serial.print(F("logParameters("));
        Serial.print(szLog);
        Serial.print("): ");
        for ( uint8_t n=0; n<ms_ParameterListSize; n++ )
        {
            Serial.print(F("["));
            Serial.print(ms_vParameterList[n]->GetName());
            Serial.print(F("="));
            ms_vParameterList[n]->logParameters();
            Serial.print(F("]"));
        }
        Serial.print(F("\n"));
    }

    static void _AddParameter(CParameter* pParameter)
    {
        // ms_vParameterList = (CParameter**)realloc(ms_vParameterList, sizeof(CParameter*)*(ms_ParameterListSize+1));
        CParameter** pNew = (CParameter**)realloc(NULL, sizeof(CParameter*)*(ms_ParameterListSize+1));
        if ( ms_vParameterList != NULL )
        {
            memcpy(pNew, ms_vParameterList, sizeof(CParameter*)*(ms_ParameterListSize));
            free(ms_vParameterList);
        }   
        pNew[ms_ParameterListSize] = pParameter;        
        ms_vParameterList = pNew; 
        ms_ParameterListSize++;
    }

    static CParameter* _GetParameter(uint8_t nIdx)
    {
        if ( nIdx>=0 && nIdx<ms_ParameterListSize )
            return ms_vParameterList[nIdx];
        
        Serial.print(F("Assert: CParameter::_GetParameter("));
        Serial.print(nIdx);
        Serial.print(F(") out of range [0 .. "));
        Serial.print(ms_ParameterListSize);
        Serial.print(F("]\n"));

        while ( true )
        {
            delay(1000);
        }
    }

    static uint8_t _GetNoOfParams() {
        return ms_ParameterListSize;
    }

    static void _EEPROMread() {
#if LOG_DEBUG == 1
        Serial.print(F("CParameter::_EEPROMread()\n"));
#endif
        char c = EEPROM.read(0);
        if ( c != m_cCheckSum ) {
            Serial.print(F("CParameter::_EEPROMread() abort, checksum error\n"));
            EEPROM.update(0, m_cCheckSum);
            for ( uint8_t n=0; n<ms_ParameterListSize; n++ )
            {
                ms_vParameterList[n]->EEPROMupdate();
            }
            return;
        }
        for ( uint8_t n=0; n<ms_ParameterListSize; n++ )
        {
            ms_vParameterList[n]->EEPROMread();
        }
#if LOG_DEBUG == 1
        Serial.print(F("CParameter::_EEPROMread() done\n"));
#endif
    }

};
// static
uint8_t CParameter::ms_nEEPromOffset = 1;
// static
char CParameter::m_cCheckSum = 0x00;
// static
CParameter** CParameter::ms_vParameterList = NULL;
// static
uint8_t CParameter::ms_ParameterListSize = 0;

class CParameterUInt8 : public CParameter {
public:
  CParameterUInt8(const __FlashStringHelper* szName, const char cUnit, uint8_t nMin, uint8_t nMax, uint8_t nDefault)
   :CParameter(szName, cUnit)
   ,m_Value(nDefault)
   ,m_nMin(nMin)
   ,m_nMax(nMax) {
     ms_nEEPromOffset += sizeof(m_Value);
     // EEPROMread();
  }

  virtual void EEPROMread() override {
    int8_t val = EEPROM.read(m_nEepromOffset);
    if ( val <= m_nMax && val >= m_nMin )
    {
#if LOG_DEBUG == 1
      Serial.print(m_pszName);
      Serial.print(F("->EEPROMread("));
      Serial.print(m_nEepromOffset);
      Serial.print(F(")="));
      Serial.print(val);
      Serial.print(F("\n"));
#endif
      m_Value = val;
    }
  }

  virtual void EEPROMupdate() override {
#if LOG_DEBUG == 1
    Serial.print(m_pszName);
    Serial.print(F("->EEPROMupdate("));
    Serial.print(m_nEepromOffset);
    Serial.print(F(", "));
    Serial.print(m_Value);
    Serial.print(F(")\n"));
#endif
    EEPROM.update(m_nEepromOffset, m_Value);
  }

  virtual void Store() {
    ms_nStore = m_Value;
  }

  virtual void Restore() {
    m_Value = ms_nStore;
  }

  virtual bool HasChanged() {
    return (m_Value != ms_nStore);
  }

  virtual void PrintValueLine(char* szTmp, size_t nSize) {
    snprintf(szTmp, nSize, "%u %c", m_Value, m_cUnit);
  }

  virtual void ChangeValue(int16_t value) {
    if ( value != 0 ) {
      int16_t nValNew = m_Value;
#if LOG_DEBUG == 1
      Serial.print(F("Encoder.getValue()="));
      Serial.print(value);
      Serial.print(F(", Value="));
      Serial.print(nValNew);
      Serial.print(F(" -> "));
#endif
      nValNew += value;
#if LOG_DEBUG == 1
      Serial.print(nValNew);
#endif
      if ( nValNew > m_nMax )
        nValNew = m_nMax;
      if ( nValNew < m_nMin )
        nValNew = m_nMin;

      m_Value = (uint8_t)nValNew;
#if LOG_DEBUG == 1
      Serial.print(F(" -> range("));
      Serial.print(m_nMin);
      Serial.print(F(", "));
      Serial.print(m_nMax);
      Serial.print(F(") -> "));
      Serial.print(m_Value);
      Serial.print(F("\n"));
#endif
    }
  }

  void logParameters() override {
      Serial.print(m_Value);
  }

public:
  uint8_t m_Value;
  uint8_t m_nMin;
  uint8_t m_nMax;
  static uint8_t ms_nStore;
};
// static 
uint8_t CParameterUInt8::ms_nStore = 0;

class CParameterUInt8_RefInt : public CParameterUInt8
{
public:
  CParameterUInt8_RefInt(const __FlashStringHelper* szName, const char cUnit, uint8_t nMin, uint8_t nMax, uint8_t nDefault, int& rRef, int nDefaultRef)
  : CParameterUInt8(szName, cUnit, nMin, nMax, nDefault)
  , m_pRaw(&rRef)
  ,m_Raw(nDefault) {
      ms_nEEPromOffset += sizeof(int);
  }

  virtual void EEPROMread() override {
    int8_t val = EEPROM.read(m_nEepromOffset);
    int ref = EEPROM.read(m_nEepromOffset + 1)*256 + EEPROM.read(m_nEepromOffset + 2);

    if ( val <= m_nMax && val >= m_nMin )
    {
#if LOG_DEBUG == 1
      Serial.print(m_pszName);
      Serial.print(F("->EEPROMread("));
      Serial.print(m_nEepromOffset);
      Serial.print(F(")="));
      Serial.print(val);
      Serial.print(F(":"));
      Serial.print(ref);
      Serial.print(F("\n"));
#endif
      m_Value = val;
      m_Raw = ref;
    }
  }

  virtual void EEPROMupdate() override {

    m_Raw = *m_pRaw;
// #if LOG_DEBUG == 1
    Serial.print(m_pszName);
    Serial.print(F("->EEPROMupdate("));
    Serial.print(m_nEepromOffset);
    Serial.print(F(", "));
    Serial.print(m_Value);
    Serial.print(F(":"));
    Serial.print(m_Raw);
    Serial.print(F(")\n"));
// #endif
    EEPROM.update(m_nEepromOffset, m_Value);
    EEPROM.update(m_nEepromOffset+1, m_Raw/256);
    EEPROM.update(m_nEepromOffset+2, m_Raw%256);
  }

    void logParameters() override {
        CParameterUInt8::logParameters();
        Serial.print(F(":"));
        Serial.print(m_Raw);
  }


public:
    int* m_pRaw;
    int m_Raw;
};