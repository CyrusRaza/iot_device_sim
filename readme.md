# IoT Device Simulator (C) — Pluggable Transport (MQTT first)

A small **IoT “device” simulator** written in **C**. It runs on your machine (no hardware required), publishes telemetry periodically, and is structured so that **different communication protocols can be selected** (MQTT first, HTTP/WebSocket planned).

This repo is intentionally “embedded-style”:
- clear separation between **device logic** and **transport**
- small, testable components
- predictable behavior and reproducible local setup

---

## What it does (current)
- Builds a `device_sim` executable with **CMake**
- Starts a local MQTT broker with **Docker Compose**
- Publishes telemetry periodically using **MQTT** (via libmosquitto)
- Supports selecting the transport backend at runtime (function-table dispatch)

---

## Roadmap (next)
- Subscribe to commands (`device/<id>/cmd`) and adjust runtime config (e.g. publish interval)
- Reconnect/backoff + “online/offline” (LWT)
- Offline queue + replay on reconnect (bounded ring buffer)
- Unit tests + CI (GitHub Actions)
- Add additional transports (HTTP and/or WebSocket)

---

## Requirements
### In WSL / Linux
- `gcc`, `cmake`, `pkg-config`
- `libmosquitto-dev` (MQTT client library)
- Docker + Docker Compose
- `mosquitto-clients` (for manual pub/sub testing)

Install typical deps on Ubuntu:
```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libmosquitto-dev mosquitto-clients
