# SlimMQ

**SlimMQ** is a lightweight, UDP-based messaging middleware designed for high-performance publish/subscribe communication in resource-constrained environments.

Inspired by MQTT and Solace, SlimMQ provides topic-based routing and QoS guarantees, while eliminating the overhead of TCP and runtime dependencies.

---

## 🔧 Features

- **UDP-based topic Pub/Sub messaging**
- **Lightweight 14-byte fixed header format**
- **QoS 0 / 1 / 2** supported  
  - At most once (fire-and-forget)  
  - At least once (with ACK)  
  - Exactly once (4-stage handshake with state tracking)
- **Globbing-style topic filters** (`/sensor/#`, `+/temp`)
- **Internal event queue** with threaded message listener
- **Transparent client API**: no need to manage sockets or threads manually

---

## 🔬 Performance

- **Broker executable size**: 27KB  
- **Runtime memory footprint**: ~2.8MB  
- **Delivers 1000+ messages/sec** in low-resource environments  
- **QoS 1 tested to recover all messages with 30% artificial packet loss**

---

## 🛠️ Planned Enhancements

- Zero-copy payload transmission  
- Pool allocator for lock-free memory reuse  
- Message fragmentation & reassembly for large payloads  
- Batch transmission support for efficient I/O

---

## 📜 License

MIT License

---

## ✍️ Author

A distributed systems course project from Ajou University, South Korea.  
Designed and developed by **Kim Jihwan**.
