#include <math.h>

#define botao 7
#define bombaPin 8
#define ledModo 12
#define Bomba 11
#define sensorMin A1
#define sensorMax A2

// --- Variáveis globais ---
bool estadoBotaoEstavel = HIGH;
bool ultimoEstadoLido = HIGH;
unsigned long ultimoTempoTroca = 0;
const unsigned long tempoDebounce = 50;

bool modoAutomatico = false;
bool bombaLigada = false;

// --- Estados do poço ---
enum EstadoPoco { POCO_CHEIO, POCO_DESCENDO, POCO_VAZIO, POCO_ENCHENDO, SENSOR_FALHA };
EstadoPoco estadoAtual = POCO_VAZIO;
EstadoPoco estadoAnterior = POCO_VAZIO;

float calibrationFactor = 242.0;
const float LIMITE_MOLHADO = 7.0;
unsigned long ultimaLeitura = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("=== Sistema de Controle de Poço Artesiano com Estados ===");
  Serial.println("💤 Sistema pronto — Bomba desligada");
  Serial.println("Aperte o botão para ligar o sistema 🟢");

  pinMode(botao, INPUT_PULLUP);
  pinMode(bombaPin, OUTPUT);
  pinMode(ledModo, OUTPUT);
  pinMode(Bomba, OUTPUT);

  digitalWrite(bombaPin, LOW);
  digitalWrite(ledModo, LOW);
  digitalWrite(Bomba, LOW);
}

// --- Funções ---
bool tempoDecorrido(unsigned long intervalo);
float lerTensaoAC(int pinoSensor);
void atualizarEstado(float tensaoMin, float tensaoMax);
void controlarBomba();
void ligarBomba();
void desligarBomba();
bool verificarBotao();

void loop() {
  if (verificarBotao() && estadoBotaoEstavel == LOW) {
    modoAutomatico = !modoAutomatico;
    digitalWrite(ledModo, modoAutomatico ? HIGH : LOW);

    if (modoAutomatico)
      Serial.println("⚙️ Sistema ligado — modo automático");
    else {
      Serial.println("💤 Sistema pronto — bomba desligada");
      desligarBomba();
    }
  }

  if (modoAutomatico && tempoDecorrido(1000)) {
    float tensaoMin = lerTensaoAC(sensorMin);
    float tensaoMax = lerTensaoAC(sensorMax);
    atualizarEstado(tensaoMin, tensaoMax);
    controlarBomba();
  }
}

// --- Função de temporização ---
bool tempoDecorrido(unsigned long intervalo) {
  unsigned long agora = millis();
  if (agora - ultimaLeitura >= intervalo) {
    ultimaLeitura = agora;
    return true;
  }
  return false;
}

// --- Leitura do sensor ZMPT101B ---
float lerTensaoAC(int pinoSensor) {
  const int amostras = 500;
  long soma = 0;
  for (int i = 0; i < amostras; i++) {
    int leituraBruta = analogRead(pinoSensor);
    int centrado = leituraBruta - 512;
    soma += (long)centrado * centrado;
  }
  float media = soma / (float)amostras;
  float rms = sqrt(media);
  float tensaoSensor = (rms * 5.0) / 1024.0;
  return tensaoSensor * calibrationFactor;
}

