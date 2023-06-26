#include <SPI.h>
#include <Ethernet.h>
#include <NewPing.h>
#include <HX711.h>

// Definição dos pinos
const int rainSensorPin = A0;          // Pino analógico para o sensor de chuva
const int ultrasonicTrigPin = 2;      // Pino de saída do sinal de disparo do sensor ultrassônico
const int ultrasonicEchoPin = 3;      // Pino de entrada do sinal de eco do sensor ultrassônico
const int infraredSensorPin = A1;     // Pino de entrada do sensor de proximidade infravermelho
const int ledPin = 7;                 // Pino para o LED
const int ledPin2 = 9; 
const int buttonPin = 8;              // Pino para o botão
const int HX711_DT_PIN = 6;           // Pino de dados do módulo HX711
const int HX711_SCK_PIN = 5;          // Pino de clock do módulo HX711

// Configuração da conexão Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };    // Endereço MAC do Ethernet Shield
IPAddress ip(192, 168, 0, 175);                         // Endereço IP do Arduino
EthernetServer server(80);                              // Porta do servidor web

// Variáveis globais
bool isRainy = false;
int proximityCount = 0;
float weightValue = 0.0;
bool ledStatus = false;
float calibration_factor = 910.0;

// Objeto NewPing
NewPing sonar(ultrasonicTrigPin, ultrasonicEchoPin);

// Objeto HX711
HX711 scale;

