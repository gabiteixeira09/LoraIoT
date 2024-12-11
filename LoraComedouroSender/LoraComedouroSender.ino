#include <Wire.h>
#include <RTClib.h>
#include <ESP32Servo.h>
#include <TinyGPSPlus.h>
#include <LoRa.h>

const int trigPin = 12; 
const int echoPin = 13; 
const float alturaRecipiente = 10.0; 

RTC_DS3231 rtc;
Servo meuServo;
TinyGPSPlus gps;

const int servoPin = 23;    // Pino para o controle do servo
const int rtcSDA = 21;      // Pino SDA do RTC
const int rtcSCL = 22;      // Pino SCL do RTC
const int gpsRX = 16;       // Pino RX do GPS
const int gpsTX = 17;       // Pino TX do GPS

#define MISO 19  // GPIO19 -- SX1278's MISO
#define MOSI 27  // GPIO27 -- SX1278's MOSI
#define SCK 5    // GPIO5  -- SX1278's SCK
#define SS 18    // Chip Select do modulo LoRa
#define RST 14   // Reset do modulo LoRa
#define DI0 26   // Pino DI0 ou irqPin

#define LORA_FREQUENCY 915E6  

const int horarioAlimentacao1[2] = {7, 30};  // Primeira alimentação
const int horarioAlimentacao2[2] = {17, 30}; // Segunda alimentação

bool alimentacao1Realizada = false;
bool alimentacao2Realizada = false;

int counter = 0;

void setup() {
  // Inicializa o monitor serial
  Serial.begin(115200);

  // Configura os pinos do sensor ultrassônico
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Inicializa a comunicação I2C e o RTC
  Wire.begin(rtcSDA, rtcSCL);
  if (!rtc.begin()) {
    Serial.println("RTC não encontrado. Verifique as conexões.");
    while (1);
  }

  // Se o RTC perder energia, a hora precisa ser reajustada
  if (rtc.lostPower()) {
    Serial.println("RTC perdeu a energia, ajustando a hora!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Inicializa o servo no pino especificado
  meuServo.attach(servoPin);
  meuServo.write(0);  // Move o servo para a posição inicial (0 graus)

  // Inicializa a comunicação serial com o módulo GPS
  Serial1.begin(9600, SERIAL_8N1, gpsRX, gpsTX);
  Serial.println("Iniciando leitura do GPS...");

  // Inicializa o módulo LoRa
  LoRa.setPins(SS, RST, DI0); 
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("Erro ao iniciar modulo LoRa.");
    while (true);
  }
  LoRa.setTxPower(20);  
  LoRa.setSpreadingFactor(12);
  Serial.println("Modulo LoRa inicializado com sucesso.");
}

void loop() {
  // Lê os dados do GPS
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    gps.encode(c);
  }

  // Verifica se há dados válidos de localização
  if (gps.location.isValid()) {
    String latitude = String(gps.location.lat(), 6); 
    String longitude = String(gps.location.lng(), 6); 

    Serial.print("Latitude: ");
    Serial.println(latitude);
    Serial.print("Longitude: ");
    Serial.println(longitude);

    // Envia as coordenadas via LoRa
    sendLoRaPacket(latitude, longitude);
  } else {
    Serial.println("Localização não válida. Tentando novamente...");
  }

  // Obtém a data e hora atuais do RTC
  DateTime now = rtc.now();

  // Verifica se é hora de realizar a primeira alimentação
  if (now.hour() == horarioAlimentacao1[0] && now.minute() == horarioAlimentacao1[1] && !alimentacao1Realizada) {
    realizarAlimentacao(now, 1); 
    alimentacao1Realizada = true;
  }

  // Verifica se é hora de realizar a segunda alimentação
  if (now.hour() == horarioAlimentacao2[0] && now.minute() == horarioAlimentacao2[1] && !alimentacao2Realizada) {
    realizarAlimentacao(now, 2); 
    alimentacao2Realizada = true;
  }

  // Verifica se é um novo dia e reinicia as variáveis de alimentação
  if (now.hour() == 0 && now.minute() == 0 && (alimentacao1Realizada || alimentacao2Realizada)) {
    alimentacao1Realizada = false;
    alimentacao2Realizada = false;
    Serial.println("Novo dia detectado. Resetando status de alimentações.");
  }

  // Função do sensor ultrassônico para medir o volume
  float distancia = medirDistancia();
  float volumePercentual = calcularVolume(distancia);
  enviarLoRaVolume(volumePercentual);

  // Delay para evitar sobrecarga de dados
  delay(20000);
}

// Função para medir a distância usando o sensor ultrassônico
float medirDistancia() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duracao = pulseIn(echoPin, HIGH);
  float distancia = (duracao * 0.0343) / 2;

  return distancia;
}

// Função para calcular o volume restante em percentual
float calcularVolume(float distancia) {
  if (distancia > alturaRecipiente) {
    distancia = alturaRecipiente; // Limita a distância ao valor máximo
  }
  return ((alturaRecipiente - distancia) / alturaRecipiente) * 100;
}

// Função para enviar o volume restante via LoRa
void enviarLoRaVolume(float volumePercentual) {
  Serial.print("Enviando volume via LoRa: ");
  Serial.print(volumePercentual);
  Serial.println(" %");

  LoRa.beginPacket();
  LoRa.print("volume:");
  LoRa.print(volumePercentual);
  LoRa.endPacket();
}

void realizarAlimentacao(const DateTime& now, int alimentacao) {
  // Mover o servo para a posição de alimentação
  meuServo.write(180);
  delay(1500);  
  meuServo.write(0);

  String dataHora = now.timestamp(DateTime::TIMESTAMP_FULL);
  Serial.println("Alimentação realizada em: " + dataHora + " - Alimentacao número: " + String(alimentacao));

  // Enviar dados de alimentação via LoRa
  LoRa.beginPacket();
  LoRa.print("alimentacao:");
  LoRa.print(dataHora);
  LoRa.print(", Alimentacao: ");
  LoRa.print(alimentacao);
  
  int status = LoRa.endPacket();
  if (status == 1) {
    Serial.println("Pacote de alimentação enviado com sucesso.");
  } else {
    Serial.println("Erro ao enviar o pacote de alimentação.");
  }
  
  // Pequeno delay para garantir que o pacote foi completamente enviado antes de outras operações
  delay(100);
}

void sendLoRaPacket(String lat, String lng) {
  Serial.print("Enviando pacote LoRa com coordenadas: ");
  Serial.print("Latitude: ");
  Serial.print(lat);
  Serial.print(", Longitude: ");
  Serial.println(lng);

  LoRa.beginPacket();
  LoRa.print("Lat: ");
  LoRa.print(lat);
  LoRa.print(", Lng: ");
  LoRa.print(lng);
  LoRa.endPacket();
}
