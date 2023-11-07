
/********************************************************************************************************
   The program reads the radio weather forecast from hamsql.com and iap-kühlungsborn and outputs it to a display
  the datas are from "hamsql.com" by OM Paul Herrman , N0NBH -> THANKS a lot for it !!
  Thanks also to "Institut für Ionophärenphysik Rostock e.V. Aussenstelle Kühlungsborn" , Herrn Jens Mielich.
  Hardware: WEMOS D1 mini and an Adafruit_ILI9341 touch-display (320x240)[2.4" or 2.8" or 3.2" displays works also]
  get WEMOS D1 mini , 3.2" TFT w. Touch ILI9341 (or 2.4" or 2.8" or 3.2") from : ebay or similar providers
  it works with LOLIN-TFT 2.4" shield also - you have it to define in line 20 - or comment it out
  program idea and realization by Dr. Peter Heblik , DD6USB , version 2.5 from 5/1/2022
  "OTA"  means: the D1 mini can be programmed via WLAN - it does not have to be connected to a computer.
  how to program: in Arduino-IDE you find -> "Tools" -> "Ports" the Adress from "D1 mini for Solardat" and his local IP.
  Select it and download - thats all folks ! But attention! in OTA-mode the serial monitor does NOT work !!!
********************************************************************************************************/

String V_String = "ver 2.5ext edited 5/1/2022 by DD6USB" ;

//#define debug           //  scratch out if not used (dont work by OTA)
//#define debug1          //  deep debug,scratch out if not used (dont work by OTA)
#define WiFiOn            // to switch WiFi & WWW on/off (for faster Tests)
#define LOLIN_TFT_SHIED  //  if using LOLIN TFT-2.4" shield - otherwise uncomment it

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <XPT2046_Touchscreen.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

WiFiClientSecure client ;

#include "Adafruit_GFX.h"          // screen graphics
#include "Adafruit_ILI9341.h"      // hardware-spi
#include <gfxfont.h>

//!!!!!write this in program direct!!!!!!
//char* ssid = "WLAN-SSID-name...." ; // your SSID (for instance: "Area51")
//char* passw = "passw...1234567.." ; // your password (for instance "11223344")

//!!!!!!change your own path to these libraries !!!!!
#include <disk:\path\FreeMono9pt7b.h>
#include <disk:\path\FreeSans12pt7b.h>
#include <disk:\path\FreeSans9pt7b.h>

#ifdef LOLIN_TFT_SHIED
#define TFT_DC     D8
#define TFT_CS     D0
#define TS_CS      D3
#define TFT_RST    -1
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TS_CS);
#else
// Pins for the ILI9341 and ESP8266 (D1 mini)
#define TFT_DC    D2
#define TFT_CS    D1
#define TFT_LED   D8
#define TOUCH_CS  D3
#define TOUCH_IRQ D4
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC) ; // instance for the tft-screen
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ) ;   // instance for touch-screen
#endif


// all my known colors for ILI9341 TFT (but not all used in program)
#define B_DD6USB 0x0006    //   0,   0,   4  my preferred background color !!!
#define BLACK 0x0000       //   0,   0,   0
#define NAVY 0x000F        //   0,   0, 123
#define DARKGREEN 0x03E0   //   0, 125,   0
#define DARKCYAN 0x03EF    //   0, 125, 123
#define MAROON 0x7800      // 123,   0,   0
#define PURPLE 0x780F      // 123,   0, 123
#define OLIVE 0x7BE0       // 123, 125,   0
#define LIGHTGREY 0xC618   // 198, 195, 198
#define DARKGREY 0x7BEF    // 123, 125, 123
#define BLUE 0x001F        //   0,   0, 255
#define GREEN 0x07E0       //   0, 255,   0
#define CYAN 0x07FF        //   0, 255, 255
#define RED 0xF800         // 255,   0,   0
#define MAGENTA 0xF81F     // 255,   0, 255
#define YELLOW 0xFFE0      // 255, 255,   0
#define WHITE 0xFFFF       // 255, 255, 255
#define ORANGE 0xFD20      // 255, 165,   0
#define GREENYELLOW 0xAFE5 // 173, 255,  41
#define PINK 0xFC18        // 255, 130, 198

