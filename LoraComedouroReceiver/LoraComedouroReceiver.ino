#include <WiFi.h>
#include <HTTPClient.h>
#include <LoRa.h>

// Configurações de rede Wi-Fi
const char* ssid = "LIT-HUAWEI-WIFI6";
const char* password = "obr@@ifce2323";

// URLs dos servidores backend
const char* serverUrlAlimentacao = "http://192.168.3.17:8888/alimentacao";  // Backend para dados de alimentação
const char* serverUrlGPS = "http://192.168.3.17:3333/api/gps-data";         // Backend para dados de GPS
const char* serverUrlVolume = "http://192.168.3.17:3334/api/volume-data";    // Backend para dados de volume

// Pinos do módulo LoRa
#define MISO 19  // GPIO19 -- SX1278's MISO
#define MOSI 27  // GPIO27 -- SX1278's MOSI
#define SCK 5    // GPIO5  -- SX1278's SCK
#define SS 18    // Chip Select do modulo LoRa
#define RST 14   // Reset do modulo LoRa
#define DI0 26   // Pino DI0 ou irqPin

void setup() {
  // Inicializa a comunicação serial para monitoramento
  Serial.begin(115200);

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");

  // Inicializa o módulo LoRa
  LoRa.setPins(SS, RST, DI0); // Configura os pinos do LoRa
  if (!LoRa.begin(915E6)) {   // Frequência 915 MHz (ajuste conforme necessário)
    Serial.println("Erro ao iniciar modulo LoRa. Verifique a conexao dos seus pinos!! ");
    while (true) {}           // Trava o código em caso de falha na inicialização
  }
  LoRa.setSpreadingFactor(12);  // Ajusta o spreading factor para alcance máximo (ajuste conforme necessário)
  Serial.println("LoRa Receiver inicializado com sucesso.");
}

void loop() {
  // Tenta receber um pacote LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Exibe o tamanho do pacote recebido
    Serial.print("Pacote recebido com tamanho: ");
    Serial.println(packetSize);

    // Lê o pacote
    String receivedText = "";
    while (LoRa.available()) {
      receivedText += (char)LoRa.read();
    }

    // Exibe o texto recebido no monitor serial
    Serial.print("Texto recebido: ");
    Serial.println(receivedText);

    // Verifica o tipo de dados recebidos
    if (receivedText.startsWith("alimentacao:")) {
      // Dados de alimentação
      handleAlimentacao(receivedText);
    } else if (receivedText.startsWith("Lat:") && receivedText.indexOf("Lng:") != -1) {
      // Dados de GPS
      handleGPS(receivedText);
    } else if (receivedText.startsWith("volume:")) {
      // Dados de volume
      handleVolume(receivedText);
    } else {
      Serial.println("Tipo de pacote desconhecido. Ignorado.");
    }

    // Exibe a força do sinal RSSI (Received Signal Strength Indicator)
    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());
  }

  // Delay para evitar sobrecarga de dados
  delay(1000);
}

// Função para lidar com pacotes de alimentação
void handleAlimentacao(String receivedText) {
  String dataHora = parseDataHora(receivedText, "alimentacao:");
  int alimentacao = parseAlimentacao(receivedText, "Alimentacao: ");
  
  // Verifica se dataHora e alimentacao foram extraídos corretamente
  if (alimentacao != -1 && dataHora != "") {
    String payload = "{\"data_hora\":\"" + dataHora + "\", \"alimentacao\":" + String(alimentacao) + "}";
    Serial.println("Enviando para o backend dados de alimentação: " + payload);
    enviarParaBackend(serverUrlAlimentacao, payload);
  } else {
    Serial.println("Erro ao analisar o pacote de alimentação. Dados inválidos.");
  }
}

// Função para lidar com pacotes de GPS
void handleGPS(String receivedText) {
  float latitude = parseData(receivedText, "Lat: ");
  float longitude = parseData(receivedText, "Lng: ");

  // Verifica se os valores de latitude e longitude são válidos
  if (!isnan(latitude) && !isnan(longitude)) {
    String payload = "{\"latitude\": " + String(latitude, 6) + ", \"longitude\": " + String(longitude, 6) + "}";

    // Verifica se o payload está correto antes de enviar
    if (payload.length() > 0) {
      Serial.println("Enviando para o backend dados de GPS: " + payload);
      enviarParaBackend(serverUrlGPS, payload);
    } else {
      Serial.println("Erro na criação do payload de GPS. Payload vazio ou inválido.");
    }
  } else {
    Serial.println("Erro ao analisar os dados do pacote LoRa. Dados inválidos.");
  }
}

