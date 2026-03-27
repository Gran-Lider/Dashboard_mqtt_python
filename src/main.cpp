#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "FIBRAMAX_NIVICELA";
const char* password = "Steph3@nY7899";
const char* mqtt_server = "192.168.18.11";
const uint16_t mqtt_port = 1883;
const char* topicPot = "casa/potenciometro";
const char* topicLed = "casa/led";

WiFiClient espClient;
PubSubClient client(espClient);

const int pinPot = 34;
const int pinLed = 2;
unsigned long lastMsg = 0;

const char* estadoMQTT(int estado) {
  switch (estado) {
    case MQTT_CONNECTION_TIMEOUT:
      return "timeout de conexion";
    case MQTT_CONNECTION_LOST:
      return "conexion perdida";
    case MQTT_CONNECT_FAILED:
      return "fallo en la conexion TCP";
    case MQTT_DISCONNECTED:
      return "cliente desconectado";
    case MQTT_CONNECT_BAD_PROTOCOL:
      return "protocolo no soportado";
    case MQTT_CONNECT_BAD_CLIENT_ID:
      return "client ID invalido";
    case MQTT_CONNECT_UNAVAILABLE:
      return "broker no disponible";
    case MQTT_CONNECT_BAD_CREDENTIALS:
      return "credenciales invalidas";
    case MQTT_CONNECT_UNAUTHORIZED:
      return "no autorizado";
    default:
      return "estado desconocido";
  }
}

void conectarWiFi() {
  Serial.println("Conectando a WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char mensaje[32];
  unsigned int copyLength = min(length, sizeof(mensaje) - 1);
  memcpy(mensaje, payload, copyLength);
  mensaje[copyLength] = '\0';

  Serial.print("Mensaje recibido en ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(mensaje);

  if (strcmp(topic, topicLed) == 0) {
    if (strcmp(mensaje, "ON") == 0) {
      digitalWrite(pinLed, HIGH);
      Serial.println("LED encendido");
    } else if (strcmp(mensaje, "OFF") == 0) {
      digitalWrite(pinLed, LOW);
      Serial.println("LED apagado");
    } else {
      Serial.println("Comando no reconocido. Usa ON u OFF.");
    }
  }
}

void conectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando a MQTT en ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(mqtt_port);
    Serial.print(" ... ");

    String clientId = "ESP32_POT_";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("conectado");
      bool suscrito = client.subscribe(topicLed);
      Serial.print("Suscripcion a ");
      Serial.print(topicLed);
      Serial.print(": ");
      Serial.println(suscrito ? "ok" : "fallo");
    } else {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
      Serial.print(" (");
      Serial.print(estadoMQTT(client.state()));
      Serial.println(")");
      Serial.println("Verifica IP del broker, Mosquitto escuchando en red local y firewall.");
      Serial.println("Reintentando en 2 segundos...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  randomSeed(micros());
  pinMode(pinLed, OUTPUT);
  digitalWrite(pinLed, LOW);

  conectarWiFi();
  client.setCallback(callback);
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, intentando reconectar...");
    conectarWiFi();
  }

  if (!client.connected()) {
    conectarMQTT();
  }

  client.loop();

  unsigned long ahora = millis();
  if (ahora - lastMsg > 5000) {
    lastMsg = ahora;

    int valor = analogRead(pinPot);

    char mensaje[16];
    snprintf(mensaje, sizeof(mensaje), "%d", valor);

    bool publicado = client.publish(topicPot, mensaje);
    Serial.print("Valor enviado: ");
    Serial.println(mensaje);
    Serial.print("Resultado publish: ");
    Serial.println(publicado ? "ok" : "fallo");
  }
}
