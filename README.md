# IoT-Based Autonomous Smart Home Monitoring System

## 📖 Project Description
This project is an IoT-based smart home system designed for **autonomous automation and real-time monitoring** using multiple sensors.  
The system operates **without remote mobile control**, relying entirely on sensor-based decision-making while displaying live data on a web dashboard.

The ESP32 acts as a **local WiFi access point**, hosting a web dashboard that shows real-time sensor values.

---

## 🚀 Features
- PIR sensor-based automatic light control
- Clap (sound) sensor-based LED/relay toggling
- Gas sensor monitoring with buzzer alert
- Ultrasonic sensor for water level detection
- Real-time web dashboard using ESP32 WebServer
- No remote/manual control (autonomous operation)

---

## 🧰 Hardware Components
- ESP32 Development Board  
- PIR Sensor  
- Gas Sensor (MQ series)  
- Sound (Clap) Sensor  
- Ultrasonic Sensor (HC-SR04)  
- Relay Module  
- Buzzer  
- LEDs  
- Jumper Wires  

---

## 💻 Software & Technologies Used
- Arduino IDE  
- ESP32 WiFi Library  
- ESP32 WebServer  
- HTML, CSS, JavaScript  
- Chart.js (for real-time graphs)

---

## 🌐 Working Principle
- Sensors collect environmental data continuously.
- ESP32 processes sensor values and performs automatic actions.
- ESP32 hosts a web dashboard via WiFi Access Point.
- Users can view real-time sensor data but **cannot control devices remotely**.

---

## 📡 WiFi Access Details
- **SSID:** ESP32-Dashboard  
- **Password:** 12345678  
- **Dashboard URL:** http://192.168.4.1

---

## 📂 Project Structure