#define HOST "hamqsl.com"             // source page in www
#define HOST1 "www.ionosonde.iap-kborn.de" // another source page in www

#define max_page 3                    // only 4 pages, no more (at this time)[p0->p3]


//============>>   here the global variables: <<=============

char atom ;                           // single char of content
int8_t page_nr , old_page_nr ;        // which page I am ?
uint16_t color , x , k , pos , pos1 ; // for band-field-colors and string-positions
uint16_t client_bytes = 1700 ;        // max. bytes to read from content
uint64_t start_time ;                 // for time_loop
uint64_t loop_time = 900000 ;         // 900.000 milli-secs = 15 min
bool is_basic_pattern ;               // flag for page0-pattern
bool flag_color = false ;             // flag for color change or not
String pbuf, old_pbuf, t, t2 ;        // to hold the old and new compressed content
String old_iap , new_iap , iap_buf ;  // hold the iap-datas for comparison
String content_strings3[10] ;         // hold the iap-datas

//***************************************************************************
void setup(void)
{
  tft.begin();                  // initialize the screen
#ifndef LOLIN_TFT_SHIED
  pinMode(TFT_LED, OUTPUT) ;    // Pin for Backlight
  digitalWrite(TFT_LED, HIGH) ; // switch Backlight on
#endif
  tft.setRotation(0) ;          // orientation of the screen
  Serial.begin(115200) ;        // initialize the serial interface
  tft.fillScreen(BLACK) ;       // clear the screen
#ifdef WiFiOn
  WiFi.mode(WIFI_STA) ;         // for connect with WLAN
  WiFi.disconnect() ;           // if an old connection exists
  delay(100) ;
#endif
#ifdef debug
  Serial.print("==> trying connecting Wifi: ") ;
  Serial.println(ssid) ;
#endif
  WiFi.begin(ssid, passw) ;  // connect to WLAN now
  while (WiFi.status() != WL_CONNECTED)
  {
#ifdef debug
    Serial.print(".") ;
#endif
    delay(250) ;
  }
#ifdef debug
  Serial.print("\n  WiFi connected ; ") ;
  Serial.print("my IP address: ") ;
  Serial.println(WiFi.localIP()) ; // show the IP in local WLAN
#endif
  client.setInsecure() ;
  handle_OTA() ;                 // initialize OTA
#ifdef debug
  Serial.println(" * Initializing touch screen now...") ;
#endif
  ts.begin() ;                  // initialize the touch-screen
  ts.setRotation(2) ;           // set orientation for touch-screen
  is_basic_pattern = false ;    // flag for screen 0
  page_nr = 0 ;                 // start-page
  handle_pages() ;              // go a "first round"
  start_time = millis() ;       // start timeloop
}
//***************************************************************
void loop(void)
{
  ArduinoOTA.handle();                           // occur new OTA programming ?
  if (ts.touched())                              // new page wanted ?
  {
    page_nr == max_page ? page_nr = 0 : page_nr++ ; // rolling pages
    handle_pages() ;
  }
  if (time_loop_finished())
  {
    if ((new_iap != old_iap) || !check_N0NBH_datas())
    {
#ifdef debug
      Serial.println("old_iap = " + old_iap) ;
      Serial.println("new_iap = " + new_iap) ;
      Serial.println(check_N0NBH_datas()) ;
#endif
      old_iap = new_iap ;
      handle_pages() ;
    }
    start_time = millis() ;
  }
}
//**************************************************************
void handle_pages(void)
{
  old_page_nr != page_nr ? old_page_nr = page_nr : page_nr ;
  switch (page_nr)         // good for handling more pages (maybe in future)
  {
    case 0 :
      {
        handle_page0() ;  // work page 0
        break ;
      }
    case 1 :
      {
        handle_page1() ;  // work page 1
        break ;
      }
    case 2 :
      {
        handle_page2() ;  // work page 2
        break ;
      }
    case 3 :
      {
        handle_page3() ;  // work page 3
        break ;
      }
    default: break ;
  }
}
//***************************************************************
void handle_page0(void)            // side of the band openings
{
  if (!is_basic_pattern)
  {
    draw_basic_pattern() ;              // draw the basic pattern
  }
#ifdef WiFiOn
  get_and_compress_all_client_data() ; // read the main content of the webpage
  get_and_show_bottombar_datas() ;     // create the bottom-bar
  get_and_show_topbar_datas() ;        // create the top-bar
  get_and_show_band_colors() ;         // show band colors
#endif
}
//**************************************************************
void handle_page2(void)   // first side of the solar datas
{
  char * content_strings1[] = { "<solarflux>", "<aindex>", "<kindex>",
                                "<kindexnt>", "<xray>", "<sunspots>",
                                "<heliumline>", "<protonflux>", "<electonflux>",
                                "<aurora>", "<normalization>", "<latdegree>"
                              } ; // names of content (part 1)
  flag_color = true ;             // flag for changing colors
  tft.setFont() ;
  tft.fillScreen(B_DD6USB + 7);   // background for the page
  is_basic_pattern = false ;      // flag for page0
  tft.fillRoundRect(0, 0, 240, 20, 5, MAROON) ;
  tft.setCursor(180, 5) ;         // generate the top bar
  tft.setTextColor(GREEN) ;
  tft.print("tnx:N0NBH") ;
  get_and_show_topbar_datas() ;   // show top bar
  tft.setFont(&FreeSans9pt7b) ;
  tft.setCursor(0, 40) ;          // start displaying the sun values
  color = GREEN ;
  tft.setTextColor(color) ;
  for (uint8_t i = 0 ; i < 12 ; i++)   // list the content (part 1)
  {
    parse_string_and_tftprint(content_strings1[i]) ;
  }
  tft.setTextColor(WHITE) ;
  tft.setCursor(0, 307) ;
  tft.print("===>>  continued next page") ; //hint for page 2
  flag_color = false ;
}
//***************************************************************
void handle_page3(void)   // second side of the solar datas
{
  char * content_strings2[] = {"<solarwind>", "<magneticfield>", "<geomagfield>",
                               "<signalnoise>", "<fof2>", "<muffactor>", "<muf>"
                              } ;   // names of content (part 2)
  flag_color = true ;                 // flag for changing colors
  tft.setFont() ;
  tft.fillScreen(B_DD6USB + 7);        // background for the page
  is_basic_pattern = false ;           // flag for page0
  tft.fillRoundRect(0, 0, 240, 20, 5, MAROON) ;
  tft.setCursor(180, 5) ;             // generate the top bar
  tft.setTextColor(GREEN) ;
  tft.print("tnx:N0NBH") ;             // thanks PAUL !
  get_and_show_topbar_datas() ;        // show top bar
  tft.setFont(&FreeSans9pt7b) ;
  tft.setCursor(0, 43) ;               // start display of the sun values
  color = GREEN ;
  tft.setTextColor(color) ;
  for (uint8_t i = 0 ; i < 7 ; i++)    // list the content (part 2)
  {
    parse_string_and_tftprint(content_strings2[i]) ;
  }
  tft.print("\n   =====>>  EOM  <<=====") ;  // END OF MESSAGE !
  tft.setFont() ;
  tft.fillRoundRect(0, 300, 240, 20, 5, MAROON) ;
  tft.drawRoundRect(0, 300, 240, 20, 5, WHITE) ;
  tft.setTextColor(CYAN) ;
  tft.setCursor(15, 306) ;
  tft.setTextColor(WHITE) ;
  tft.print(V_String) ;           // show version-number
  flag_color = false ;
}
//**************************************************************
void handle_page1(void)      // side of the MUF-table
{
  String km_string[8] = {" 100", " 200", " 400", " 600", " 800", "1000", "1500", "3000"} ;
  uint16_t g ;
  flag_color = true ;
  new_iap = "" ;                                //state of affairs
  is_basic_pattern = false ;                    // flag for page0
  tft.fillScreen(BLACK) ;                       // clear blue screen
  tft.fillRoundRect(0, 0, 240, 16, 5, MAROON) ; // build top header
  tft.drawRoundRect(0, 0, 240, 17, 5, WHITE) ;
  tft.setTextColor(CYAN) ;
  read_iap_datas() ;                            // get the muf-table-datas
  tft.setFont() ;
  tft.setCursor(3, 5) ;
  tft.print(" " + content_strings3[10] + " UTC ") ; //show the last change-time
  tft.setTextColor(WHITE) ;
  tft.print(" tnx:iap_kborn.de") ;
  tft.fillRoundRect(3, 19, 234, 298, 5, B_DD6USB + 12) ;// background
  tft.drawRoundRect(3, 19, 234, 298, 5, YELLOW) ; // generate the borders
  tft.setCursor(10, 35) ;                         // headline position
  tft.setTextColor(GREEN) ;                       // headline color
  tft.setFont(&FreeSans9pt7b) ;                   // headline font
  tft.print("    Juliusruh  MUF-Table") ;         // generate headline
  tft.setTextColor(WHITE) ;
  tft.setCursor(0, 69) ;
  tft.println("          km                MHz") ;// table header
  tft.setFont(&FreeSans12pt7b) ;
  tft.setTextColor(YELLOW) ;
  tft.setCursor(0, 100) ;                 // startposition of table-text
  for (g = 0 ; g < 8 ; g++)               // print the values in her columns & rows
  {
    tft.print("      " + km_string[g]) ;
    tft.setCursor(150 , 100 + g * 29) ;
    if (content_strings3[7 - g].toInt() < 10) tft.print("  ") ; // arrange positions
    tft.println(content_strings3[7 - g]) ;        // print the MUF-values
  }
  tft.drawFastVLine(120, 53, 256, CYAN) ;      // build column-bar
  tft.drawRect(11, 50, 218, 262, CYAN) ;       // generate table-frame
  for (g = 77 ; g < 300 ; g += 29)             // build the table-rows
  {
    tft.drawFastHLine(12, g , 216, CYAN) ;
  }
}
//**************************************************************
void read_iap_datas(void)
{
  iap_buf = "" ;                              // clear pbuf
  if (!client.connect(HOST1, 443))            //Open iap-server Connection
  {
#ifdef debug
    Serial.println(F("Connection failed")) ;
#endif
    return ;
  } else
  {
#ifdef debug
    Serial.println(F("\thost is connected now")) ;
#endif
  }
  client.print(F("GET ")) ;                       // question to client
  client.print("/latest_JR055_MUF.txt") ;         // go to table-page
  client.println(F(" HTTP/1.1")) ;                // send question header
  client.print(F("Host: ")) ;
  client.println(HOST1) ;
  client.println(F("Cache-Control: no-cache")) ;  // no old cache please!
  if (client.println() == 0)
  {
#ifdef debug
    Serial.println(F("client error")) ;
#endif
    return ;
  }
  delay(10);
  char endOfHeaders[] = "\r\n\r\n" ;                // ignore HTTP Header
  if (!client.find(endOfHeaders))
  {
#ifdef debug
    Serial.println(F("server error")) ;
#endif
    return ;
  }
  //===============>>  now read the webpage  <<==================
  for ( uint16_t i = 0 ; i < 180 ; i++)
  {
    atom = client.read() ;
    if (atom != 0xFF)                             //"forget" the unused atoms
    {
      iap_buf = iap_buf + atom ;
    }
  }
#ifdef debug
  Serial.println("===========================================") ;
  Serial.print(iap_buf) ;
  Serial.println("===========================================") ;
#endif
  new_iap = "" ;
  for (uint8_t z = 0 ; z < 8 ; z++)            // parse iap_content
  {
    pos = iap_buf.lastIndexOf("\n") ;          // search for every newline
    t = iap_buf.substring(pos - 5 , pos) ;     // ectract substring
    iap_buf = iap_buf.substring(0, pos - 5 ) ; // build iap_substring
    t.trim() ;                                 // delete whitespace
    content_strings3[z] = t ;                  // create iap_fullstring
    new_iap = new_iap + t ;
#ifdef debug
    Serial.println("\n " + content_strings3[z]) ;
#endif
  }
  if (old_iap = "") old_iap = new_iap ;        // first initialization of old_iap
  content_strings3[10] = iap_buf.substring(16, 32) ; // len = 16 holds: date & time
  content_strings3[10].setCharAt(4, '/') ;           // change single signs
  content_strings3[10].setCharAt(7, '/') ;
  content_strings3[10].setCharAt(10, ' ') ;
  content_strings3[10].setCharAt(13, ':') ;
#ifdef debug
  Serial.println(content_strings3[10]) ;
#endif
}
//**************************************************************
void parse_string_and_tftprint(String s11)   // side of the band openings
{
  t2 = "</" + s11.substring(1, s11.length()) ;// parse content-strings
  pos  = pbuf.indexOf(s11) + s11.length() ;
  pos1 = pbuf.indexOf(t2) ;
  t2 = ("  " + s11.substring(1, s11.length() - 1) + " = ") ;
  if (t2 == "  electonflux = ")
  {
    t2 = "  electronflux = " ;
  }
  if (flag_color)                             // show on screen ?
  {
    tft.print(t2) ;
    t = pbuf.substring(pos, pos1) ;
    if ((t.length() == 0) || (t == "NoRpt"))
    {
      t = "NoReport" ;
    }
    tft.println(t) ;
    color == GREEN ? color = YELLOW : color = GREEN ;
    tft.setTextColor(color) ;          // change text color Green<=>Yellow
  }
}
//***************************************************************
bool time_loop_finished(void)
{
  if ((start_time + loop_time) > millis())
  {
    return false ;
  }
  else
  {
    return true ;
  }
}
//***************************************************************
void draw_basic_pattern(void)
{
  tft.fillScreen(B_DD6USB + 3) ; // a little more intense
  draw_WiFi_Quality() ;          // draw top bar WiFi
  draw_band_pattern() ;
}
//***************************************************************
void draw_band_pattern(void)
{
  for ( int i = 20; i < 280; i += 40)
  {
    tft.drawFastHLine(0, i, 240, YELLOW) ;
  }
  tft.drawFastVLine(180, 20, 280, YELLOW) ;
  tft.drawFastVLine(122, 20, 200, YELLOW) ;
  tft.drawFastVLine( 0, 20, 280, YELLOW) ;
  tft.drawFastVLine(239, 20, 280, YELLOW) ;
  tft.setFont(&FreeSans12pt7b) ;
  tft.setCursor(25, 48) ;
  tft.setTextColor(YELLOW) ;
  tft.print("Band") ;
  tft.setCursor(5, 87) ;
  tft.print("80m - 40m") ;
  tft.setCursor(5, 127) ;
  tft.print("30m - 20m") ;
  tft.setCursor(5, 167) ;
  tft.print("17m - 15m") ;
  tft.setCursor(5, 207) ;
  tft.print("12m - 10m") ;
  tft.setCursor(0, 247) ;
  tft.print(" E - skip 6m [EU]") ;
  tft.setCursor(0, 287) ;
  tft.print(" E - skip 4m [EU]") ;
  tft.setFont(&FreeMono9pt7b) ;
  tft.setTextColor(WHITE) ;
  tft.setCursor(133, 45) ;
  tft.print("day") ;
  tft.setTextColor(CYAN) ;
  tft.setCursor(183, 45) ;
  tft.print("night") ;
}
//***************************************************************
int8_t getWifiQuality() // converts the dBm to a range between 0 and 100%
{
  int32_t dbm = WiFi.RSSI() ; // get WiFi RSSI
  if (dbm <= -100)
  {
    return 0 ;
  } else if (dbm >= -50)
  {
    return 100;
  } else
  {
    return 2 * (dbm + 100);
  }
}
//******************************************************************
void draw_WiFi_Quality(void)
{
  tft.setFont() ;
  char buf[10] ;
  int8_t quality = getWifiQuality() ;
  tft.setTextColor(GREEN) ;
  tft.setTextSize(1) ;
  tft.setCursor(170, 5) ;
  sprintf(buf, "RSSI:%2d%c", quality, '%') ;
  tft.print(buf) ;
  for (int8_t i = 0; i < 4; i++)
  {
    for (int8_t j = 0; j < 2 * (i + 1); j++)
    {
      if (quality > i * 25 || j == 0)
      {
        tft.drawPixel(224 + 2 * i, 12 - j, YELLOW);
      }
    }
  }
}
//******************************************************************
void get_and_compress_all_client_data(void)
{
  pbuf = "" ;                      // clear pbuf
  String  sbuf = "" ;
  if (!client.connect(HOST, 443))  //Open Server Connection
  {
#ifdef debug
    Serial.println(F("Connection failed")) ;
#endif
    return ;
  } else
  {
#ifdef debug
    Serial.println(F("\thost is connected now")) ;
#endif
  }
  client.print(F("GET ")) ;                       // question to client
  client.print("/solarxml.php") ;                 // go to www.hamsql.com/solarxml.php)
  client.println(F(" HTTP/1.1")) ;                // send question header
  client.print(F("Host: ")) ;
  client.println(HOST) ;
  client.println(F("Cache-Control: no-cache")) ;
  if (client.println() == 0)
  {
    Serial.println(F("client error")) ;
    return ;
  }
  delay(10);
  char endOfHeaders[] = "\r\n\r\n" ;                // ignore HTTP Header
  if (!client.find(endOfHeaders))
  {
    Serial.println(F("server error")) ;
    return ;
  }
  //===============>>  now read the webpage  <<==================
  for ( int i = 0 ; i < client_bytes ; i++)
  {
    atom = client.read() ;
    sbuf = sbuf + atom ;
  }
#ifdef debug1
  Serial.println(sbuf) ;
  Serial.print(" len sbuf = ") ;
  Serial.println(sbuf.length()) ;
#endif
  //===============>>  now compress the content  <<==================
  // means: delete all: cr,lf,",space,tab and shortening the string
  String t  = "<updated>" ; // begin of "interest" of the content
  String t1 = "</muf>" ;    //  end  of "interest" of the content
  for (int j = (sbuf.indexOf(t) + t.length()) ; j < sbuf.indexOf(t1) ; j++)
  {
    atom = sbuf[j] ;
    if (!isWhitespace(atom) && !isControl(atom) && (atom != 34) && (atom != 255))
    {
      pbuf = pbuf + atom ;     // build the compressed string
    }
  }                            // in pbuf is now the compessed content-ready for parsing
#ifdef debug
  Serial.println() ;
  Serial.println(pbuf) ;
  Serial.print(" len pbuf = ") ;
  Serial.println(pbuf.length()) ;
#endif
}
//*******************************************************************
void get_and_show_bottombar_datas(void)
{
  char * content_string[] = {"<solarflux>", "<signalnoise>", "<muf>"} ;
  String Flux, SN, Muf ;
  char kbuf[30] ;
  flag_color = false ;
  tft.setFont() ;
  tft.fillRoundRect(0, 300, 240, 20, 5, MAROON) ;
  tft.drawRoundRect(0, 300, 240, 20, 5, WHITE) ;
  tft.fillRoundRect(1, 302, 57, 16, 5, BLACK) ;
  tft.setTextColor(CYAN) ;
  tft.setCursor(2, 306) ;
  tft.print("tnx:N0NBH") ;
  parse_string_and_tftprint(content_string[0]) ;
  Flux = pbuf.substring(pos, pos1) ;
  parse_string_and_tftprint(content_string[1]) ;
  SN = pbuf.substring(pos, pos1) ;
  parse_string_and_tftprint(content_string[2]) ;
  Muf = pbuf.substring(pos, pos1) ;
  if (isAlpha(Muf.charAt(1)))
  {
    Muf = "noInfo" ;
  }
  tft.setCursor(51, 306) ;
  tft.setTextColor(WHITE) ;
  sprintf(kbuf, "  Flux:%s S/N:%s MUF:%s", Flux, SN, Muf) ;
  tft.print(kbuf) ;
#ifdef debug1
  Serial.println(kbuf) ;
#endif
}
//***************************************************************
void get_and_show_topbar_datas(void)
{
  tft.setFont() ;
  tft.setTextSize(1) ;
  tft.setTextColor(CYAN) ;
  char buf1[30] ;
  String date1 = pbuf.substring(0, 2) ;   // day
  String date2 = pbuf.substring(2, 5) ;   // month
  String date3 = pbuf.substring(5, 9) ;   // year
  String date4 = pbuf.substring(9, 11) ;  // hour
  String date5 = pbuf.substring(11, 13) ; // min
#ifdef debug
  Serial.println("string-parts:" + date1 + "," + date2 + "," + date3 + "," + date4 + "," + date5) ;
#endif
  sprintf(buf1, "from %s %s %s %s:%s UTC", date1, date2, date3, date4, date5) ;
  tft.setCursor(5, 5) ;
  tft.print(buf1) ;
  tft.drawRoundRect(0, 0, 240, 18, 5, WHITE) ;
}
//******************************************************************
int check_color(String)  // colors of the Ham-band-areas
{
  if (t == "P")          // check "poor"
  {
    color = RED ;
  }
  else if ( t == "F")   // check "fair"
  {
    color = YELLOW ;
  }
  else if ( t == "G")   // check "good"
  {
    color = GREEN ;
  }
  return (color) ;
}
//********************************************************************
int check_day_80_40m(void)
{
  t = "40mtime=day>" ;
  x = pbuf.indexOf(t) + t.length();
  t = pbuf.substring(x, x + 1) ;      // one letter is significant
  return (check_color(t)) ;
}
//********************************************************************
int check_night_80_40m(void)
{
  t = "40mtime=night>" ;
  x = pbuf.indexOf(t) + t.length() ;
  t = pbuf.substring(x, x + 1) ;
  return (check_color(t)) ;
}
//********************************************************************
int check_day_30_20m(void)
{
  t = "20mtime=day>" ;
  x = pbuf.indexOf(t) + t.length() ;
  t = pbuf.substring(x, x + 1) ;
  return (check_color(t)) ;
}
//********************************************************************
int check_night_30_20m(void)
{
  t = "20mtime=night>" ;
  x = pbuf.indexOf(t) + t.length() ;
  t = pbuf.substring(x, x + 1) ;
  return (check_color(t)) ;
}
//********************************************************************
int check_day_17_15m(void)
{
  t = "15mtime=day>" ;
  x = pbuf.indexOf(t) + t.length() ;
  t = pbuf.substring(x, x + 1) ;
  return (check_color(t)) ;
}
//********************************************************************
int check_night_17_15m(void)
{
  t = "15mtime=night>" ;
  x = pbuf.indexOf(t) + t.length() ;
  t = pbuf.substring(x, x + 1) ;
  return (check_color(t)) ;
}
//********************************************************************
int check_day_12_10m(void)
{
  t = "10mtime=day>" ;
  x = pbuf.indexOf(t) + t.length() ;
  t = pbuf.substring(x, x + 1) ;
  return (check_color(t)) ;
}
//********************************************************************
int check_night_12_10m(void)
{
  t = "10mtime=night>" ;
  x = pbuf.indexOf(t) + t.length() ;
  t = pbuf.substring(x, x + 1) ;
  return (check_color(t)) ;
}
//********************************************************************
int check_6m(void)
{
  t = "_6m>" ;
  x = pbuf.indexOf(t) + t.length() ;
  t = pbuf.substring(x, x + 1) ;
  return (check_color(t)) ;
}
//********************************************************************
int check_4m(void)
{
  t = "_4m>" ;
  x = pbuf.indexOf(t) + t.length() ;
  t = pbuf.substring(x, x + 1) ;
  return (check_color(t)) ;
}
//********************************************************************
void  get_and_show_band_colors(void)
{
  tft.fillRect(125, 63, 53, 35, check_day_80_40m()) ;    // 80 day
  tft.fillRect(183, 63, 55, 36, check_night_80_40m()) ;  // 80 night
  tft.fillRect(126, 103, 52, 35, check_day_30_20m()) ;   // 20 day
  tft.fillRect(183, 103, 54, 35, check_night_30_20m()) ; // 20 night
  tft.fillRect(126, 143, 52, 35, check_day_17_15m()) ;   // 15 day
  tft.fillRect(183, 143, 54, 35, check_night_17_15m()) ; // 15 night
  tft.fillRect(126, 183, 52, 35, check_day_12_10m()) ;   // 10 day
  tft.fillRect(183, 183, 54, 35, check_night_12_10m()) ; // 10 night
  tft.fillRect(183, 223, 55, 35, check_6m()) ;           //  6
  tft.fillRect(183, 263, 54, 35, check_4m()) ;           //  4
}
//********************************************************************
bool check_N0NBH_datas(void)
{
  old_pbuf = pbuf ;                       // hold the last values
  get_and_compress_all_client_data() ;    // get the present values
  if (old_pbuf.substring(20, old_pbuf.length()) == pbuf.substring(20, pbuf.length()))
  {
#ifdef debug
    Serial.println("N0NBH-Strings are equal") ;
#endif
    return true ;
  }
  else
  {
#ifdef debug
    Serial.println("N0NBH-Strings not equal !!") ;
#endif
    old_page_nr  = -1 ;            //force updating of the present page
    return false ;
  }
}
//***************************************************************
void handle_OTA(void)
{
  //ArduinoOTA.setPort(8266) ;                       // Port defaults to 8266
  ArduinoOTA.setHostname("D1 mini for Solardat") ;              // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setPassword((const char *)"123") ; // No authentication by default
  ArduinoOTA.onStart([]()
  {
#ifdef debug
    Serial.println("Start") ;
#endif
  } ) ;
  ArduinoOTA.onEnd([]()
  {
#ifdef debug
    Serial.println("End") ;
#endif
  } ) ;
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
#ifdef debug
    Serial.printf("Progress: %u%%\n", (progress / (total / 100))) ;
#endif
  } ) ;
  ArduinoOTA.onError([](ota_error_t error)
  {
#ifdef debug
    Serial.printf("Error[%u]: ", error) ;
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed") ;
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed") ;
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed") ;
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed") ;
    else if (error == OTA_END_ERROR) Serial.println("End Failed") ;
#endif
  } ) ;
  ArduinoOTA.begin() ;
#ifdef debug
  Serial.println("OTA READY") ;
  Serial.println("Ready") ;
  Serial.print("IP address: ") ;
  Serial.println(WiFi.localIP()) ;
#endif
}
//***************************************************************************
