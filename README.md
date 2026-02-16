# 🌱 Automated Indoor Plant Care & Climate Management

An IoT-based system that monitors indoor air quality and automates plant irrigation — built to explore how plants can improve air quality in office and home environments.

Inspired by projects like [SproutsIO](https://www.sproutsio.com/) and MIT Media Lab's [Personal Food Computer](https://www.media.mit.edu/projects/personal-food-computer/overview/), the goal is to develop a scalable prototype that measures indoor air quality parameters like formaldehyde and VOC gases, which are often prevalent in office spaces and can be harmful at chronic exposure.

## What It Does

The system connects up to **8 plants**, monitors their environment in real-time, and waters them automatically based on configurable schedules and sensor readings.

Each plant has its own subpage where the user can view its requirements, read a species description (via the [Encyclopedia of Life API](https://eol.org/docs/what-is-eol/data-services)), and trigger manual actions like irrigation.

Monitored parameters include soil moisture, light spectrum and intensity, water flow, and water reservoir levels. The current state of each plant is displayed on a physical dot matrix display attached to the prototype.

All irrigation events are documented, persisted, and displayed in the dashboard.

## Architecture

### Hardware

The prototype is built around an **ESP32** microcontroller with the following components:

| Component | Purpose |
|-----------|---------|
| AMS-Osram TSL2591 | Light spectrum and intensity |
| VL53L0X (ToF sensor) | Water reservoir level |
| CD74HC4067 (Multiplexer) | Scaling sensor inputs across 8 plants |
| INA219 | Current/power monitoring |
| MAX7219 Dot Displays | Per-plant status display |
| Solenoid valves | Distributing and securing irrigation flow |

Several irrigation methods, 3D-printed water distributors, and pump types were tested to find an economical use of resources, so the prototype can operate autonomously for up to two weeks.

### State Machine

System behavior is modeled as a **hierarchical state machine** with several sub-state machines that control individual sensor processes. The system operates through distinct states where tasks like connecting, sensor initialization, measurement, and data evaluation are processed sequentially. This architecture allows, for example, the coordinated control of solenoid valves to distribute and secure irrigations safely.

### Backend & Data Flow

Sensor data is collected periodically by the prototype and sent to **InfluxDB**. From there, it is aggregated and served through a **RESTful API** built with **Node.js** and **Express**, backed by **MongoDB** for persistent storage.

### Visualization

Data is visualized via multiple **Grafana dashboards** that aggregate readings per plant, plant group, and time range. The user can monitor live data, review historical trends, and inspect individual irrigation events.

## Tech Stack

| Layer | Technologies |
|-------|-------------|
| Firmware | C++, Arduino, ESP32 |
| Backend | Node.js, Express, REST API, MongoDB |
| Data | InfluxDB, Grafana |
| Hardware | ESP32, custom PCB, solenoid valves, various I²C sensors |

## Context

This project was developed as a **Bachelor's thesis** in Computer Science at the University of Applied Sciences Trier, Germany.

## System Architecture

![UML diagram of the system behaviour.](./images/system_uml.jpg)

## System-Dashboard

![Overview of the System sensor data](./images/system_sensors_1.png)

![Manual system control options](./images/system_controls.png)

## Plants-Dashboard

![User Interface for individual Plants connected to the system](./images/plants_controls.png)

![Plants Sensors Soil Moisture, Light Intensity](./images/plants_sensors_1.png)

![Plants Sensor Data aggregated](./images/plants_sensors_2.png)

## License

MIT License — see [LICENSE](LICENSE) for details.

---

*Built by [Julian Heller](https://github.com/julizs)*
