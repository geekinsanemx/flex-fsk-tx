# Binary Protocol Test Client

Cliente en C para probar el protocolo binario UART de FLEX-FSK-TX v2.5.1.

## Compilación

```bash
gcc -o flex-binary-client flex-binary-client.c -O2 -Wall
```

## Uso

### Sintaxis Básica

```bash
./flex-binary-client [OPTIONS] CAPCODE MESSAGE
```

### Opciones

| Opción | Descripción | Default |
|--------|-------------|---------|
| `-d DEVICE` | Puerto serial | `/dev/ttyUSB0` |
| `-b BAUD` | Baudrate | `115200` |
| `-f FREQ` | Frecuencia (ignorado, usa config del device) | - |
| `-p POWER` | Potencia (ignorado, usa config del device) | - |
| `-v` | Modo verbose | No |
| `-w` | Esperar por evento TX_DONE | No |
| `-h` | Mostrar ayuda | - |

**Nota:** Las opciones `-f` y `-p` se aceptan por compatibilidad con `flex-fsk-tx` pero son ignoradas. El dispositivo usa su configuración interna.

## Ejemplos

### 1. Envío Básico (Fire and Forget)

```bash
./flex-binary-client 1234567 "Hello World"
```

**Output:**
```
Connected to /dev/ttyUSB0 @ 115200 baud
Sent message (msg_id=0x0001, capcode=1234567)
ACK: Message accepted
Done.
```

### 2. Con Confirmación de Transmisión

```bash
./flex-binary-client -w -d /dev/ttyUSB0 37137 "Test message"
```

**Output:**
```
Connected to /dev/ttyUSB0 @ 115200 baud
Sent message (msg_id=0x0001, capcode=37137)
ACK: Message accepted
Waiting for TX completion...
EVENT: TX_START (msg_id=0x0001) - Transmitting...
EVENT: TX_DONE (msg_id=0x0001) - SUCCESS
```

### 3. Modo Verbose

```bash
./flex-binary-client -v -w 1234567 "Long message test"
```

**Output:**
```
Device: /dev/ttyUSB0
Baudrate: 115200
Capcode: 1234567
Message: Long message test
Length: 17 bytes

Connected to /dev/ttyUSB0 @ 115200 baud
Sending CMD_SEND_FLEX (msg_id=0x0001, seq=1, len=22)
Sent message (msg_id=0x0001, capcode=1234567)
ACK: Message accepted
Waiting for TX completion...
EVENT: TX_QUEUED (msg_id=0x0001, pos=1)
EVENT: TX_START (msg_id=0x0001) - Transmitting...
EVENT: TX_DONE (msg_id=0x0001) - SUCCESS
```

### 4. Mensaje Largo (como tu ejemplo)

```bash
./flex-binary-client -d /dev/ttyUSB0 -w 37137 "Greatness is achieved not by chance but by consistent effort, vision, and resilience; every challenge faced is an opportunity to grow stronger, every failure a lesson, and every step taken with courage a path toward lasting impact and meaningful"
```

**Output:**
```
Warning: Message truncated to 248 chars
Connected to /dev/ttyUSB0 @ 115200 baud
Sent message (msg_id=0x0001, capcode=37137)
ACK: Message accepted
Waiting for TX completion...
EVENT: TX_START (msg_id=0x0001) - Transmitting...
EVENT: TX_DONE (msg_id=0x0001) - SUCCESS
```

## Comparación con flex-fsk-tx (AT Mode)

### Comando AT Mode Original:
```bash
flex-fsk-tx -d /dev/ttyUSB0 -f 931.9375 -p 10 37137 "Message"
```

### Equivalente Binary Protocol:
```bash
./flex-binary-client -d /dev/ttyUSB0 -w 37137 "Message"
```

**Diferencias:**
- ❌ Binary protocol NO usa `-f` (frecuencia) ni `-p` (potencia)
- ✅ El dispositivo usa su configuración interna de frecuencia/potencia
- ✅ Binary protocol soporta eventos asíncronos (TX_START, TX_DONE)
- ✅ Binary protocol es non-blocking (puede enviar múltiples mensajes)
- ✅ Binary protocol usa msg_id para correlacionar requests/responses

## Protocolo Binario

### Flujo de Comunicación

