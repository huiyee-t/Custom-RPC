# Custom-RPC

Custom-RPC is a custom-built Remote Procedure Call (RPC) framework in C that allows computations to be distributed and executed across multiple computers using a client-server architecture.

This system enables clients to remotely invoke registered procedures on a server, supporting dynamic communication, protocol-defined message handling, and memory-safe interactions over TCP.

---

## 🚀 Features

- Custom binary protocol for RPC communication
- Big-endian compatibility for cross-platform execution
- Reliable transport using TCP sockets
- Support for dynamic client-server connections
- Clean separation between RPC framework and user-defined functions
- Robust error checking and input validation

---

## 📦 Protocol Overview

### Message Flow

1. **FIND request**: Client checks if a procedure is registered on the server.
2. **RESPONSE (FIND)**: Server replies with procedure ID or -1.
3. **CALL request**: Client sends data to invoke the procedure.
4. **RESPONSE (CALL)**: Server returns result or -1 for null.

### Packet Format

All messages follow this format (with all numbers in big-endian):

```
Packet Format:
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| |
| Number of bytes of message type |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| |
| The type of message |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| |
| Number of bytes of Field 1 |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| |
| Field 1 |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| |
| Number of bytes of Field 2 (Optional) |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| |
| Field 2 (Optional) |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| ... |
| ... |
```

- `Type` is a string: `"find"`, `"call"`, or `"resp"`
- Each field is preceded by its byte length
- `data1` (in `rpc_data`) is 64-bit; all other integers are 32-bit

---

## 📚 Message Types

| Type   | Purpose                 | Key Fields                      |
| ------ | ----------------------- | ------------------------------- |
| `find` | Search for procedure    | Procedure name (string)         |
| `call` | Invoke remote function  | Procedure ID, data1, data2      |
| `resp` | Return result from call | data1 and data2, or -1 for null |

---

## 🧠 Design Highlights

- **Authentication** is handled by individual functions, offering flexibility.
- **Sockets** are initialized in `rpc_init_client()` and `rpc_init_server()` for modular control.
- **Dynamic memory** usage ensures scalability; remember to free memory after use.
- **Error handling** detects invalid arguments, oversized payloads, and protocol mismatches.
- **Transport layer** uses TCP for reliability, avoiding complications of QUIC and unreliability of UDP.

---

## ⚠️ Error Handling Examples

- Null arguments in `rpc_find()` or `rpc_call()` are rejected.
- Data size inconsistencies (e.g., `data2_len` mismatched with `data2`) trigger client/server-side warnings.
- Handle IDs outside valid range are ignored and logged.
- Payloads over 100,000 bytes are blocked for safety.

---
