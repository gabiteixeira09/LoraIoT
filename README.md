# LoraIoT

IoT communication project using LoRa technology for remote data monitoring. This project includes two main codes: **Sender** (transmitter) and **Receiver**, with specific functionalities for data collection and transmission.

---

## Project Features

### **Sender Code**
The transmitter code is responsible for:
- Collecting location data (GPS).
- Measuring the amount of feed available in the reservoir using an ultrasonic sensor.
- Activating a servo motor to feed animals at scheduled times.
- Sending collected data (location, feeding status, feed volume) via LoRa to the receiver.

#### **Main Features**
- **Libraries Used**:
  - `RTClib`: Real-time clock (RTC) control.
  - `ESP32Servo`: Servo motor control.
  - `TinyGPSPlus`: GPS data reading.
  - `LoRa`: Wireless LoRa communication.
- **Hardware Configurations**:
  - Servo motor connected to an ESP32Lora pin.
  - Ultrasonic sensor to measure feed.
  - RTC with I2C for timekeeping.
  - LoRa module configured at 915 MHz.
- **Key Functionalities**:
  - Sending location data (latitude and longitude).
  - Feeding control with predefined schedules (two automatic feedings daily).
  - Calculating the volume of feed available in the reservoir.

---

### **Receiver Code**
The receiver code is responsible for:
- Receiving data transmitted by the LoRa module.
- Interpreting and classifying data packets (feeding, GPS, volume).
- Sending information to a backend server via HTTP requests.

#### **Main Features**
- **Libraries Used**:
  - `WiFi.h`: Wi-Fi network connection.
  - `HTTPClient.h`: HTTP requests for sending data to the server.
  - `LoRa`: Wireless LoRa communication.
- **Hardware Configurations**:
  - Wi-Fi connection (SSID and password defined in the code).
  - LoRa module configured at 915 MHz.
- **Key Functionalities**:
  - Receiving and interpreting LoRa packets (feeding, GPS, volume).
  - Sending data to backend endpoints as JSON.
  - Monitoring signal quality (RSSI).

---

## How to Use

### **Prerequisites**
1. **Required Hardware**:
   - 2 ESP32Lora (one for Sender and another for Receiver), with LoRa modules (compatible with 915 MHz).
   - Ultrasonic sensor.
   - RTC with I2C interface.
   - Servo motor.
   - GPS module.
2. **Required Software**:
   - Arduino IDE configured for ESP32.
   - Mentioned libraries (available in the Arduino IDE Library Manager).

### **Implementation Steps**
1. Configure the **Sender**:
   - Upload the `LoraComedouroSender.ino` code to the ESP32Lora transmitter.
   - Connect the sensors and modules according to the pins defined in the code.
2. Configure the **Receiver**:
   - Upload the `LoraComedouroReceiver.ino` code to the ESP32Lora receiver.
   - Set up the Wi-Fi network and endpoints in the code.
3. Test the system:
   - Ensure the LoRa modules communicate correctly.
   - Verify the data sent to the backend.

---

## Technical Details

### **Main Functions (Sender)**
- `medirDistancia()`: Measures the distance in the reservoir using the ultrasonic sensor.
- `calcularVolume(float distancia)`: Converts the distance into the available feed volume.
- Sends data via LoRa with GPS information, volume, and feeding status.

### **Main Functions (Receiver)**
- `parseDataHora()`: Extracts date and time from LoRa packets.
- `parseAlimentacao()`: Interprets received feeding data.
- `parseVolume()`: Converts volume data into numbers for backend submission.

---
