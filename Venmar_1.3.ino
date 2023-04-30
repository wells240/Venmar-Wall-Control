/* Venmar_V1.3 - 2023-04-30
 * This is my first programming project.
 * Any suggestion to improve or optimize the code is welcome.
 * Special thanks to my friend Jörgen from the Netherlands for his very precious help.
 * Have a look at his project https://www.instructables.com/member/Jörgen%20van%20den%20Berg/
 * 
 * I never liked the way our air exchanger worked.
 * After optimizing with a relay circuit to control
 * the exchange/recirculation cycle it is time to improve the whole control unit.
 * 
 * We often use the Boost mode to evacuate cooking fumes and odors.
 * However, we frequently forget the exchanger in this mode. This greatly cools the house in winter time and increases the heating costs.
 * This is why the two functions Timed Boost and normal cycling/recirculation timer were made.
 * While we're at it, why not take the opportunity to make it a little more smart with web & app control
 * 
 * Off = Does it need to be explained?
 * Normal ack Exch = Fresh air at normal speed
 * Boost  = Fresh air at hi speed
 * Recirculation ack Recir = Mooving air without fresh air at normal speed 
 * Timer Boost = Fresh air at hi speed for the set time
 * Alterning Normal and Reciculation = Alterning Fresh air at normal speed and mooving air without fresh air for a set time
 * 
 * Have a look at my Instructables https://www.instructables.com/member/Christian%20Boudreau/
 * Material:
 * ESP32-doit-dev kit V1
 * MCP4141-503E/P
 * 1.5" RGB Oled 128x128 SPI - https://www.waveshare.com/product/displays/oled/oled-3/1.5inch-rgb-oled-module.htm
 *                 https://www.waveshare.com/wiki/1.5inch_RGB_OLED_Module
 * LED Blue
 * Resistor 1/4w 3.5Ω & 211Ω
 * Micro Switch - PTS645VM13SMTR92 LFS
 * Terminal block - DG301-5.0-02P-12-00A(H)
 * 
 * 
 * App
 * http://ai2.appinventor.mit.edu
 * https://microcontrollerslab.com/esp32-controller-android-mit-app-inventor/
 * ezButton
 * https://arduinogetstarted.com/tutorials/arduino-button-library
 * Button Ddebounce
 * https://esp32io.com/tutorials/esp32-button-debounce
 * Server
 * https://lastminuteengineers.com/creating-esp32-web-server-arduino-ide/
 * https://github.com/me-no-dev/ESPAsyncWebServer
 * https://github.com/me-no-dev/AsyncTCP
 * OTA
 * https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/
 * Get-POST
 * https://randomnerdtutorials.com/esp32-http-get-post-arduino/
 * Input data form
 * https://randomnerdtutorials.com/esp32-esp8266-input-data-html-form/
 * https://www.w3schools.com/html/tryit.asp?filename=tryhtml_input_time
 * tft
 * https://electropeak.com/learn/absolute-beginners-guide-to-tft-lcd-displays-by-arduino/#Basic_Commands
 * 
 * UPDATE
 * You can use AsyncElegantOTA to perform over the air update.
 * I do use a second ESP32 to test new code and wen ever ready I just do an over the air update so I dont have to open wall mount setup
 * https://github.com/ayushsharma82/AsyncElegantOTA
 * https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/
*/

// Dont forget to set your SSID & wifi password
// Open serial monitor to see your Ip - needed to be set in web page/APP.
// In any browser type the Ip from serial monitor ex: http://xxx.xxx.x.xx/
// This will open the web page on the server of ESP32
// With this webpage/app you can set the timer and activate fonction

