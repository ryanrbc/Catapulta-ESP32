#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>

#define IN1 17
#define IN2 16
#define IN3 5
#define IN4 4
#define SERVO_PIN 19


AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);
Servo gatilho;

const char* ssid = "Catapulta_Equipe_01";
AsyncWebServer server(80);

bool dispararAgora = false;

// HTML 
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Catapulta 2026</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: sans-serif; text-align: center; background: #1a1a1a; color: white; }
        .card { background: #1c23a2; padding: 20px; border-radius: 15px; display: inline-block; margin-top: 20px; }
        .slider { width: 280px; height: 20px; }
        .btn { background: #e74c3c; color: white; padding: 20px; border-radius: 10px; border: none; font-size: 20px; cursor: pointer; width: 100%; margin-top: 15px; }
        #val { font-size: 40px; color: #1abc9c; }
    </style>
</head>
<body>
    <div class="card">
        <h1>CATAPULTA</h1>
        <div id="val">0.5</div><p>Metros</p>
        <input type="range" min="0.5" max="4.0" step="0.1" value="0.5" class="slider" 
               oninput="document.getElementById('val').innerHTML = this.value" 
               onchange="fetch('/setDist?val=' + this.value)">
        <br>
        <button class="btn" onclick="fetch('/fire')">LANÇAR!</button>
    </div>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    
stepper.setMaxSpeed(200);      
stepper.setAcceleration(100);  

    gatilho.attach(SERVO_PIN);
    gatilho.write(0); 

    WiFi.softAP(ssid);
    Serial.println("Conecte no Wi-Fi: Catapulta_Equipe_01");
    Serial.println("Acesse: 192.168.4.1");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    server.on("/setDist", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("val")) {
            float v = request->getParam("val")->value().toFloat();
            // Mapeamento: 0.5m = 0 passos | 4.0m = 2000 passos (esperar a estrutura estar pronta para verificar de forma mais precisa)
            long p = map(v * 10, 5, 40, 0, 2048); 
            stepper.moveTo(p);
            Serial.printf("Alvo: %.1f m -> Passos: %ld\n", v, p);
        }
        request->send(200, "text/plain", "OK");
    });

    server.on("/fire", HTTP_GET, [](AsyncWebServerRequest *request){
        dispararAgora = true;
        request->send(200, "text/plain", "Fogo!");
    });

    server.begin();
}

void loop() {
    
    stepper.run(); 

    if (dispararAgora) {
        
        if (stepper.distanceToGo() == 0) {
            Serial.println("DISPARANDO!");
            gatilho.write(90); 
            delay(1000);
            gatilho.write(0); 
            dispararAgora = false;
        } else {
            Serial.println("Aguardando motor chegar na posição...");
        }
    }
}