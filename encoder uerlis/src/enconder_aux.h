#include <Arduino.h>

// ===== CONTROLE PWM DO MOTOR =====
const int PWM_PIN = 9;                 // Pino PWM
int potencia = 100;                      // Potência em % (0-100)

// ===== SENSOR ENCODER =====
const int ENCODER_PIN = 2;             // Pino com interrupção (INT0)
const int PULSOS_POR_ROTACAO = 20;     // 20 imãs → 20 pulsos por rotação

// Diâmetro do círculo dos ímãs (mm) e raio em metros (para velocidade tangencial)
const float DIAMETRO_MAGNETOS_MM = 52.5; 
const float RAIO_MAGNETOS_M = (DIAMETRO_MAGNETOS_MM / 2.0) / 1000.0; // m
volatile long contador_pulsos = 0;     // Volátil para ISR

// ===== VARIÁVEIS DE TEMPO =====
unsigned long tempo_anterior = 0;
unsigned long intervalo_envio = 100;   // Enviar dados a cada 100ms
unsigned long tempo_inicio = 0;
const unsigned long TEMPO_POR_STEP_MS = 1000; // Duracao de cada degrau de potencia em ms

// ===== CONSTANTES =====
const float CONSTANTE_CONVERSAO = (60000.0 / PULSOS_POR_ROTACAO);
const float PI_2 = 2.0 * 3.14159265359;


// ===== FUNÇÕES - INTERRUPÇÃO =====

// ISR - Interrupção ao detectar pulso do encoder
void contarPulso() {
  contador_pulsos++;
}

// ===== FUNÇÕES - ENCODER =====

float calcularRPM(long pulsos, unsigned long tempo_ms) {
  if (tempo_ms == 0) return 0;
  return (pulsos * CONSTANTE_CONVERSAO) / tempo_ms;
}
float calcularOmega(float rpm) {
  return rpm * PI_2 / 60.0;
}
void enviarDados() {
  unsigned long agora = millis();
  unsigned long tempo_atual = agora - tempo_inicio;
  unsigned long delta_t = agora - tempo_anterior;
  long pulsos = contador_pulsos;
  
  float rpm = calcularRPM(pulsos, delta_t);
  float omega = calcularOmega(rpm);
  
  // Envia: tempo_ms,rpm,omega_rad_s
  Serial.print(tempo_atual);
  Serial.print(",");
  Serial.print(rpm, 2);
  Serial.print(",");
  Serial.println(omega, 2);
  
  contador_pulsos = 0;
  tempo_anterior = agora;
}

// ===== FUNÇÕES - CONTROLE PWM =====

void exibirBarra(int pct) {
  int blocos = pct / 5;
  Serial.print(F("  ["));
  for (int i = 0; i < 20; i++) {
    Serial.print(i < blocos ? "#" : ".");
  }
  Serial.print(F("] "));
  Serial.print(pct);
  Serial.println(F("%"));
}
void definirPotencia(int novaPotencia) {
  novaPotencia = constrain(novaPotencia, 0, 100);
  potencia = novaPotencia;

  int pwmValor = (potencia == 0) ? 0 : map(potencia, 0, 100, 51, 255);
  analogWrite(PWM_PIN, pwmValor);

  float vgs = (pwmValor / 255.0) * 5.0;
  float tensaoMedia = (potencia / 100.0) * 12.0;

  Serial.print(F("[PWM] Potencia: "));
  Serial.print(potencia);
  Serial.print(F("% | PWM: "));
  Serial.print(pwmValor);
  Serial.print(F("/255 | VGS: ~"));
  Serial.print(vgs, 2);
  Serial.print(F("V | Carga: ~"));
  Serial.print(tensaoMedia, 1);
  Serial.println(F("V"));

  exibirBarra(potencia);
}
void exibirStatus() {
  int pwmValor = map(potencia, 0, 100, 0, 255);
  Serial.println(F("--- Status Atual ---"));
  Serial.print(F("  Pino PWM : "));
  Serial.println(PWM_PIN);
  Serial.print(F("  Potência : "));
  Serial.print(potencia);
  Serial.println(F("%"));
  Serial.print(F("  Valor PWM: "));
  Serial.print(pwmValor);
  Serial.println(F(" / 255"));
  exibirBarra(potencia);
}
void exibirAjuda() {
  Serial.println(F(""));
  Serial.println(F("  Comandos disponíveis:"));
  Serial.println(F("  ----------------------"));
  Serial.println(F("  0 a 100  → define a potência (%)"));
  Serial.println(F("  +        → aumenta 5%"));
  Serial.println(F("  -        → diminui 5%"));
  Serial.println(F("  on       → potência máxima (100%)"));
  Serial.println(F("  off      → desliga (0%)"));
  Serial.println(F("  status   → exibe situação atual"));
  Serial.println(F("  help     → exibe esta ajuda"));
  Serial.println(F(""));
}
void processarComando(String cmd) {
  if (cmd == "help" || cmd == "?") {
    exibirAjuda();

  } else if (cmd == "status") {
    exibirStatus();

  } else if (cmd == "off") {
    definirPotencia(0);

  } else if (cmd == "on") {
    definirPotencia(100);

  } else if (cmd == "+") {
    definirPotencia(potencia + 5);

  } else if (cmd == "-") {
    definirPotencia(potencia - 5);

  } else if (cmd.toInt() != 0 || cmd == "0") {
    int valor = cmd.toInt();
    definirPotencia(valor);

  } else {
    Serial.print(F("[ERRO] Comando desconhecido: \""));
    Serial.print(cmd);
    Serial.println(F("\" — Digite 'help' para ver os comandos."));
  }
}
void lerComandos() {
  if (Serial.available() > 0) {
    String entrada = Serial.readStringUntil('\n');
    entrada.trim();
    entrada.toLowerCase();

    if (entrada.length() > 0) {
      processarComando(entrada);
    }
  }
}
void initialize() {
  // Inicializa Serial
  Serial.begin(9600);
  
  // Configura pino PWM do motor
  pinMode(PWM_PIN, OUTPUT);
  analogWrite(PWM_PIN, 0);  // Começa desligado
  
  // Configura pino do encoder
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  
  // Ativa interrupção para encoder (INT0 = Pino 2)
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), contarPulso, RISING);
  
  // Marca tempo inicial
  tempo_inicio = millis();
  tempo_anterior = millis();
  
  // Mensagem de início
  Serial.println(F("========================================"));
  Serial.println(F("  MOTOR PWM + AQUISIÇÃO DE ENCODER"));
  Serial.println(F("  PWM: Pino 9 | Encoder: Pino 2"));
  Serial.println(F("  Baud: 9600"));
  Serial.println(F("========================================"));
  Serial.println(F("  Enviando: tempo_ms,rpm,omega_rad_s"));
  Serial.println(F("========================================"));
  exibirAjuda();
}

void motorLoop() {
  for (int passo = 0; passo <= 100; passo += 10) {
    definirPotencia(passo);
    unsigned long inicio_step = millis();
    while (millis() - inicio_step < TEMPO_POR_STEP_MS) {
      lerComandos();
      if (millis() - tempo_anterior >= intervalo_envio) {
        enviarDados();
      }

    }
  }

  for (int passo = 100; passo >= 0; passo -= 10) {
    definirPotencia(passo);
    unsigned long inicio_step = millis();
    while (millis() - inicio_step < TEMPO_POR_STEP_MS) {
      lerComandos();
      if (millis() - tempo_anterior >= intervalo_envio) {
        enviarDados();
      }

    }
  }
}