//-------- Loading library -------
  #include <Arduino.h>
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <Wire.h>                     // For the Oled
  #include <SPI.h>                      // Serial com needed for the Potentiometer
  #include <ezButton.h>                 // For the physical button
  #include <SPIFFS.h>                   // For reading, writing and adding data to a file - save time for timer
  #include <AsyncTCP.h>                 // Acces to server
  #include <AsyncElegantOTA.h>;         // OverTheAir update https://github.com/ayushsharma82/AsyncElegantOTA
  #include <ESPAsyncWebServer.h>        // WebServer
  #include <Adafruit_GFX.h>             // Core graphics library
  #include <Adafruit_SSD1351.h>         // Hardware-specific library
  #include <Fonts/FreeSerif9pt7b.h>     // Font - just to make things look nicer
  #include "Icons.h"            // This is the Icon file - needs to be in directory of main code 
  AsyncWebServer server(80);            // Set WebServer on port 80
  
//-------- end Loading library -------

//-------- Variable declaration -------
// ------- YOU HAVE TO EDIT THIS SECTION -------
  const char* ssid = "YOUR SSID";                               // Is the variable that hold your local wifi SSID
  const char* password = "YOUR PASSWORD";                            // Is the variable that hold your wifi password
  //To adapt the program to your system take the resistance reading between the black and green wire for each position of the original control switch.
  // adjust those # to match your reading
  int Off = 95;                                                  
  int Normal = 47;
  int Recir = 9;
  int Boost = 23;
// ------- NO OTHER CODE NEED TO BE EDIT BELLOW THIS LINE -------

  const char* PARAM_INT_1 = "ExchDura";                         // PARAM_INT_1 is used to transfer Exchange time from web server to ESP32
  const char* PARAM_INT_2 = "RecirDura";                        // PARAM_INT_2 is used to transfer Recicling time from web server to ESP32
  const char* PARAM_INT_3 = "BoostDura";                        // PARAM_INT_3 is used to transfer Boost time from web server to ESP32
  unsigned long  InpMsgExchDura;                                // will be used to calculate time of ExchDura in milisecond - hh mm ss (no:)
  unsigned long  InpMsgRecirDura;                               // will be used to calculate time of RecirDura in milisecond - hh mm ss (no:)
  unsigned long  InpMsgBoostDura;                               // will be used to calculate time of Boost in milisecond - hh mm ss (no:)
  int long ExchDura;                                            // Hold Exchange time from input form - hh:mm:ss
  int long RecirDura;                                           // Hold input Recicle time from input form - hh:mm:ss
  int long BoostDura;                                           // Hold input Boost time from input form - hh:mm:ss
  String yourInputExchDura;                                     // Hold Exchange time read from text file - hh:mm:ss
  String yourInputRecirDura;                                    // Hold Recicle time read from text file - hh:mm:ss
  String yourInputBoostDura;                                    // Hold Boost time read from text file - hh:mm:ss
  String BoostDuraTime;                                         // Hold Boost time for server read from text file - hh:mm:ss
  String ExchDuraTime;                                          // Hold Boost time for server read from text file - hh:mm:ss
  String RecirDuraTime;                                         // Hold Boost time for server read from text file - hh:mm:ss
  
  bool IsExchCyclingOn = false;                                 // For Cycling Normal/Recirculation timer
  bool IsOn = true;
  long rememberTime=0;

  bool IsExchBoostOn = false;                                   // For Boost Timer
  bool TimerOn = true; 
//-------- end Variable declaration -------

