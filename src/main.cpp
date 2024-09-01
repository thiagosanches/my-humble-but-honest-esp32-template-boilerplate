#include <esp_task_wdt.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>
#include <HTTPClient.h>
#include <my_headers.h>

#define WATCHDOG_TIMEOUT 10
#define WATCHDOG_REPORT_TIME 5000

unsigned long watchdog;
unsigned long healthchecker;
WebServer server(80);

// Feel free to change the password if needed.
// If you'd like to modify the style, you can use ota-template-login.html as a reference. Just minify and update this variable accordingly.
const char *loginHTML = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\"content=\"width=device-width, initial-scale=1.0\"><title>Login Page</title><link rel=\"stylesheet\"href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css\"><style>body{background-image:linear-gradient(to bottom right,#6c5ce7,#a85eea);background-size:100%100%;height:100vh;margin:0;font-family:Arial,sans-serif;}.login-form{width:300px;margin:50px auto;padding:20px;background-color:#fff;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1);}.login-form h2{text-align:center;margin-bottom:20px;}.login-form form{margin-bottom:20px;}.login-form.form-control{margin-bottom:20px;}.login-form.btn{width:100%;padding:10px;font-size:16px;}.error-message{color:red;font-size:14px;margin-bottom:10px;}</style></head><body><div class=\"login-form\"><h2>Login</h2><form id=\"login-form\"><div class=\"form-group\"><input type=\"text\"class=\"form-control\"id=\"username\"placeholder=\"Username\"required></div><div class=\"form-group\"><input type=\"password\"class=\"form-control\"id=\"password\"placeholder=\"Password\"required></div><div id=\"error-message\"class=\"error-message\"></div><button type=\"submit\"class=\"btn btn-primary\">Login</button></form></div><script>const form=document.getElementById('login-form');const usernameInput=document.getElementById('username');const passwordInput=document.getElementById('password');const errorMessage=document.getElementById('error-message');form.addEventListener('submit',(e)=>{e.preventDefault();const username=usernameInput.value.trim();const password=passwordInput.value.trim();if(username==='admin'&&password==='W!!!Iy!Ai!7a2!r_O_TY'){window.open('/serverIndex');}else{errorMessage.textContent='Invalid username or password';}});</script></body></html>";

// If you'd like to modify the style, you can use ota-template-server.html as a reference. Just minify and update this variable accordingly.
const char *serverHTML = "<!DOCTYPE html><html lang=en><link href=https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css rel=stylesheet><style>body{background-image:linear-gradient(to bottom right,#6c5ce7,#a85eea);height:100vh;margin:0;font-family:Arial,sans-serif}.upload-container{width:300px;margin:50px auto;padding:20px;background-color:#fff;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,.1)}</style><body><script src=https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js></script><script src=https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js></script><div class=upload-container><h2>Upload File</h2><form action=# enctype=multipart/form-data id=upload_form method=POST><div class=form-group><input class=form-control-file name=update type=file></div><button class=\"btn btn-primary\"type=submit>Upload</button></form><div class=mt-2 id=prg>progress: 0%</div></div><script>$(\"form\").submit(function(t){t.preventDefault();var e=$(\"#upload_form\")[0],o=new FormData(e);$.ajax({url:\"/update\",type:\"POST\",data:o,contentType:!1,processData:!1,xhr:function(){var t=new window.XMLHttpRequest;return t.upload.addEventListener(\"progress\",function(t){if(t.lengthComputable){var e=t.loaded/t.total;$(\"#prg\").html(\"progress: \"+Math.round(100*e)+\"%\")}},!1),t},success:function(t,e){console.log(\"success!\")},error:function(t,e,o){}})})</script>";

void reportHealthCheck()
{
  HTTPClient http;
  String requestURL = HEALTHCHECKER_ENDPOINT + "[✅ " + WiFi.localIP().toString().c_str() + "] %20is%20up%20and%20running!";
  http.begin(requestURL.c_str());
  http.GET();
  http.end();
}

void setup()
{
  Serial.begin(115200);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Trying to connect to WIFI!");

  int count = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    Serial.print(WiFi.status());
    count++;
    if (count > 10)
      ESP.restart();
  }

  delay(500);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  reportHealthCheck();

  server.on("/", HTTP_GET, []()
            { server.sendHeader("Connection", "close"); server.send(200, "text/html", loginHTML); });

  server.on("/serverIndex", HTTP_GET, []()
            { server.sendHeader("Connection", "close"); server.send(200, "text/html", serverHTML); });

  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); }, []()
            {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } });
  server.begin();

  // Enable WDT with a timeout of WATCHDOG_TIMEOUT seconds
  esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
  // Add the current task to the WDT
  esp_task_wdt_add(NULL);
  watchdog = millis();
  healthchecker = millis();
}

void loop()
{
  if (millis() - watchdog >= WATCHDOG_REPORT_TIME)
  {
    Serial.println("Resetting WatchDog...");
    esp_task_wdt_reset();
    watchdog = millis();
  }

  if (millis() - healthchecker >= HEALTHCHECKER_INTERVAL)
  {
    Serial.println("Reporting HealthCheck...");
    reportHealthCheck();
    healthchecker = millis();
  }

  server.handleClient();
  // Insert your custom code below, leaving the code above unchanged.
}
