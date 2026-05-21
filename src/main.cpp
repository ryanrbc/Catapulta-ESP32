#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>

#define IN1_A 5
#define IN2_A 17
#define IN3_A 16
#define IN4_A 4

#define IN1_B 33
#define IN2_B 25
#define IN3_B 26
#define IN4_B 27

#define SERVO_PIN 19

#define LED_VERDE 23
#define LED_AMARELO 22


AccelStepper stepperA(AccelStepper::HALF4WIRE, IN1_A, IN3_A, IN2_A, IN4_A);
AccelStepper stepperB(AccelStepper::HALF4WIRE, IN1_B, IN3_B, IN2_B, IN4_B);
Servo gatilho;

const char* ssid = "Catapulta_IPE_I";
AsyncWebServer server(80);

bool dispararAgora = false;

// HTML 
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
    <meta charset="UTF-8>> <title>Catapulta IPE I</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: sans-serif; text-align: center; background: #1a1a1a; color: white; }
        .card { background: #0072BE; padding: 20px; border-radius: 15px; display: inline-block; margin-top: 20px; }
        .slider { width: 280px; height: 20px; }
        .btn { background: #ee151f; color: white; padding: 20px; border-radius: 10px; border: none; font-size: 20px; cursor: pointer; width: 100%; margin-top: 15px; }
        #val { font-size: 40px; color: #EAC200; }
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

    pinMode(LED_VERDE, OUTPUT);
    pinMode(LED_AMARELO, OUTPUT);

    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_AMARELO, LOW);
        
    stepperA.setMaxSpeed(500);      
    stepperA.setAcceleration(100); 
    
    stepperB.setMaxSpeed(500);
    stepperB.setAcceleration(100);

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
            // Mapeamento: 0.5m = 0 passos | 4.0m = 2048 passos (esperar a estrutura estar pronta para verificar de forma mais precisa)
            long p = map(v * 10, 5, 40, 0, 6144); 
            stepperA.moveTo(-p);
            stepperB.moveTo(p);
            Serial.printf("Alvo: %.1f m -> Passos: %ld\n", v, p);
        }
        request->send(200, "text/plain", "OK");
    });

    server.on("/fire", HTTP_GET, [](AsyncWebServerRequest *request){
        dispararAgora = true;
        request->send(200, "text/plain", "Fogo!");
    });

    server.begin();

    digitalWrite(LED_VERDE, HIGH);
    Serial.println("Catapulta Online e LED Verde Aceso");
}

void loop() {
    
    stepperA.run(); 
    stepperB.run();

    if (stepperA.distanceToGo() != 0 || stepperB.distanceToGo() != 0)
    {
        digitalWrite(LED_AMARELO,HIGH);
    }

    else
    {
        digitalWrite(LED_AMARELO, LOW);
    }
    

    if (dispararAgora) {
        
        if (stepperA.distanceToGo() == 0 && stepperB.distanceToGo() == 0) {
            Serial.println("DISPARANDO!");
            gatilho.write(90); 
            delay(3000);
            gatilho.write(0); 
            dispararAgora = false;
        } else {
            Serial.println("Aguardando motor chegar na posição...");
        }
    }
}