// -------- WebPage Part in HTML -------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);
    }
  </script>
  <style>
  .button {
  background-color: #4CAF50; /* Green */
  border: none;
  color: white;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
.button {width: 150px;}

.button {
  background-color: white; 
  color: black; 
  border: 2px solid #4CAF50;
}

.button:hover {
  background-color: #4CAF50;
  color: white;
}

</style>
  </head><body>

    <form action="/get"target="hidden-form">
    <label for="ExchDura">Fresh Air Exchange Duration:</label>
     ExchDura (current value %ExchDura%): <input type="time" name="ExchDura" step="2"> 
  <Br>
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form> <Br>

    <form action="/get"target="hidden-form">
    <label for="RecirDura">Recirculation Duration:</label>
    RecirDura (current value %RecirDura%): <input type="time" name="RecirDura" step="2">
  <Br>
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><Br>
  
    <form action="/get"target="hidden-form">
    <label for="BoostDura">Boost Duration:</label>
    BoostDura (current value %BoostDura%): <input type="time" name="BoostDura" step="2">
  <Br>
    <input type="submit" value="Submit" onclick="submitMessage()">
    </form><Br>


              <button class="button" onclick="OffRequest();">Off</button>
              <script>
              function OffRequest () {
                var xhttp = new XMLHttpRequest();
                xhttp.open("GET", "/Off", true);
                xhttp.send();
              }
              </script>
 <Br><Br>
              <button class="button"  onclick="NormalRequest();">Normal</button>
              <script>
              function NormalRequest () {
                var xhttp = new XMLHttpRequest();
                xhttp.open("GET", "/Normal", true);
                xhttp.send();
              }
              </script>
 <Br><Br>
            <button class="button" onclick="RecirRequest();">Recir</button>
            <script>
            function RecirRequest () {
              var xhttp = new XMLHttpRequest();
              xhttp.open("GET", "/Recir", true);
              xhttp.send();
            }
            </script>
 <Br><Br>
            <button class="button" onclick="BoostRequest();">Boost</button>
            <script>
            function BoostRequest () {
              var xhttp = new XMLHttpRequest();
              xhttp.open("GET", "/Boost", true);
              xhttp.send();
            }
            </script>
 <Br><Br>
            <button class="button" onclick="CyclingRequest();">Cycling</button>
            <script>
            function CyclingRequest () {
              var xhttp = new XMLHttpRequest();
              xhttp.open("GET", "/Cycling", true);
              xhttp.send();
            }
            </script>
 <Br><Br>
            <button class="button" onclick="BoostTimerRequest();">Boost Timer</button>
            <script>
            function BoostTimerRequest () {
              var xhttp = new XMLHttpRequest();
              xhttp.open("GET", "/BoostTimer", true);
              xhttp.send();
            }
            </script>
      <iframe style="display:none" name="hidden-form"></iframe>
            </body></html>)rawliteral";

  void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
  }
  String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}
// Replaces placeholder with stored values
String processor(const String& var){
  Serial.println(var);
  if(var == "ExchDura"){
    return readFile(SPIFFS, "/ExchDura.txt");
  }
  else if(var == "RecirDura"){
    return readFile(SPIFFS, "/RecirDura.txt");
  }
  else if(var == "BoostDura"){
    return readFile(SPIFFS, "/BoostDura.txt");
  }
  return String();
}
//-------- End Webpage part -------

//-------- Screen part --------
  #define SCREEN_WIDTH 128      // OLED display width, in pixels
  #define SCREEN_HEIGHT 128     // OLED display height, in pixels
  #define SCLK_PIN 18     // GPIO 18
  #define MOSI_PIN 23     // GPIO 23
  #define CS_PIN   17     // GPIO 17 TX2
  #define DC_PIN   16     // GPIO 16 RX2
  #define RST_PIN  5      // GPIO 5

// Colour definitions
  #define BLACK           0x0000      /*   0,   0,   0 */
  #define BLUE            0x001F      /*   0,   0, 255 */
  #define GREEN           0x07E0      /*   0, 255,   0 */
  #define CYAN            0x07FF      /*   0, 255, 255 */
  #define RED             0xF800      /* 255,   0,   0 */
  #define MAGENTA         0xF81F      /* 255,   0, 255 */
  #define YELLOW          0xFFE0      /* 255, 255,   0 */
  #define WHITE           0xFFFF      /* 255, 255, 255 */
  #define ORANGE          0xFD20      /* 255, 165,   0 */

Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN); // start the Oled
//-------- End Screen part --------

