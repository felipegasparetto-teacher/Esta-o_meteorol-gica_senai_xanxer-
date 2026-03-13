/*
ESTE CÓDIGO É PARA VERIFICAÇÃO DA CONEXÃO ENTRE A PLATAFORMA TAGO-IO, E A PLACA 
ESP-32, SENDO NECESSÁRIA A CRIAÇÃO DE UMA CONTA NESTA PLATAFORMA, https://tago.io/
AS ALTERAÇÕES PARA OUTRAS PLATAFORMAS FICAM A CARGO DE QUEM UTILIZAR O CÓDIGO,
ESTE EM ESPECÍFICO É FEITO PARA FINS EDUCACIONAIS APENAS. 
*/


#include <WiFi.h>
#include <HTTPClient.h> //Localize a biblioteca PubSubClient feita por Nick O'Leary
#include <ArduinoJson.h> //Localize a biblioteca feita por Benoit Blanchon.

// --- Configurações de Rede ---
const char* ssid = "SSID_DA_REDE";
const char* password = "PASSWORD_DA_REDE";

// --- Configurações TagoIO ---
const char* serverName = "http://api.tago.io/data";
const char* device_token = "SEU TOKEN DO TAGO IO";

// --- Configurações Deep Sleep ---
#define uS_TO_S_FACTOR 1000000ULL  /* Fator de conversão de micro para segundos */
#define TIME_TO_SLEEP  120         /* Tempo de sono alterado para 2 minutos (2 * 60 segundos) */

void setup() {
  Serial.begin(115200);
  delay(1000); // Pausa para estabilidade do Serial

  // Conectar WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Conectado!");
    
    // Chamamos a lógica de envio dentro do setup pois o Deep Sleep reinicia o chip
    executarEnvio();
  } else {
    Serial.println("\nFalha na conexão WiFi.");
  }

  // --- Implementação do Deep Sleep ---
  // Configura o ESP32 para acordar daqui a 2 minutos
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  
  Serial.println("Entrando em Deep Sleep por 2 minutos...");
  Serial.flush(); 
  esp_deep_sleep_start();
}

void loop() {
  // Com Deep Sleep, o código nunca chegará aqui.
}

void executarEnvio() {
    HTTPClient http;

    // Inicia a conexão no endpoint do TagoIO
    http.begin(serverName);

    // Adiciona os Headers necessários
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Device-Token", device_token);

    // 1. Inicializa a semente aleatória usando o tempo atual
    // Isso garante que a sequência de números mude a cada execução
    randomSeed(millis() + analogRead(0)); 

    // 2. Gera os valores usando millis() para criar oscilações
    float temp = 20.0 + (random(0, 100) / 10.0);       // Base 20°C + aleatório 0-10
    float hum  = 50.0 + (random(0, 400) / 10.0);      // Base 50% + aleatório 0-40
    float wind_speed = (millis() % 50) + (random(0, 50) / 10.0); // Usa o resto da divisão de millis

    // 3. Sorteia a direção do vento usando o millis para escolher o índice
    String direcoes[] = {"Norte", "Sul", "Leste", "Oeste", "Nordeste", "Sudeste"};
    int indice = (millis() / 1000) % 6; // Muda o índice baseado nos segundos
    String wind_dir = direcoes[indice];

    // 4. Chuva: simula que só chove se o millis for par e o random for alto
    float rain_mm = (millis() % 50) + (random(0, 50) / 10.0);

    Serial.print("Dados gerados com millis: ");
    Serial.println(temp);

    // --- Construção do JSON (Mesma estrutura anterior) ---
    JsonDocument doc; // Na V7 basta usar JsonDocument
    JsonArray root = doc.to<JsonArray>();

    // Função auxiliar para evitar repetição de código
    auto addVar = [&](String v, float val, String unit = "") {
      JsonObject obj = root.createNestedObject();
      obj["variable"] = v;
      obj["value"] = val;
      if (unit != "") obj["unit"] = unit;
    };

    // Mantendo unidades simples para evitar erro de codificação
    addVar("Temperatura_Atual", temp, "°C");
    addVar("Umidade_Atual", hum, "%");
    addVar("Velocidade_do_vento", wind_speed, "km/h");
    addVar("Quantidade_de_Chuva", rain_mm, "mm");
    
    // Para string (direção do vento)
    JsonObject var4 = root.createNestedObject();
    var4["variable"] = "direcao_vento";
    var4["value"] = wind_dir;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Enviar o POST
    Serial.print("Enviando via HTTP (Intervalo 2min): ");
    Serial.println(jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Código de resposta: " + String(httpResponseCode));
      Serial.println("Resposta do Tago: " + response);
    } else {
      Serial.print("Erro no envio: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Fecha a conexão
}
