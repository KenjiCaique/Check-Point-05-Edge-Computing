# Vinheria Agnello - IoT (Checkpoint 05)

**FIAP - Engenharia de Software | Edge Computing & Computer Systems | Turma 1ESR**
Prof. Lucas Demetrius Augusto

## Integrantes do grupo

Caique Kenji Yafuco - 570368

## Descrição do projeto

A Vinheria Agnello precisa monitorar as condições de armazenamento da adega, pois
luminosidade, temperatura e umidade afetam a qualidade do vinho
(ideal: ~13 °C, umidade entre 60% e 80% e ambiente em penumbra).

Este checkpoint conecta um **ESP32** com sensores **DHT11** (temperatura e umidade) e
**LDR** (luminosidade) ao **servidor de IoT (FIWARE)** via **Wi-Fi + MQTT**.
Cada leitura é enviada em JSON com **timestamp** (NTP, UTC). O ESP32 também recebe
comandos (liga/desliga do LED) enviados pelo aplicativo **MyMQTT**.

```
[DHT11 + LDR] -> [ESP32] --Wi-Fi/MQTT--> [Mosquitto :1883] -> [IoT Agent MQTT] -> [Orion / STH-Comet]
                    ^                          |
                    +---- comandos (MyMQTT) ---+
```

## Componentes

- 1x ESP32 DevKit (ESP32-WROOM-32)
- 1x sensor DHT11
- 1x LDR (com divisor de tensão/resistor, já presente na placa de apoio)
- 1x LED (D2)
- Cabo USB e rede Wi-Fi 2.4 GHz

## Ligações

| Componente | Pino ESP32 |
|------------|-----------|
| DHT11 (DATA) | D4 |
| LDR (saída analógica) | D34 |
| LED | D2 |
| VCC | 3V3 |
| GND | GND |

(Conforme a montagem do slide 10 do enunciado.)

## Como executar

1. Instale a **Arduino IDE** e o suporte à placa ESP32
   (Gerenciador de Placas -> `esp32` by Espressif).
2. Instale as bibliotecas (Gerenciador de Bibliotecas):
   - `PubSubClient`
   - `DHT sensor library` (Adafruit) e `Adafruit Unified Sensor`
3. Abra `vinheria_agnello.ino` e edite o bloco **CONFIGURAÇÃO**:
   - `WIFI_SSID` / `WIFI_PASS`
   - `MQTT_HOST` / `MQTT_PORT` (IP e porta do broker)
   - `DEVICE_ID` e `API_KEY` (se diferentes do padrão do servidor)
4. Selecione a placa **ESP32 Dev Module** e a porta COM, e faça o upload.
5. Abra o Monitor Serial a **115200 baud** e confirme Wi-Fi, NTP e MQTT conectados.

## Tópicos MQTT

| Função | Tópico | Payload |
|--------|--------|---------|
| Publicar leituras | `/json/TEF/adega001/attrs` | `{"temperature":22.5,"humidity":65.0,"luminosity":30,"TimeInstant":"2026-10-22T14:00:00Z"}` |
| Receber comandos | `/TEF/adega001/cmd` | `adega001@on\|` ou `adega001@off\|` |
| Resposta do comando | `/TEF/adega001/cmdexe` | `adega001@on\|on` |

## Teste com o MyMQTT

1. No app **MyMQTT**, configure: *Broker* = IP do servidor, *Port* = 1883 (e usuário/senha, se houver).
2. **Leitura:** em *Subscribe*, inscreva-se em `/json/TEF/adega001/attrs` e veja as leituras a cada 5 s.
3. **Escrita:** em *Publish*, envie para `/TEF/adega001/cmd` o payload `adega001@on|` (LED acende) e `adega001@off|` (LED apaga).
4. Inscreva-se em `/TEF/adega001/cmdexe` para ver a confirmação do comando.

## Arquivos

- `vinheria_agnello.ino` - firmware do ESP32
- `README.md` - este documento

## Solução de problemas

- **Não conecta ao Wi-Fi:** use rede 2.4 GHz.
- **Falha no DHT11:** verifique o pino D4 e a alimentação em 3V3.
- **MQTT rc=-2 / -4:** confira IP, porta 1883 e se o servidor está acessível na rede.
- **Luminosidade invertida:** inverta o `map()` no código conforme o divisor de tensão da sua placa.