// --- Atualiza o estado do poço ---
void atualizarEstado(float tensaoMin, float tensaoMax) {
  bool eminMolhado = tensaoMin <= LIMITE_MOLHADO;
  bool emaxMolhado = tensaoMax <= LIMITE_MOLHADO;

  Serial.print("Vmin: ");
  Serial.print(tensaoMin, 1);
  Serial.print(" V | Vmax: ");
  Serial.print(tensaoMax, 1);
  Serial.print(" V | ");

  estadoAnterior = estadoAtual;

  // --- Verificação de falha de sensor ---
  if (!eminMolhado && emaxMolhado) {
    estadoAtual = SENSOR_FALHA;
    Serial.println("🚨 🚨 Falha detectada: Sensor inferior com defeito"); 
    return;
  }

  switch (estadoAtual) {
    case POCO_VAZIO:
      if (eminMolhado && emaxMolhado) {
        estadoAtual = POCO_CHEIO;
        Serial.println("💧 Poço cheio — pronto para bombear");
      } else if (eminMolhado && !emaxMolhado) {
        estadoAtual = POCO_ENCHENDO;
        Serial.println("🔄 Poço começando a encher...");
      } else {
        Serial.println("⚠️ Poço vazio — aguardando enchimento ou 🚨 Falha elétrica: Verifique alimentação AC, relé ou disjuntor!");
      }
      break;

    case POCO_ENCHENDO:
      if (eminMolhado && emaxMolhado) {
        estadoAtual = POCO_CHEIO;
        Serial.println("💧 Poço cheio — pronto para bombear");
      } else if (!eminMolhado && !emaxMolhado) {
        estadoAtual = POCO_VAZIO;
        Serial.println("⚠️ Poço ainda vazio");
      } else {
        Serial.println("⏳ Enchendo...");
      }
      break;

    case POCO_CHEIO:
      if (eminMolhado && !emaxMolhado) {
        estadoAtual = POCO_DESCENDO;
        Serial.println("⏬ Nível descendo...");
      } else if (!eminMolhado && !emaxMolhado) {
        estadoAtual = POCO_VAZIO;
        Serial.println("⚠️ Poço secou!");
      } else {
        Serial.println("💧 Mantendo estado: cheio");
      }
      break;

    case POCO_DESCENDO:
      if (!eminMolhado && !emaxMolhado) {
        estadoAtual = POCO_VAZIO;
        Serial.println("⚠️ Nível crítico — poço vazio");
      } else if (eminMolhado && emaxMolhado) {
        estadoAtual = POCO_CHEIO;
        Serial.println("💧 Poço reabastecido");
      } else {
        Serial.println("⏳ Poço ainda com água");
      }
      break;

    case SENSOR_FALHA:
      // Mantém o estado até a falha desaparecer
      if (eminMolhado || !emaxMolhado) {
        Serial.println("✅ Falha resolvida — voltando à leitura normal");
        estadoAtual = POCO_VAZIO;  // volta à detecção normal
      } 
      else {
        Serial.println("🚨 Aguardando correção do sensor inferior...");
      }
      break;
  }

  if (estadoAtual != estadoAnterior) {
    Serial.print("📊 Transição: ");
    Serial.print(estadoAnterior);
    Serial.print(" ➜ ");
    Serial.println(estadoAtual);
  }
}

// --- Controle da bomba baseado no estado do poço ---
void controlarBomba() {
  if (estadoAtual == SENSOR_FALHA) {
    desligarBomba();
    return;
  }

  switch (estadoAtual) {
    case POCO_CHEIO:
    case POCO_DESCENDO:
      ligarBomba();
      break;
    case POCO_VAZIO:
    case POCO_ENCHENDO:
      desligarBomba();
      break;
    default:
      break;
  }

  Serial.print("Estado atual: ");
  switch (estadoAtual) {
    case POCO_CHEIO: Serial.println("POÇO CHEIO"); break;
    case POCO_DESCENDO: Serial.println("POÇO DESCENDO"); break;
    case POCO_ENCHENDO: Serial.println("POÇO ENCHENDO"); break;
    case POCO_VAZIO: Serial.println("POÇO VAZIO"); break;
    case SENSOR_FALHA: Serial.println("SENSOR COM DEFEITO"); break;
  }
}

// --- Liga/Desliga bomba ---
void ligarBomba() {
  if (!bombaLigada) {
    bombaLigada = true;
    digitalWrite(bombaPin, HIGH);
    digitalWrite(Bomba, HIGH);
    Serial.println("✅ Bomba LIGADA");
  }
}

void desligarBomba() {
  if (bombaLigada) {
    bombaLigada = false;
    digitalWrite(bombaPin, LOW);
    digitalWrite(Bomba, LOW);
    Serial.println("⚠️ Bomba DESLIGADA");
  }
}

// --- Botão com debounce ---
bool verificarBotao() {
  bool leituraAtual = digitalRead(botao);
  unsigned long agora = millis();

  if (leituraAtual != ultimoEstadoLido) {
    ultimoTempoTroca = agora;
    ultimoEstadoLido = leituraAtual;
  }

  if ((agora - ultimoTempoTroca) > tempoDebounce) {
    if (leituraAtual != estadoBotaoEstavel) {
      estadoBotaoEstavel = leituraAtual;
      return true;
    }
  }
  return false;
}