//-------- Digital potentiometer part --------
  byte address = 0x00;
  int CS= 5; // Potentiometer pins #3 connected on pins D5(GIPO-5)

            // Potentiometer Pins:
            // GIPO-5             --> 1 |---| 5 <-- Vin
            // GIPO-18 (vspi clk) --> 2 |   | 5 <-- To black wire on Venmar
            // GIPO-23 (vspi CS0) --> 3 |   | 7 <-- Wiper - To green wire on Venmar
            // Gnd                --> 4 |   | 8 <-- 
//-------- End Digital potentiometer part --------

//-------- Button DEBOUNCE --------
  #define DEBOUNCE_TIME 50 // the debounce time in millisecond, increase this time if button still chatters
  ezButton button(32); // create ezButton object that attach to pin GIOP32
  unsigned long lastCount = 0;
  unsigned long count = 0;
//-------- End Button DEBOUNCE --------

void digitalPotWrite(int value){                              // Fonction for digital potentiometer
    digitalWrite(CS, LOW);
    SPI.transfer(address);
    SPI.transfer(value);
    digitalWrite(CS, HIGH);
}
void ExchOff(){                                              // Fonction for put exchanger to off
  digitalPotWrite(95);
  tft.begin();
  tft.fillScreen(BLACK);
  tft.drawBitmap(27, 20, Off_icon_74x64, 74, 64, RED);
  digitalWrite(2,LOW);
  IsExchCyclingOn = false;
  IsExchBoostOn= false;
}
void ExchNormal(){                                           // Fonction for put exchanger to Normal fresh air exchage at low speed
  digitalPotWrite(47);
  tft.begin();
  tft.fillScreen(BLACK);
  tft.drawBitmap(27, 20, Normal_icon_74x64, 74, 64, ORANGE);
  digitalWrite(2,LOW);
  IsExchCyclingOn = false;
  IsExchBoostOn= false;
}
void ExchRecir(){                                           // Fonction for put exchanger to recycling air
  digitalPotWrite(9);
  tft.begin();
  tft.fillScreen(BLACK);
  tft.drawBitmap(27, 20, recirc_icon_74x64, 74, 64, GREEN);
  digitalWrite(2,LOW);
  IsExchCyclingOn = false;
  IsExchBoostOn= false;
}
void ExchBoost(){                                           // Fonction for put exchanger to Boost exchange fresh air at high speed
  digitalPotWrite(23);
  tft.begin();
  tft.fillScreen(BLACK);
  tft.drawBitmap(27, 20, Boost_icon_74x64, 74, 64, BLUE);
  digitalWrite(2,LOW);
  IsExchCyclingOn = false;
  IsExchBoostOn= false;
}
void ExchCyclingSwitch(){                                   // Fonction to switch cycling mode to On/Off - Button is of type Toggle
  if(!IsExchCyclingOn) {
    IsExchCyclingOn = true;                                 // Lets do the work
    IsExchBoostOn = false;
    IsOn = true;                                            // First time start with .?. depends on your setting of IsOn
  } else { 
    IsExchCyclingOn = false;                                // Do nothing
  }
}

void ExchBoostSwitch(){                                    // Fonction to switch Boost timer On/Off
  if(!IsExchBoostOn) {
    IsExchBoostOn = true;                                  // Lets do the work
    IsExchCyclingOn = false;
    TimerOn = true;                                        // First time start with .?. depends on your setting of TimerOn
  } else { 
    IsExchBoostOn = false;                                 // Do nothing
  }
}
 
