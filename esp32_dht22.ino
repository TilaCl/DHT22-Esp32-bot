#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Definiciones del sensor DHT
#define DHTPIN 22
#define DHTTYPE DHT22

// Configuración Wi-Fi
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "CONTRASEÑA"

// Token del Bot de Telegram
#define BOT_TOKEN " TOKEN "

// Tiempo de espera para las actualizaciones del bot (en segundos)
#define TIEMPO_ENTRE_ESCANEOS 1 // 1 segundo

// Tiempo de verificación Wi-Fi (en segundos)
#define TIEMPO_ENTRE_VERIFICACIONES_WIFI 600 // 10 minutos

// Lista de usuarios conectados (se puede agregar manualmente o el bot la va actualizando)
std::vector<String> usuarios_activos = {"123456789", " tu id "}; // Agregar chat_id manualmente

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

DHT dht(DHTPIN, DHTTYPE);

unsigned long ultima_vez_bot = 0;
unsigned long ultima_vez_estado_wifi = 0;
float temperaturaC;
float temperaturaF;
float humedad;
bool estado_wifi_previo = true; // Estado previo de la conexión Wi-Fi

// Función para conectar a Wi-Fi
void conectarWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Conectando a Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    secured_client.setInsecure();
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
    }
    Serial.println("\nConexión Wi-Fi exitosa. Dirección IP:");
    Serial.println(WiFi.localIP());
  }
}

// Función para leer el sensor DHT
void leerSensor() {
  humedad = dht.readHumidity();
  temperaturaC = dht.readTemperature();
  temperaturaF = dht.readTemperature(true);

  if (isnan(humedad) || isnan(temperaturaC) || isnan(temperaturaF)) {
    Serial.println("Error al leer el sensor DHT. Revisar conexiones.");
    delay(10000); // Esperar 10 segundos antes de reintentar
    return;
  }
}

// Función para enviar alerta a todos los usuarios activos
void enviarAlertaATodos(String mensaje) {
  for (String chat_id : usuarios_activos) {
    bot.sendMessage(chat_id, mensaje, "Markdown");
  }
}

// Función para manejar los mensajes del bot
void manejarMensajesNuevos(int numNuevosMensajes) {
  for (int i = 0; i < numNuevosMensajes; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String texto = bot.messages[i].text;
    String nombre_usuario = bot.messages[i].from_name;

    if (nombre_usuario == "") {
      nombre_usuario = "Invitado";
    }

    // Registrar el chat_id en la lista de usuarios activos
    if (std::find(usuarios_activos.begin(), usuarios_activos.end(), chat_id) == usuarios_activos.end()) {
      usuarios_activos.push_back(chat_id);
    }

    if (texto == "/start") {
      String mensaje_bienvenida = "🤖 Bienvenido al bot de monitoreo del sensor DHT.\n";
      mensaje_bienvenida += "Comandos disponibles:\n";
      mensaje_bienvenida += "/temp - Obtener temperatura en Celsius y Fahrenheit.\n";
      mensaje_bienvenida += "/hum - Obtener humedad.\n";
      mensaje_bienvenida += "/todo - Mostrar temperatura y humedad.\n";
      mensaje_bienvenida += "/usuarios - Ver usuarios conectados.\n"; // Comando agregado para mostrar usuarios
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
    } else if (texto == "/usuarios") {  // Comando para mostrar los usuarios conectados
      String usuarios = "Usuarios conectados:\n";
      for (String id : usuarios_activos) {
        usuarios += id + "\n";
      }
      bot.sendMessage(chat_id, usuarios, "");
    } else {
      bot.sendMessage(chat_id, "❌ Comando no reconocido. Usa /start para ver los comandos disponibles.", "");
    }
  }
}

// Función para verificar la conexión Wi-Fi
void verificarEstadoWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    if (estado_wifi_previo) {
      enviarAlertaATodos("⚠️ *Desconexión de Wi-Fi* ⚠️\nEl dispositivo se ha desconectado de la red Wi-Fi.");
      estado_wifi_previo = false;
    }
  } else {
    if (!estado_wifi_previo) {
      enviarAlertaATodos("✅ *Reconexión de Wi-Fi* ✅\nEl dispositivo se ha reconectado a la red Wi-Fi.");
      estado_wifi_previo = true;
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Iniciando prueba del sensor DHT..."));
  dht.begin();
  conectarWiFi();
}

void loop() {
  // Leer los datos del sensor
  leerSensor();

  // Enviar alerta si la temperatura supera los 24 °C
  if (temperaturaC > 24.0) {
    enviarAlertaATodos("⚠️ *Alerta de temperatura alta* ⚠️\nLa temperatura ha superado los 24 °C.\nTemperatura actual: " + String(temperaturaC) + " °C");
    delay(60000); // Esperar 10 segundos antes de la próxima comprobación
  }

  // Verificar estado de Wi-Fi cada 10 minutos
  unsigned long tiempo_actual = (unsigned long) (time(NULL)); // Obtener el tiempo actual en segundos
  if (tiempo_actual - ultima_vez_estado_wifi >= TIEMPO_ENTRE_VERIFICACIONES_WIFI) {
    verificarEstadoWiFi();
    ultima_vez_estado_wifi = tiempo_actual;
  }

  // Procesar mensajes del bot
  if (tiempo_actual - ultima_vez_bot > TIEMPO_ENTRE_ESCANEOS) {
    int numNuevosMensajes = bot.getUpdates(bot.last_message_received + 1);
    while (numNuevosMensajes) {
      manejarMensajesNuevos(numNuevosMensajes);
      numNuevosMensajes = bot.getUpdates(bot.last_message_received + 1);
    }
    ultima_vez_bot = tiempo_actual;
  }

  delay(1000); // Reducir carga innecesaria en el ESP32
}