// Função para lidar com pacotes de volume
void handleVolume(String receivedText) {
  float volumePercentual = parseVolume(receivedText);

  // Verifica se o valor de volume é válido
  if (!isnan(volumePercentual)) {
    String payload = "{\"volume_percentual\": " + String(volumePercentual, 2) + "}";

    // Verifica se o payload está correto antes de enviar
    if (payload.length() > 0) {
      Serial.println("Enviando para o backend dados de volume: " + payload);
      enviarParaBackend(serverUrlVolume, payload);
    } else {
      Serial.println("Erro na criação do payload de volume. Payload vazio ou inválido.");
    }
  } else {
    Serial.println("Erro ao analisar os dados do pacote LoRa. Volume inválido.");
  }
}

// Função para enviar os dados ao backend específico
void enviarParaBackend(const char* serverUrl, String payload) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Imprime o payload para debug
    Serial.println("Payload a ser enviado: " + payload);

    // Envia a requisição POST com os dados
    int httpResponseCode = http.POST(payload);

    // Verifica a resposta do servidor
    if (httpResponseCode > 0) {
      Serial.print("Código de resposta HTTP: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println("Resposta do servidor: " + response);
    } else {
      Serial.print("Erro na requisição: ");
      Serial.println(httpResponseCode);
      Serial.println("Falha ao enviar dados para o backend. Verifique o servidor e o formato do payload.");
    }

    // Fecha a conexão
    http.end();
  } else {
    Serial.println("WiFi desconectado, não foi possível enviar dados para o backend.");
  }
}

// Função para extrair valor de `data_hora` como string no formato "YYYY-MM-DD HH:MM:SS"
String parseDataHora(String data, String key) {
  int startIndex = data.indexOf(key);
  if (startIndex == -1) {
    return "";  // Retorna string vazia se a chave não for encontrada
  }

  startIndex += key.length();
  int endIndex = data.indexOf(',', startIndex); // Posição da vírgula antes de "Alimentacao"
  if (endIndex == -1) {
    return "";  // Retorna string vazia se a vírgula não for encontrada
  }

  // Extrai a substring e remove espaços em branco no início e no final
  String extractedData = data.substring(startIndex, endIndex);
  extractedData.trim();  // Remove espaços em branco no início e no final

  return extractedData;  // Retorna a string no formato "YYYY-MM-DD HH:MM:SS"
}

// Função para extrair o valor de alimentação como inteiro da string recebida
int parseAlimentacao(String data, String key) {
  int startIndex = data.indexOf(key);
  if (startIndex == -1) {
    return -1;  // Retorna -1 se a chave não for encontrada
  }

  startIndex += key.length();
  int endIndex = data.indexOf(',', startIndex);
  if (endIndex == -1) {
    endIndex = data.length();
  }

  // Extrai a substring e remove espaços em branco no início e no final
  String extractedData = data.substring(startIndex, endIndex);
  extractedData.trim();  // Remove espaços em branco no início e no final
  
  return extractedData.toInt();  // Converte a string extraída para int
}

// Função para extrair valores de Latitude e Longitude como float da string recebida
float parseData(String data, String key) {
  int startIndex = data.indexOf(key);
  if (startIndex == -1) {
    return NAN;  // Retorna NaN (Not a Number) se a chave não for encontrada
  }

  startIndex += key.length();
  int endIndex = data.indexOf(',', startIndex);
  if (endIndex == -1) {
    endIndex = data.length();
  }

  // Extrai a substring e remove espaços em branco no início e no final
  String extractedData = data.substring(startIndex, endIndex);
  extractedData.trim();  // Remove espaços em branco no início e no final
  
  return extractedData.toFloat();  // Converte a string extraída para float
}

// Função para extrair o valor do volume como float da string recebida
float parseVolume(String data) {
  int startIndex = data.indexOf("volume:") + 7;
  return data.substring(startIndex).toFloat();
}
