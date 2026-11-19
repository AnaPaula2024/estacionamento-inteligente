#include <WiFi.h>
#include <WebServer.h>

// Configurações da rede Wi-Fi criada pelo ESP32
const char* ssid = "Estacionamento_Inteligente";
const char* password = "admin123";

WebServer server(80);

struct Vaga {
  int pinoTrig;
  int pinoEcho;
  int pinoLedVerde;
  int pinoLedVermelho;
  bool ocupada;
};

const int NUM_VAGAS = 3;
Vaga vagas[NUM_VAGAS] = {
  {33, 32, 25, 26, false},
  {27, 14, 13, 4, false},
  {16, 17, 18, 19, false}
};

const int DISTANCIA_LIMITE = 5;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Estacionamento Inteligente</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f9; margin: 0; padding: 20px; }
    h1 { color: #333; }
    .container { display: flex; flex-wrap: wrap; justify-content: center; gap: 20px; margin-top: 30px; }
    .vaga { width: 120px; padding: 20px; border-radius: 12px; box-shadow: 0 4px 8px rgba(0,0,0,0.2); transition: 0.3s; }
    .livre { background-color: #4CAF50; color: white; border: 3px solid #388E3C; }
    .ocupada { background-color: #F44336; color: white; border: 3px solid #D32F2F; }
    .vaga h2 { margin: 0 0 10px 0; font-size: 1.5em; }
    .vaga p { margin: 0; font-weight: bold; font-size: 1.2em; }
  </style>
</head>
<body>
  <h1>Painel de Vagas</h1>
  <div class="container" id="painel-vagas" aria-live="polite"></div>
  <script>
    async function atualizarVagas() {
      try {
        const response = await fetch('/status');
        if (!response.ok) throw new Error('Falha ao consultar status');
        const data = await response.json();
        document.getElementById('painel-vagas').innerHTML = data.map((vaga, index) => {
          const classe = vaga.ocupada ? 'ocupada' : 'livre';
          const texto = vaga.ocupada ? 'Ocupada' : 'Livre';
          return `<div class="vaga ${classe}"><h2>Vaga ${index + 1}</h2><p>${texto}</p></div>`;
        }).join('');
      } catch (error) {
        console.error('Erro na leitura:', error);
      }
    }
    setInterval(atualizarVagas, 1000);
    atualizarVagas();
  </script>
</body>
</html>
)rawliteral";

long lerDistancia(int pinoTrig, int pinoEcho) {
  digitalWrite(pinoTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinoTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinoTrig, LOW);

  const long duracao = pulseIn(pinoEcho, HIGH, 30000);
  if (duracao == 0) return 999;
  return (duracao * 0.0343) / 2;
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_VAGAS; i++) {
    pinMode(vagas[i].pinoTrig, OUTPUT);
    pinMode(vagas[i].pinoEcho, INPUT);
    pinMode(vagas[i].pinoLedVerde, OUTPUT);
    pinMode(vagas[i].pinoLedVermelho, OUTPUT);
    digitalWrite(vagas[i].pinoLedVerde, HIGH);
    digitalWrite(vagas[i].pinoLedVermelho, LOW);
  }

  Serial.println("Configurando Access Point...");
  WiFi.softAP(ssid, password);
  Serial.print("Endereço IP do Painel: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html; charset=utf-8", index_html);
  });

  server.on("/status", HTTP_GET, []() {
    String json = "[";
    for (int i = 0; i < NUM_VAGAS; i++) {
      json += "{\"ocupada\":";
      json += vagas[i].ocupada ? "true" : "false";
      json += "}";
      if (i < NUM_VAGAS - 1) json += ",";
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Servidor web iniciado!");
}

void loop() {
  server.handleClient();

  for (int i = 0; i < NUM_VAGAS; i++) {
    const long distancia = lerDistancia(vagas[i].pinoTrig, vagas[i].pinoEcho);
    vagas[i].ocupada = distancia > 0 && distancia <= DISTANCIA_LIMITE;
    digitalWrite(vagas[i].pinoLedVerde, vagas[i].ocupada ? LOW : HIGH);
    digitalWrite(vagas[i].pinoLedVermelho, vagas[i].ocupada ? HIGH : LOW);
  }

  delay(100);
}
