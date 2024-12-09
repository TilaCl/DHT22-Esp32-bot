#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "DHT.h"

#define DHTPIN 17 
#define DHTTYPE DHT22

// Configuración Wi-Fi
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "CLAVE_WIFI"

// Token del Bot de Telegram y Chat ID
#define BOT_TOKEN "TOKEN"
#define CHAT_ID "TU_CHAT_ID"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

DHT dht(DHTPIN, DHTTYPE);

const unsigned long TIEMPO_ENTRE_ESCANEOS = 1000; 
unsigned long ultima_vez_bot; 
float temperaturaC;
float temperaturaF;
float humedad;

void setup()
{
  Serial.begin(9600);
  Serial.println(F("Iniciando prueba del sensor DHT..."));

  dht.begin();

  Serial.print("Conectando a la red Wi-Fi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setInsecure();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConexión Wi-Fi exitosa. Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  // Leer los datos del sensor
  humedad = dht.readHumidity();
  temperaturaC = dht.readTemperature();
  temperaturaF = dht.readTemperature(true);

  // Validar las lecturas
  if (isnan(humedad) || isnan(temperaturaC) || isnan(temperaturaF)) {
    Serial.println("Error al leer el sensor DHT. Revisar conexiones.");
    delay(2000);
    return;
  }

  // Enviar alerta si la temperatura supera los 24 °C
  if (temperaturaC > 24.0) {
    enviarAlerta();
  }

  // Procesar mensajes del bot
  if (millis() - ultima_vez_bot > TIEMPO_ENTRE_ESCANEOS) {
    int numNuevosMensajes = bot.getUpdates(bot.last_message_received + 1);
    while (numNuevosMensajes) {
      manejarMensajesNuevos(numNuevosMensajes);
      numNuevosMensajes = bot.getUpdates(bot.last_message_received + 1);
    }
    ultima_vez_bot = millis();
  }

  delay(1000); // Reducir carga innecesaria en el ESP32
}

void manejarMensajesNuevos(int numNuevosMensajes)
{
  for (int i = 0; i < numNuevosMensajes; i++) {
    String chat_id = String(bot.messages[i].chat_id);

    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Usuario no autorizado", "");
    } else {
      String texto = bot.messages[i].text;
      String nombre_usuario = bot.messages[i].from_name;
      if (nombre_usuario == "")
        nombre_usuario = "Invitado";

      if (texto == "/start") {
        String mensaje_bienvenida = "🤖 Bienvenido al bot de monitoreo del sensor DHT.\n";
        mensaje_bienvenida += "Comandos disponibles:\n";
        mensaje_bienvenida += "/temp - Obtener temperatura en Celsius y Fahrenheit.\n";
        mensaje_bienvenida += "/hum - Obtener humedad.\n";
        mensaje_bienvenida += "/todo - Mostrar temperatura y humedad.\n";
        bot.sendMessage(chat_id, mensaje_bienvenida, "Markdown");
      } else if (texto == "/temp") {
        String msg = "🌡️ La temperatura es:\n";
        msg += String(temperaturaC) + " °C\n";
        msg += String(temperaturaF) + " °F";
        bot.sendMessage(chat_id, msg, "");
      } else if (texto == "/hum") {
        String msg = "💧 La humedad es:\n";
        msg += String(humedad) + " %";
        bot.sendMessage(chat_id, msg, "");
      } else if (texto == "/todo") {
        String msg = "🌡️ La temperatura es:\n";
        msg += String(temperaturaC) + " °C\n";
        msg += String(temperaturaF) + " °F\n";
        msg += "💧 La humedad es:\n";
        msg += String(humedad) + " %";
        bot.sendMessage(chat_id, msg, "");
      } else {
        bot.sendMessage(chat_id, "❌ Comando no reconocido. Usa /start para ver los comandos disponibles.", "");
      }
    }
  }
}

void enviarAlerta()
{
  String mensaje_alerta = "⚠️ *Alerta de temperatura alta* ⚠️\n";
  mensaje_alerta += "La temperatura ha superado los 24 °C.\n";
  mensaje_alerta += "Temperatura actual: " + String(temperaturaC) + " °C\n";
  bot.sendMessage(CHAT_ID, mensaje_alerta, "Markdown");
}
