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

// Variáveis de controle de fluxo e tempo
bool dispararAgora = false;
bool precisaResetarWeb = false; 
unsigned long cronometroSequencia = 0;

// Nova Máquina de Estados Temporal
enum EstadoSistema { 
    IDLE, 
    SERVO_ABERTO, 
    RETORNANDO_AO_ZERO, 
    FECHANDO_SERVO 
};
EstadoSistema estadoAtual = IDLE;

// HTML Otimizado com Polling para Reset Automático da Barra
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
    <meta charset="UTF-8">
    <title>Catapulta IPE I</title>
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
        <div id="val">0</div><p>Distância</p>
        <input type="range" min="0" max="4.0" step="0.1" value="0" class="slider" id="sliderDist"
               oninput="document.getElementById('val').innerHTML = this.value" 
               onchange="fetch('/setDist?val=' + this.value)">
        <br>
        <button class="btn" onclick="fetch('/fire')">LANÇAR!</button>
    </div>

    <script>
        // Verifica a cada 1 segundo se a catapulta concluiu o ciclo de reset
        setInterval(function() {
            fetch('/checkReset')
                .then(response => response.text())
                .then(data => {
                    if (data === "1") {
                        // Retorna os elementos da página para a posição inicial (0.5m)
                        document.getElementById('sliderDist').value = "0";
                        document.getElementById('val').innerHTML = "0";
                        console.log("Interface da catapulta resetada!");
                    }
                });
        }, 1000);
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);

    //pinMode(LED_VERDE, OUTPUT);
    pinMode(LED_AMARELO, OUTPUT);

    //digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_AMARELO, LOW);
             
    stepperA.setMaxSpeed(150);      
    stepperA.setAcceleration(30); 
    
    stepperB.setMaxSpeed(150);
    stepperB.setAcceleration(30);

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    gatilho.setPeriodHertz(50); 
    gatilho.attach(SERVO_PIN, 544, 2400); 
    gatilho.write(150); // Posição inicial fechada/armada

    WiFi.softAP(ssid);
    Serial.println("Conecte no Wi-Fi: Catapulta_IPE_I");
    Serial.println("Acesse: 192.168.4.1");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    server.on("/setDist", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("val")) {
            float v = request->getParam("val")->value().toFloat();
            float p = (43.831*v*v + 775.51*v + 1009.8);
            int passo = static_cast<int>(p);
            //long p = map(v * 10, 5, 40, 0, 6144); 
            
            // Garante que as bobinas liguem para mover os motores até o alvo
            stepperA.enableOutputs();
            stepperB.enableOutputs();
            
            stepperA.moveTo(-2*passo);
            stepperB.moveTo(2*passo);
            Serial.printf("Alvo: %.1f m -> Passos: %ld\n", v, v);
        }
        request->send(200, "text/plain", "OK");
    });

    server.on("/fire", HTTP_GET, [](AsyncWebServerRequest *request){
        dispararAgora = true;
        request->send(200, "text/plain", "Fogo!");
    });

    // Endpoint que a página web consulta para saber se deve resetar a barra
    server.on("/checkReset", HTTP_GET, [](AsyncWebServerRequest *request){
        if (precisaResetarWeb) {
            request->send(200, "text/plain", "1"); // Envia sinal de reset
            precisaResetarWeb = false;             // Limpa a flag
        } else {
            request->send(200, "text/plain", "0"); // Sem alterações
        }
    });

    server.begin();

    //digitalWrite(LED_VERDE, HIGH);
    Serial.println("Catapulta Online e LED Verde Aceso");
}

void loop() {
    // Alimenta os motores de passo continuamente para executarem seus movimentos
    
    if (estadoAtual == IDLE) {
        // Usa o valor absoluto (abs) porque o stepperA se move para o lado negativo (-2*v)
        if (abs(stepperA.currentPosition()) >= 4096) {
            stepperA.setMaxSpeed(30);   // Velocidade reduzida = Mais torque no final
            stepperB.setMaxSpeed(30);
        } else {
            stepperA.setMaxSpeed(150);  // Velocidade normal no começo do curso
            stepperB.setMaxSpeed(150);
        }
    } 
    else if (estadoAtual == SERVO_ABERTO || estadoAtual == RETORNANDO_AO_ZERO) {
        // Garante que para voltar para a posição zero, o motor use a velocidade máxima
        stepperA.setMaxSpeed(600);
        stepperB.setMaxSpeed(600);
    }

    stepperA.run(); 
    stepperB.run();

    // Controle do LED indicador de movimento
    if (stepperA.distanceToGo() != 0 || stepperB.distanceToGo() != 0) {
        digitalWrite(LED_AMARELO, HIGH);
    } else {
        digitalWrite(LED_AMARELO, LOW);
    }
    
    // EXECUÇÃO TEMPORAL DA CRONOLOGIA DO PROJETO
    switch (estadoAtual) {
        
        case IDLE:
            if (dispararAgora) {
                // Só inicia se os motores já terminaram de tensionar até o alvo
                if (stepperA.distanceToGo() == 0 && stepperB.distanceToGo() == 0) {
                    Serial.println("\n--- INICIANDO LANÇAMENTO ---");
                    
                    // 1. Desliga os motores de passo para focar a corrente no servo
                    stepperA.disableOutputs();
                    stepperB.disableOutputs();
                    
                    // 2. O servo abre fazendo o lançamento
                    gatilho.write(50); 
                    Serial.println("[PASSO 1] Servo aberto (Lançamento feito).");
                    
                    cronometroSequencia = millis(); 
                    estadoAtual = SERVO_ABERTO;
                } else {
                    Serial.println("Aviso: Motores ainda estão se posicionando.");
                }
                dispararAgora = false; 
            }
            break;

        case SERVO_ABERTO:
            // Aguarda 800ms com o servo aberto para a colher/braço subir livremente
            if (millis() - cronometroSequencia >= 800) {
                Serial.println("[PASSO 2] Servo mantido aberto. Ligando motores de passo para recuar.");
                
                // 3. O motor de passo liga novamente
                stepperA.enableOutputs();
                stepperB.enableOutputs();
                
                // 4. Determina o retorno para a posição zero (Catapulta inicial)
                stepperA.moveTo(0);
                stepperB.moveTo(0);
                
                estadoAtual = RETORNANDO_AO_ZERO;
            }
            break;

        case RETORNANDO_AO_ZERO:
            // 5. Espera os motores de passo chegarem fisicamente no passo zero
            if (stepperA.distanceToGo() == 0 && stepperB.distanceToGo() == 0) {
                Serial.println("[PASSO 3] Motores retornaram ao zero. Desligando corrente dos motores.");
                
                // Desliga o motor de passo novamente conforme solicitado
                stepperA.disableOutputs();
                stepperB.disableOutputs();
                
                // 6. O servo volta a fechar para travar o sistema
                gatilho.write(150); 
                Serial.println("[PASSO 4] Servo fechado (Pronto para rearmar).");
                
                cronometroSequencia = millis();
                estadoAtual = FECHANDO_SERVO;
            }
            break;

        case FECHANDO_SERVO:
            // Aguarda 1 segundo para o servo fechar totalmente antes de liberar a interface web
            if (millis() - cronometroSequencia >= 1000) {
                Serial.println("[PASSO 5] Ciclo finalizado com sucesso.");
                
                // 7. Ativa a flag que avisa a página web para retornar a barra para 0.5m
                precisaResetarWeb = true; 
                
                estadoAtual = IDLE; // Retorna ao estado de espera por novos comandos
            }
            break;
    }
}