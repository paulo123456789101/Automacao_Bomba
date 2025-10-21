# 💧 Automação de Bomba — Controle de Nível com Sensores ZMPT101B

## 📘 Descrição do Projeto
Este projeto implementa um **sistema automatizado de controle de bomba** utilizando **sensores de tensão ZMPT101B** para detectar o nível da água em um poço ou reservatório.  
O sistema liga e desliga a bomba conforme os níveis dos sensores de **mínimo (Emin)** e **máximo (Emax)**, evitando operação a seco e protegendo o equipamento.

Inclui ainda:
- Modo automático acionado por botão.
- Detecção de falha de sensor (por exemplo, se o sensor inferior estiver defeituoso).
- Indicação visual por LEDs.
- Operação totalmente **não bloqueante** (sem `delay()`), utilizando **`millis()`**.

---

## ⚙️ Componentes Utilizados
| Componente | Função | Quantidade |
|-------------|--------|------------|
| Arduino UNO (ou compatível) | Microcontrolador principal | 1 |
| ZMPT101B | Sensor de tensão AC para leitura de eletrodos | 2 |
| LED verde | Indica bomba ligada | 1 |
| LED amarelo | Indica modo automático ativo | 1 |
| Botão (pushbutton) | Liga/desliga modo automático | 1 |
| Relé | Comando da bomba | 1 |
| Resistores diversos | Para LEDs e pull-downs | - |
| Fonte de alimentação 5V | Alimentação do circuito | 1 |

---

## 🧠 Mapeamento de Pinos
| Função | Pino Arduino |
|--------|---------------|
| Botão de modo | D7 |
| Saída da bomba (relé) | D8 |
| LED modo automático | D12 |
| LED bomba ligada | D11 |
| Sensor de nível mínimo (Emin) | A1 |
| Sensor de nível máximo (Emax) | A2 |

---

## 🧩 Algoritmo do Sistema

1. **Inicialização**
   - Configura os pinos de entrada e saída.
   - Exibe mensagem de inicialização via `Serial`.
   - Começa com a bomba desligada e modo automático desativado.

2. **Leitura dos Sensores**
   - Mede a tensão RMS de cada sensor (ZMPT101B) com 500 amostras.
   - Converte o valor lido em volts e aplica um fator de calibração (`calibrationFactor = 242.0`).

3. **Estados do Poço**
   O sistema trabalha com uma **máquina de estados**:
   - `POCO_VAZIO` → Nenhum sensor molhado.
   - `POCO_ENCHENDO` → Somente sensor inferior molhado.
   - `POCO_CHEIO` → Ambos os sensores molhados.
   - `POCO_DESCENDO` → Apenas o inferior molhado, o superior seco.
   - `SENSOR_FALHA` → O sensor máximo molhado e o mínimo seco (condição impossível).

4. **Lógica de Controle da Bomba**
   - Liga a bomba quando o poço está **cheio ou descendo**.
   - Desliga a bomba quando está **vazio, enchendo ou há falha**.
   - Se detectada falha no sensor inferior, a bomba é **bloqueada** e o sistema exibe:
     ```
     🚨 Falha detectada: Sensor inferior com defeito!
     ```

5. **Controle por Botão**
   - Um botão com **debounce por software** alterna entre:
     - Modo automático (LED modo aceso)
     - Modo desligado (bomba desarmada)

6. **Operação não bloqueante**
   - Todas as medições e atualizações usam `millis()` para temporização.
   - Isso permite rodar continuamente sem travar o microcontrolador.

---

## 📈 Fluxograma Simplificado
```
           ┌──────────────┐
           │  Iniciar     │
           └──────┬───────┘
                  │
           ┌──────▼───────┐
           │ Ler sensores │
           └──────┬───────┘
                  │
     ┌────────────┼────────────┐
     │            │            │
     ▼            ▼            ▼
Sensor falha?   Poço cheio?   Poço vazio?
     │              │             │
     ▼              ▼             ▼
Bloqueia bomba   Liga bomba    Desliga bomba
```

---

## 🧮 Calibração dos Sensores ZMPT101B
1. Meça com multímetro a tensão real.
2. Compare com a leitura exibida no `Serial`.
3. Ajuste o valor de:
   ```cpp
   float calibrationFactor = 242.0;
   ```
   até as leituras coincidirem.

---

## 📚 Referências
- **Datasheet ZMPT101B:** [https://components101.com/sensors/zmpt101b-voltage-sensor-module](https://components101.com/sensors/zmpt101b-voltage-sensor-module)
- **Documentação Arduino `millis()`:** [https://www.arduino.cc/reference/en/language/functions/time/millis/](https://www.arduino.cc/reference/en/language/functions/time/millis/)
- **Controle de estados finitos:**  
  F. Pontes, *Sistemas de Controle com Máquinas de Estado*, Ed. UFABC, 2020.  
- **Debounce digital em botões:**  
  J. Blum, *Exploring Arduino*, Cap. 5 — "Digital Inputs".

---

## 👨‍🔧 Autor
**Paulo Francisco Carvalho Araújo**  
Desenvolvedor e pesquisador em automação e IoT aplicada a sistemas de irrigação e controle de bombas com sensores inteligentes.

---

## 🏷️ Licença
Este projeto está sob a licença MIT — uso livre para fins educacionais e de pesquisa, com atribuição ao autor original.

---

## 🧰 Como usar
```bash
# Clonar o repositório
git clone https://github.com/paulo123456789101/Automacao_Bomba.git

# Abrir o arquivo principal
Sensor_ER_EI_EI_Atual.ino
```