void setup() {
  // Inicialização dos componentes
  pinMode(rainSensorPin, INPUT);
  pinMode(ultrasonicTrigPin, OUTPUT);
  pinMode(ultrasonicEchoPin, INPUT);
  pinMode(infraredSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  // Inicialização da conexão Ethernet
  Ethernet.begin(mac, ip);
  server.begin();

  // Inicialização da porta serial
  Serial.begin(9600);

  // Inicialização do módulo HX711
  scale.begin(HX711_DT_PIN, HX711_SCK_PIN);
  scale.set_scale(calibration_factor);  // Fator de calibração para a sua configuração específica
  scale.tare();                         // Tare a escala (subtraia o peso atual do valor de referência)

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
  isRainy = (analogRead(rainSensorPin) < 920);  // Altere o valor de referência de acordo com o sensor de chuva utilizado

  // Atualiza o valor do peso em gramas
  weightValue = scale.get_units() * 1000.0;     // Valor do peso em gramas
  if (weightValue < 0) {
    weightValue = 0; // Define o valor como zero se for negativo
  }

  // Lógica para ligar ou desligar o LED com base no status do sensor de chuva
  if (analogRead(rainSensorPin) < 920) {
    digitalWrite(ledPin2, HIGH);  // Liga o LED
  } else {
    digitalWrite(ledPin2, LOW);   // Desliga o LED
  }

  // Conta as detecções do sensor de proximidade infravermelho
  if (digitalRead(infraredSensorPin) == LOW) {
    proximityCount++;
  }

  // Imprime os valores dos sensores na porta serial
  Serial.print("Sensor de Chuva: ");
  Serial.println(isRainy ? "Está molhado!" : "Está seco!");
  Serial.print("Valor do Sensor de Chuva: ");
  Serial.println(analogRead(rainSensorPin));
  Serial.print("Sensor de Proximidade Ultrassônico: ");
  Serial.print(getUltrasonicDistance());
  Serial.println(" cm");
  Serial.print("Sensor de Proximidade Infravermelho (Detecções): ");
  Serial.println(proximityCount);
  Serial.print("Sensor de Peso: ");
  Serial.println(weightValue >= 0 ? String(weightValue) + " g" : "0 g");

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
  client.println("Connection: close");
  client.println();

  // Envio do corpo HTML
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head>");
  client.println("<meta charset='UTF-8'>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");  // Adiciona a meta tag para tornar a página responsiva
  client.println("<link rel='icon' type='image/png' href='https://blogmasterwalkershop.com.br/arquivos/artigos/sub_wifi/logo_mws.png'/>");
  client.println("<title>Monitoramento apiario</title>");
  client.println("<style>");
  client.println("body {");
  client.println("  background-color: #ADD8E6;");
  client.println("  margin: 0;");
  client.println("  padding: 20px;");
  client.println("  font-family: Arial, Helvetica, sans-serif;");
  client.println("  display: flex;");
  client.println("  flex-direction: column;");
  client.println("  align-items: center;");
  client.println("  text-align: center;");
  client.println("}");
  client.println(".container {");
  client.println("  max-width: 600px;");  // Define a largura máxima do contêiner
  client.println("  margin: 0 auto;");  // Centraliza o contêiner horizontalmente
  client.println("  background-color: #FFFFFF;");
  client.println("  padding: 20px;");
  client.println("  border-radius: 10px;");
  client.println("}");
  client.println(".button-group {");
  client.println("  display: flex;");
  client.println("  justify-content: center;");
  client.println("  margin-top: 20px;");
  client.println("}");
  client.println(".button-group button {");
  client.println("  margin: 0 5px;");
  client.println("  padding: 10px 20px;");  // Altera o tamanho dos botões
  client.println("  border-radius: 5px;");  // Adiciona a borda arredondada aos botões
  client.println("  font-size: 16px;");  // Altera o tamanho da fonte dos botões
  client.println("}");
  client.println(".ligar-button {");
  client.println("  background-color: green;");  // Altera a cor de fundo do botão "Ligar" para verde
  client.println("  color: white;");  // Altera a cor do texto do botão "Ligar" para branco
  client.println("}");
  client.println(".desligar-button {");
  client.println("  background-color: red;");  // Altera a cor de fundo do botão "Desligar" para vermelho
  client.println("  color: white;");  // Altera a cor do texto do botão "Desligar" para branco
  client.println("}");
  client.println("</style>");
  client.println("<meta http-equiv='refresh' content='5'>");  // Atualiza a página a cada 5 segundos
  client.println("</head>");
  client.println("<body>");
  client.println("<div class='container'>");  // Abre o contêiner

  client.println("<h1>Monitoramento de Sensores</h1>");
  client.println("<h2>Sensor de Chuva:</h2>");
  client.println(isRainy ? "<p>Está molhado!</p>" : "<p>Está seco!</p>");
  client.println("<h2>Valor do Sensor de Chuva:</h2>");
  client.println("<p>" + String(analogRead(rainSensorPin)) + "</p>");
  client.println("<h2>Sensor de Proximidade Ultrassônico:</h2>");
  client.println("<p>" + String(getUltrasonicDistance()) + " cm</p>");
  client.println("<h2>Sensor de Proximidade Infravermelho (Detecções):</h2>");
  client.println("<p>" + String(proximityCount) + "</p>");
  client.println("<h2>Sensor de Peso:</h2>");
  client.println("<p>" + String(weightValue >= 0 ? weightValue : 0) + " g</p>");

  // Botões para ligar ou desligar o LED
  client.println("<h2>Controle do LED:</h2>");
  client.println("<div class='button-group'>");
  client.println("<form method='post' action='ligar'>");
  client.println("<button class='ligar-button' type='submit'>Ligar</button>");  // Adiciona a classe de estilo para o botão "Ligar"
  client.println("</form>");
  client.println("<form method='post' action='desligar'>");
  client.println("<button class='desligar-button' type='submit'>Desligar</button>");  // Adiciona a classe de estilo para o botão "Desligar"
  client.println("</form>");
  client.println("</div>");

  client.println("</div>");  // Fecha o contêiner
  client.println("</body>");
  client.println("</html>");
}

int getUltrasonicDistance() {
  unsigned int duration = sonar.ping_median(5);  // Realiza 5 medições para obter uma leitura mais precisa
  return duration / US_ROUNDTRIP_CM;
}