```
Cliente                      ESP32 (Device)
  |                             |
  |--- CMD_SEND_FLEX ---------->|
  |<-- RSP_ACK -----------------|  (< 10ms)
  |<-- EVT_TX_QUEUED -----------|  (opcional)
  |                             |
  |                             | [Transmisión RF 2-5s]
  |                             |
  |<-- EVT_TX_START ------------|  (inicio TX)
  |<-- EVT_TX_DONE -------------|  (fin TX)
  |                             |
```

### Estructura de Paquete

```
[LEN:1][TYPE:1][OPCODE:1][FLAGS:1][SEQ:1][MSG_ID:2][PAYLOAD:N][CRC16:2]
```

- **LEN**: Longitud total del paquete
- **TYPE**: 0x01=CMD, 0x02=RSP, 0x03=EVT
- **OPCODE**: Comando/respuesta/evento específico
- **FLAGS**: 0x01=ACK_REQUIRED
- **SEQ**: Número de secuencia de transporte
- **MSG_ID**: ID de mensaje (big-endian) para correlación
- **PAYLOAD**: Datos específicos del comando
- **CRC16**: CRC-16-CCITT

### Framing COBS

Todos los paquetes usan COBS (Consistent Overhead Byte Stuffing):
- Elimina bytes 0x00 del stream
- Usa 0x00 como delimitador de frame
- Overhead: ~0.4%

## Códigos de Error

### Status Codes (RSP_ACK)

| Code | Nombre | Descripción |
|------|--------|-------------|
| 0x00 | STATUS_ACCEPTED | Mensaje aceptado y encolado |
| 0x01 | STATUS_REJECTED | Mensaje rechazado |
| 0x02 | STATUS_QUEUE_FULL | Queue lleno (max 10 mensajes) |
| 0x03 | STATUS_INVALID_PARAM | Parámetros inválidos |

### Result Codes (EVT_TX_DONE/FAILED)

| Code | Nombre | Descripción |
|------|--------|-------------|
| 0x00 | RESULT_SUCCESS | Transmisión exitosa |
| 0x01 | RESULT_RADIO_ERROR | Error del radio |
| 0x02 | RESULT_ENCODING_ERROR | Error de codificación FLEX |

## Troubleshooting

### Error: Timeout waiting for ACK

**Causa:** Device no responde
**Solución:**
1. Verificar que el firmware v2.5.1+ esté flashed
2. Verificar puerto serial correcto (`ls /dev/tty*`)
3. Verificar permisos (`sudo usermod -aG dialout $USER`)

### Error: Queue full

**Causa:** Más de 10 mensajes encolados
**Solución:** Esperar a que se transmitan mensajes anteriores (-w flag)

### Error: CRC mismatch

**Causa:** Corrupción de datos en serial
**Solución:**
1. Verificar cable USB
2. Reducir baudrate: `-b 115200`
3. Verificar interferencia electromagnética

## Límites

- **Max message length:** 248 caracteres
- **Queue size:** 10 mensajes
- **Transmission time:** 2-5 segundos por mensaje
- **Baudrate:** 115200 bps (default), hasta 921600 bps

## Notas Técnicas

### Non-Blocking Operation

El protocolo binario es completamente non-blocking:

```bash
# Enviar 3 mensajes sin esperar
./flex-binary-client 1111111 "Message 1" &
./flex-binary-client 2222222 "Message 2" &
./flex-binary-client 3333333 "Message 3" &

# Los 3 se encolarán y transmitirán secuencialmente
```

### Message ID Correlation

Cada mensaje tiene un `msg_id` único que permite correlacionar:
- CMD_SEND_FLEX (msg_id=0x0001)
- RSP_ACK (msg_id=0x0001)
- EVT_TX_QUEUED (msg_id=0x0001)
- EVT_TX_START (msg_id=0x0001)
- EVT_TX_DONE (msg_id=0x0001)

Esto permite operación concurrente sin race conditions.

## Ver También

- [BINARY_PROTOCOL_IMPLEMENTATION_SPEC.md](../BINARY_PROTOCOL_IMPLEMENTATION_SPEC.md) - Especificación completa del protocolo
- [PHASE1_COMPLETE.md](../PHASE1_COMPLETE.md) - Documentación Fase 1 (Core Protocol)
- [PHASE2_COMPLETE.md](../PHASE2_COMPLETE.md) - Documentación Fase 2 (Integration)
