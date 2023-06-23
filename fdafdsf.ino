#include <SPI.h>
#include <Ethernet.h>
#include <NewPing.h>
#include <HX711.h>

// Definição dos pinos
const int rainSensorPin = A0;        // Pino analógico para o sensor de chuva
const int ultrasonicTrigPin = 2;     // Pino de saída do sinal de disparo do sensor ultrassônico
const int ultrasonicEchoPin = 3;     // Pino de entrada do sinal de eco do sensor ultrassônico
const int infraredSensorPin = 4;     // Pino de entrada do sensor de proximidade infravermelho
const int ledPin = 7;                // Pino para o LED
const int buttonPin = 8;             // Pino para o botão
const int HX711_DT_PIN = 6;          // Pino de dados do módulo HX711
const int HX711_SCK_PIN = 5;         // Pino de clock do módulo HX711

// Configuração da conexão Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };    // Endereço MAC do Ethernet Shield
IPAddress ip(192, 168, 0, 175);                         // Endereço IP do Arduino
EthernetServer server(80);                              // Porta do servidor web

// Variáveis globais
bool isRainy = false;
int proximityCount = 0;
float weightValue = 0.0;
bool ledStatus = false;
float calibration_factor = 48011.00;

// Objeto NewPing
NewPing sonar(ultrasonicTrigPin, ultrasonicEchoPin);

// Objeto HX711
HX711 scale;

void setup() {
  // Inicialização dos componentes
  pinMode(ultrasonicTrigPin, OUTPUT);
  pinMode(ultrasonicEchoPin, INPUT);
  pinMode(infraredSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  // Inicialização da conexão Ethernet
  Ethernet.begin(mac, ip);
  server.begin();

  // Inicialização da porta serial
  Serial.begin(9600);

  // Inicialização do módulo HX711
  scale.begin(HX711_DT_PIN, HX711_SCK_PIN);
  scale.set_scale(calibration_factor);  // Fator de calibração para a sua configuração específica
  scale.tare();         // Tare a escala (subtraia o peso atual do valor de referência)

  // Aguarda estabilidade do sensor
  delay(1000);
}

void loop() {
  // Verifica se o botão foi pressionado para ligar ou desligar o LED
  if (digitalRead(buttonPin) == LOW) {
    ledStatus = !ledStatus;
    digitalWrite(ledPin, ledStatus);
  }

  // Verifica o status dos sensores e atualiza as variáveis correspondentes
  isRainy = (analogRead(rainSensorPin) < 55);  // Altere o valor do limiar conforme necessário
  weightValue = scale.get_units() / 1000.0;      // Valor do peso em Kg

  // Zera o contador de detecções
  proximityCount = 0;

  // Conta as detecções do sensor de proximidade infravermelho
  for (int i = 0; i < 2000; i++) {
    if (digitalRead(infraredSensorPin) == LOW) {
      proximityCount++;
    }
    delay(10);
  }

  // Imprime os valores dos sensores na porta serial
  Serial.print("Sensor de Chuva: ");
  Serial.println(isRainy ? "Está molhado!" : "Está seco!");
  Serial.print("Sensor de Proximidade Ultrassônico: ");
  Serial.print(getUltrasonicDistance());
  Serial.println(" cm");
  Serial.print("Sensor de Proximidade Infravermelho (Detecções): ");
  Serial.println(proximityCount);
  Serial.print("Sensor de Peso: ");
  Serial.println(weightValue);

  // Lida com as requisições do cliente na página web
  EthernetClient client = server.available();
  if (client) {
    handleClientRequest(client);
    client.stop();
  }

  // Aguarda 00.1 segundos antes da próxima iteração
  delay(100);
}

void handleClientRequest(EthernetClient client) {
  // Verifica se foi feita uma solicitação POST
  if (client.available()) {
    String request = client.readStringUntil('\r');
    if (request.indexOf("POST /ligar") != -1) {
      ledStatus = true;
      digitalWrite(ledPin, HIGH);
    } else if (request.indexOf("POST /desligar") != -1) {
      ledStatus = false;
      digitalWrite(ledPin, LOW);
    }
  }

  // Prepara a resposta para o cliente
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();

  // Envio do corpo HTML
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head>");
  client.println("<meta charset='UTF-8'>");
  client.println("<link rel='icon' type='image/png' href='https://blogmasterwalkershop.com.br/arquivos/artigos/sub_wifi/logo_mws.png'/>");
  client.println("<title>Monitoramento apiario</title>");
  client.println("<style>body{background-color:#ADD8E6; margin:0; padding:20px; font-family: Arial, Helvetica, sans-serif; display:flex; flex-direction:column; align-items:center; text-align:center;} h1{margin-bottom:20px;} p{margin-top:0;}</style>"); // Adiciona estilos CSS inline
  client.println("<meta http-equiv='refresh' content='5'>");  // Atualiza a página a cada 5 segundos
  client.println("</head>");
  client.println("<body style=background-color:#ADD8E6>");
  client.println("<h1>Monitoramento de Sensores</h1>");
  client.println("<h2>Sensor de Chuva:</h2>");
  client.println(isRainy ? "<p>Está molhado!</p>" : "<p>Está seco!</p>");
  client.println("<h2>Sensor de Proximidade Ultrassônico:</h2>");
  client.println(String(getUltrasonicDistance()) + " cm");
  client.println("<h2>Sensor de Proximidade Infravermelho:</h2>");
  client.println("<p>Detecções: " + String(proximityCount) + "</p>");
  client.println("<h2>Sensor de Peso:</h2>");
  client.println(String(weightValue) + " kg");
  client.println("<h2>LED:</h2>");
  client.println("<form action='/ligar' method='POST'><button type='submit'>Ligar</button></form>");
  client.println("<form action='/desligar' method='POST'><button type='submit'>Desligar</button></form>");
  client.println("<p>Status: " + String(ledStatus ? "Ligado" : "Desligado") + "</p>");
  client.println("</body></html>");
}

float getUltrasonicDistance() {
  unsigned int duration = sonar.ping_cm();
  int distance = duration;
  return distance;
}