void writeFile(fs::FS &fs, const char * path, const char * message){ // Fonction to write time in text file
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void ReadTimeExchDura(){
  yourInputExchDura = readFile(SPIFFS, "/ExchDura.txt");            // Recall echange duration from text file
  Serial.print("*** Your Fresh air timer: ");
  Serial.println(yourInputExchDura);
       uint8_t hour = 0;
       uint8_t min  = 0;
       uint8_t sec  = 0;
                                                                    // Notice millis() is of type <unsigned long> so we define with the same type:
       InpMsgExchDura = 0;                                          // Limited Range is 0 to 4,294,967,295 max.     
                                                                    // MyChar is a pointer to the first byte of yourInputExchDura, c_str() is a method of class String
       const char* MyChar = yourInputExchDura.c_str();              // MyChar is a pointer to the first byte of InpMsgExchDura
       sscanf(MyChar, "%d:%d:%d", &hour, &min, &sec);               // Load values into the variables hour, min, sec
       Serial.printf("InpMsgExchDura");
       Serial.printf("[2] %d %d %d ",hour,min,sec);                 // Show the content of the variables hour, min, sec
       InpMsgExchDura = (hour*3600 + min*60 + sec)*1000;            // Sum it all up in seconds and convert to ms
       Serial.printf(" --> [%10d] ms", InpMsgExchDura);             // Show the formatted content of the variable InpMsgExchDura
       Serial.println(); 
 } 
 
void ReadTimeRecirDura(){
     yourInputRecirDura = readFile(SPIFFS, "/RecirDura.txt");       // Recall recycling duration from text file
  Serial.print("*** Your Reciculating air timer: ");
  Serial.println(yourInputRecirDura);
        uint8_t hour = 0;
        uint8_t min  = 0;
        uint8_t sec  = 0;
       InpMsgRecirDura = 0;                                         // Limited Range is 0 to 4,294,967,295 max     
       const char* MyChar2 = yourInputRecirDura.c_str();            // MyChar2 is a pointer to the first byte of yourInputRecirDura     
       sscanf(MyChar2, "%d:%d:%d", &hour, &min, &sec);              // Load values into the variables hour, min, sec
       Serial.printf("InpMsgRecirDura");
       Serial.printf("[2] %d %d %d ",hour,min,sec);                 // Show the content of the variables hour, min, sec
       InpMsgRecirDura = (hour*3600 + min*60 + sec)*1000;           // Sum it all up in seconds and convert to ms
       Serial.printf(" --> [%10d] ms", InpMsgRecirDura);            // Show the formatted content of the variable InpMsgRecirDura
       Serial.println();
}

void ReadTimeBoostDura(){
     yourInputBoostDura = readFile(SPIFFS, "/BoostDura.txt");       // Recall recycling duration from text file
  Serial.print("*** Your Boost timer: ");
  Serial.println(yourInputBoostDura);
        uint8_t hour = 0;
        uint8_t min  = 0;
        uint8_t sec  = 0;
       InpMsgBoostDura = 0;                                         // Limited Range is 0 to 4,294,967,295 max     
       const char* MyChar3 = yourInputBoostDura.c_str();            // MyChar2 is a pointer to the first byte of yourInputRecirDura     
       sscanf(MyChar3, "%d:%d:%d", &hour, &min, &sec);              // Load values into the variables hour, min, sec
       Serial.printf("InpMsgBoostDura");
       Serial.printf("[2] %d %d %d ",hour,min,sec);                 // Show the content of the variables hour, min, sec
       InpMsgBoostDura = (hour*3600 + min*60 + sec)*1000;           // Sum it all up in seconds and convert to ms
       Serial.printf(" --> [%10d] ms", InpMsgBoostDura);            // Show the formatted content of the variable InpMsgRecirDura
       Serial.println(); 
}
//------ Fonction to read stored time in txt file for client control - web page/android app/ESP32 -------------

void ReadExchDuraTime(){
  ExchDuraTime = readFile(SPIFFS, "/ExchDura.txt");            // Recall echange duration from text file and set the value in a variable
  Serial.print("*** Your Fresh air timer is: ");
  Serial.println(ExchDuraTime);
}
void ReadRecirDuraTime(){
  RecirDuraTime = readFile(SPIFFS, "/RecirDura.txt");            // Recall echange duration from text file and set the value in a variable
  Serial.print("*** Your Ricycling air timer is: ");
  Serial.println(RecirDuraTime);
}
void ReadBoostTime(){
  BoostDuraTime = readFile(SPIFFS, "/BoostDura.txt");            // Recall echange duration from text file and set the value in a variable
  Serial.print("*** Your Boost Fresh air timer is: ");
  Serial.println(BoostDuraTime);
}


void setup(){

    pinMode (CS, OUTPUT);                                           // Specify the signal pin (Gpio 5) to the potentiometer (pin 1)

    button.setDebounceTime(DEBOUNCE_TIME);                          // set debounce time to 50 milliseconds
    button.setCountMode(COUNT_RISING);                              // Set the button push to count from 1
    
    ExchOff();                                                      // Set the exchanger to Off for initial start
    
    Serial.begin(115200);
    tft.begin();
    tft.setFont(&FreeSerif9pt7b);
//---------------- Led for timed fresh air in mode -------------------
  pinMode(2,OUTPUT);
  digitalWrite(2,LOW);
//-------------- End of Led Cycling -------------------
    
// ---------- Wifi Server part ---------------
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
      WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  // ---------- end Wifi Server part ---------------

// -------- Getting the Data from web page ----------
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){      // Send web page with input fields to client
   
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
  
  String inputMessage;

    if (request->hasParam(PARAM_INT_1)) {
      inputMessage = request->getParam(PARAM_INT_1)->value();
      writeFile(SPIFFS, "/ExchDura.txt", inputMessage.c_str());    // Call fonction WriteFile exchange duration to text file and converts string to an array of characters with a null character at the end
      }
    else if (request->hasParam(PARAM_INT_2)) {
      inputMessage = request->getParam(PARAM_INT_2)->value();
      writeFile(SPIFFS, "/RecirDura.txt", inputMessage.c_str());   // Call fonction WriteFile recyclig duration to text file and converts string to an array of characters with a null character at the end
    }
    else if (request->hasParam(PARAM_INT_3)) {
      inputMessage = request->getParam(PARAM_INT_3)->value();
      writeFile(SPIFFS, "/BoostDura.txt", inputMessage.c_str());   // Call fonction WriteFile Boost duration to text file and converts string to an array of characters with a null character at the end
    }
    else {
          inputMessage = "No message sent";
    }
     Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });

