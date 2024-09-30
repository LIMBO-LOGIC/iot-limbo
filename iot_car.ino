#include <WiFi.h>
#include <PubSubClient.h>

// Configurações - variáveis editáveis
const char* default_SSID = "Wokwi-GUEST"; // Nome da rede Wi-Fi
const char* default_PASSWORD = ""; // Senha da rede Wi-Fi
const char* default_BROKER_MQTT = "40.90.199.82"; // IP do Broker MQTT
const int default_BROKER_PORT = 1883; // Porta do Broker MQTT
const char* default_TOPICO_SUBSCRIBE = "/TEF/car1/cmd"; // Tópico MQTT de escuta
const char* default_TOPICO_PUBLISH_1 = "/TEF/car1/attrs"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_2 = "/TEF/car1/attrs/l"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_3 = "/TEF/car1/attrs/t"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_4 = "/TEF/car1/attrs/h"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_5 = "/TEF/car1/attrs/v"; // Tópico MQTT de envio de informações para Broker
const char* default_ID_MQTT = "fiware_001"; // ID MQTT
const int default_D4 = 2; // Pino do LED onboard
// Declaração da variável para o prefixo do tópico
const char* topicPrefix = "car1";

// Variáveis para configurações editáveis
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* TOPICO_PUBLISH_TEMPERATURE = const_cast<char*>(default_TOPICO_PUBLISH_3);
char* TOPICO_PUBLISH_HUMIDITY = const_cast<char*>(default_TOPICO_PUBLISH_4);
char* TOPICO_PUBLISH_SPEED = const_cast<char*>(default_TOPICO_PUBLISH_5);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4;

//Importações, configurações, variáveis e funções para o DHT22 e suas funcionalidades
#include "DHT.h"
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);      // Inicialização do sensor DHT22 no pino 2

// Definições do sensor ultrassônico
#define TRIGGER_PIN 19 // Pino GPIO para o pino trigger do sensor ultrassonico
#define ECHO_PIN 18    // Pino GPIO para o pino ECHO do sensor ultrassonico
#define DISTANCE_BETWEEN_SENSORS 2.0 // Distância entre os dois pontos de detecção em metros (ajuste conforme necessário)

long duration;
float distance;
unsigned long startTime;
unsigned long endTime;

WiFiClient espClient;
PubSubClient MQTT(espClient);
char EstadoSaida = '0';

void initSerial() {
  Serial.begin(115200);
}

void initWiFi() {
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");
  reconectWiFi();
}

void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

void setup() {
  dht.begin();
  InitOutput();
  initSerial();
  initWiFi();
  initMQTT();

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  delay(1000);
  MQTT.publish(TOPICO_PUBLISH_1, "s|on");
  handleSpeed();
}

void loop() {
  VerificaConexoesWiFIEMQTT();
  EnviaEstadoOutputMQTT();
  handleLuminosity();
  handleHumidity();
  handleTemperature();
  handleSpeed();
  MQTT.loop();
}

void reconectWiFi() {
  if (WiFi.status() == WL_CONNECTED)
    return;
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());

  // Garantir que o LED inicie desligado
  digitalWrite(D4, LOW);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }
  Serial.print("- Mensagem recebida: ");
  Serial.println(msg);

  // Forma o padrão de tópico para comparação
  String onTopic = String(topicPrefix) + "@on|";
  String offTopic = String(topicPrefix) + "@off|";

  // Compara com o tópico recebido
  if (msg.equals(onTopic)) {
    digitalWrite(D4, HIGH);
    EstadoSaida = '1';
  }

  if (msg.equals(offTopic)) {
    digitalWrite(D4, LOW);
    EstadoSaida = '0';
  }
}

void VerificaConexoesWiFIEMQTT() {
  if (!MQTT.connected())
    reconnectMQTT();
  reconectWiFi();
}

void EnviaEstadoOutputMQTT() {
  if (EstadoSaida == '1') {
    MQTT.publish(TOPICO_PUBLISH_1, "s|on");
    Serial.println("- Led Ligado");
  }

  if (EstadoSaida == '0') {
    MQTT.publish(TOPICO_PUBLISH_1, "s|off");
    Serial.println("- Led Desligado");
  }
  Serial.println("- Estado do LED onboard enviado ao broker!");
  delay(1000);
}

void InitOutput() {
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);
  boolean toggle = false;

  for (int i = 0; i <= 10; i++) {
    toggle = !toggle;
    digitalWrite(D4, toggle);
    delay(200);
  }
}

void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPICO_SUBSCRIBE);
    } else {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Haverá nova tentativa de conexão em 2s");
      delay(2000);
    }
  }
}

void handleLuminosity() {
  const int potPin = 34;
  int sensorValue = analogRead(potPin);
  int luminosity = map(sensorValue, 0, 4095, 0, 100);
  String mensagem = String(luminosity);
  Serial.print("Valor da luminosidade: ");
  Serial.println(mensagem.c_str());
  MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str());
}

void handleHumidity() {
  float humidity = dht.readHumidity();
  String mensagem = String(humidity);

  Serial.print("Valor da umidade: ");
  Serial.println(mensagem.c_str());
  MQTT.publish(TOPICO_PUBLISH_HUMIDITY, mensagem.c_str());

}

void handleTemperature() {
  float temperatura = dht.readTemperature();
  String mensagem = String(temperatura);

  Serial.print("Temperatura: ");
  Serial.print(mensagem.c_str());
  Serial.println("°C");
  MQTT.publish(TOPICO_PUBLISH_TEMPERATURE, mensagem.c_str());
}

float velocidadeAtual = 0;
const float velocidadeMinima = 180.0; // Velocidade mínima em km/h
const float velocidadeMaxima = 260.0; // Velocidade máxima em km/h

void handleSpeed() {
  Serial.println("Função velocidade iniciada.");

  // Gera um pulso curto no pino TRIGGER (caso você ainda precise do sensor)
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  // Aqui você pode gerar um valor de velocidade aleatório
  velocidadeAtual = random(velocidadeMinima * 10, velocidadeMaxima * 10) / 10.0; // km/h

  // Imprime a velocidade gerada
  Serial.print("Velocidade: ");
  Serial.print(velocidadeAtual);
  Serial.println(" km/h");

  // Publica a velocidade no MQTT
  MQTT.publish(TOPICO_PUBLISH_SPEED, String(velocidadeAtual).c_str());
}


