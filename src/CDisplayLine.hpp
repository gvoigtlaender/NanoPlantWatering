#include <U8x8lib.h>

class CDisplayLine  {
public:

  CDisplayLine(U8X8& rU8X8, uint8_t nY, const uint8_t *font_8x8, uint8_t nLines = 1, const char* szStr = NULL)
  : m_pU8X8(&rU8X8)
  , m_nY(nY)
  , m_font_8x8(font_8x8)
  , m_nLines(nLines)
  /*, m_sText("")*/  {
    #if LOG_DEBUG == 1
    Serial.print(F("CDisplayLine()\n"));
    #endif
    delay(100);
    strncpy(m_szText, "xx", sizeof(m_szText)); 
    if ( szStr != NULL )
      drawString(szStr, 0);
    else
      drawString("---", 0);
  }

  CDisplayLine(U8X8& rU8X8, uint8_t nY, const uint8_t *font_8x8, uint8_t nLines = 1, const __FlashStringHelper* szStr = NULL)
  : m_pU8X8(&rU8X8)
  , m_nY(nY)
  , m_font_8x8(font_8x8)
  , m_nLines(nLines)
  /*, m_sText("")*/  {
    #if LOG_DEBUG == 1
    Serial.print(F("CDisplayLine()\n"));
    #endif
    delay(100);
    strncpy(m_szText, "xx", sizeof(m_szText)); 
    if ( szStr != NULL )
      drawString(szStr, 0);
    else
      drawString("---", 0);
  }

  void drawString(const char* szStr, uint8_t nX = 0)  {
    if ( strncmp(szStr, m_szText, sizeof(m_szText)) == 0 )
     return;
    #if LOG_DEBUG == 1
    Serial.print(F("drawString("));
    Serial.print(m_szText);
    Serial.print(F(" -> "));
    Serial.print(szStr);
    Serial.print(F(")\n"));
    #endif

    m_pU8X8->setFont(m_font_8x8);
    if ( strlen(szStr) != strlen(m_szText) )  {
      for ( uint8_t nl=0; nl<m_nLines; nl++ )
        m_pU8X8->clearLine(m_nY+nl);
    }
    m_pU8X8->drawString(nX,m_nY,szStr);
    strncpy(m_szText, szStr, sizeof(m_szText));
  }

  void drawString(const __FlashStringHelper* szStr, uint8_t nX = 0)  {
    char m_szTmp[24];
    memcpy_P(m_szTmp, szStr, sizeof(m_szTmp));

    if ( strncmp(m_szTmp, m_szText, sizeof(m_szText)) == 0 )
     return;

    #if LOG_DEBUG == 1
    Serial.print(F("drawString("));
    Serial.print(m_szText);
    Serial.print(F(" -> "));
    Serial.print(m_szTmp);
    Serial.print(F(")\n"));
    #endif

    m_pU8X8->setFont(m_font_8x8);
    if ( strlen(m_szTmp) != strlen(m_szText) )  {
      for ( uint8_t nl=0; nl<m_nLines; nl++ )
        m_pU8X8->clearLine(m_nY+nl);
    }
    strncpy(m_szText, m_szTmp, sizeof(m_szText));
    m_pU8X8->drawString(nX,m_nY,m_szText);
  }

protected:
  U8X8* m_pU8X8;
  uint8_t m_nY;
  const uint8_t *m_font_8x8;
  uint8_t m_nLines;
  char m_szText[24];
};