// ------ Off Buton -------
 server.on("/Off", HTTP_GET, [](AsyncWebServerRequest *request){
   ExchOff();
  });
// ------ Normal Buton -------  
 server.on("/Normal", HTTP_GET, [](AsyncWebServerRequest *request){
   ExchNormal();
  });
// ------ Recirculation Buton -------  
 server.on("/Recir", HTTP_GET, [](AsyncWebServerRequest *request){
   ExchRecir();
  });  
// ------ Boost Buton ------- 
 server.on("/Boost", HTTP_GET, [](AsyncWebServerRequest *request){
   ExchBoost();
  });   
// ------ Cycling Buton -------  
 server.on("/Cycling", HTTP_GET, [](AsyncWebServerRequest *request){
  ExchCyclingSwitch();
  Serial.print("*** Your Fresh air timer is: ");
  Serial.println(ExchDuraTime);
  Serial.print("*** Your Ricycling air timer is: ");
  Serial.println(RecirDuraTime);
  });
// ------ Boost timer Buton ------- 
 server.on("/BoostTimer", HTTP_GET, [](AsyncWebServerRequest *request){
    ExchBoostSwitch();
    Serial.print("*** Your Boost Fresh air timer is: ");
    Serial.println(BoostDuraTime);
  }); 
// ------ Send stored time to client -------
  server.on("/ExchDuraStoredTime", HTTP_GET, [](AsyncWebServerRequest *request){
    ReadExchDuraTime();
  request->send(200, "text/plain", ExchDuraTime); // send response to client
});
  server.on("/RecirDuraStoredTime", HTTP_GET, [](AsyncWebServerRequest *request){
    ReadRecirDuraTime();
  request->send(200, "text/plain", RecirDuraTime); // send response to client
});
  server.on("/BoostStoredTime", HTTP_GET, [](AsyncWebServerRequest *request){
    ReadBoostTime();
  request->send(200, "text/plain", BoostDuraTime); // send response to client
});




  server.onNotFound(notFound);
  AsyncElegantOTA.begin(&server);                                          // Over The Air update Start ElegantOTA
  server.begin();
// -------- End of Getting the Data from web page ----------

} //------- End of Setup ----------

void loop() {

  
//------------ Push button count ----------------
    button.loop();                                             // MUST call the loop() function first https://arduinogetstarted.com/library/button/example/arduino-button-count

    count = button.getCount();
    if (count != lastCount) {
    Serial.println(count);
    int countIn6 = count % 6 + 1;

    switch (countIn6) {
      case 1: {
        ExchOff();
        break;
      }
      case 2: {
        ExchNormal();
        break;
      }
      case 3: {
        ExchRecir();
        break;
      }
      case 4: {
        ExchBoost();
        break;
      }
      case 5: {                                               // Treat as a toggle switch
        ExchCyclingSwitch();
        break;
      }
      case 6: {                                               // Treat as a toggle switch
        ExchBoostSwitch();
        break;
      }
    }
    lastCount = count;
  }
//------------ End of Physical button push count ----------------

// ---------- Loop for cycling between fresh air exchange and recycling ------------------
if(IsExchCyclingOn) {
  if(IsOn) {
      ReadTimeExchDura();
      if(millis() - rememberTime >= InpMsgRecirDura) {
      digitalWrite(2, HIGH);
      digitalPotWrite(47);
      tft.begin();
      tft.fillScreen(BLACK);
      tft.drawBitmap(27, 20, Normal_icon_74x64, 74, 64, ORANGE);
      tft.drawBitmap(85, 0, epd_bitmap_T, 40, 40, WHITE );
      tft.setCursor(33, 100);
      tft.setTextColor(WHITE);
      tft.println(yourInputExchDura);
      IsOn = false;
      rememberTime = millis();
    }
  } 
  if(!IsOn) {
      ReadTimeRecirDura();
      if(millis() - rememberTime >= InpMsgExchDura) {
      digitalWrite(2, LOW);
      digitalPotWrite(9);
      tft.begin();
      tft.fillScreen(BLACK);
      tft.drawBitmap(27, 20, recirc_icon_74x64, 74, 64, GREEN);
      tft.drawBitmap(85, 0, epd_bitmap_T, 40, 40, WHITE);
      tft.setTextSize(1);
      tft.setCursor(33, 100);
      tft.setTextColor(WHITE);
      tft.println(yourInputRecirDura);
      IsOn = true;
      rememberTime = millis();
    }  //end if(millis()
  }    //end if(!IsOn)
}      // end IsExchCyclingOn
// ---------- End of Loop for cycling between air exchange and recycling ------------------

// ------------- Boost Timer ----------------------------------
if(IsExchBoostOn) {
  if(TimerOn) {
      ReadTimeBoostDura();
      digitalWrite(2,HIGH);
      digitalPotWrite(23);
      tft.begin();
      tft.fillScreen(BLACK);
      tft.drawBitmap(27, 20, Boost_icon_74x64, 74, 64, BLUE);
      tft.drawBitmap(85, 0, epd_bitmap_T, 40, 40, WHITE);
      tft.setTextSize(1);
      tft.setCursor(33, 100);
      tft.setTextColor(WHITE);
      tft.println(yourInputBoostDura);
      TimerOn = false;
      rememberTime = millis();
      }

  if(!TimerOn) {
      if(millis() - rememberTime >= InpMsgBoostDura){
      digitalPotWrite(47);
      tft.begin();
      tft.fillScreen(BLACK);
      tft.drawBitmap(27, 20, Normal_icon_74x64, 74, 64, ORANGE);
      digitalWrite(2,LOW);
      TimerOn = true;
      IsExchBoostOn =false;
    } // end if (millis()
  }   // end !TimerOn
}     // end IsExchBoostOn
// ------------- End Boost timer ----------------------------------
} // --- end loop ---

//----- This is the end -----
