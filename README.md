<!-- LANGUAGE SWITCH -->
<p align="center">
  <a href="#english-version">
    <img src="https://img.shields.io/badge/Language-English-blue?style=for-the-badge" />
  </a>
  <a href="#türkçe-versiyon">
    <img src="https://img.shields.io/badge/Dil-Türkçe-red?style=for-the-badge" />
  </a>
</p>

[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
<a href="https://www.linkedin.com/in/hilmi-can-%C3%BCrl%C3%BC-307ba630a/" target="_blank">
  <img src="https://img.shields.io/badge/LinkedIn-0077B5?style=for-the-badge&logo=linkedin&logoColor=white" />
</a>
<img align="right" src="https://visitor-badge.laobi.icu/badge?page_id=HilmiCanUrlu.Real-time-IoT-Monitoring-with-Telegram-Alerts" />

---

<a id="english-version"></a>

## English Version

<h3 align="center">Real-time IoT Monitoring with Telegram Alerts</h3>

### 📌 Overview

This project presents a **real-time IoT monitoring system** based on the **ESP32 microcontroller**, designed to collect, transmit, and visualize environmental sensor data using the **MQTT protocol**.

Environmental parameters such as **temperature, humidity, and light intensity** are monitored continuously and displayed via a **web-based dashboard**.  
When predefined threshold values are exceeded, the system sends **instant alerts via a Telegram bot**, including location information.

The project was developed as an **individual academic project** at  
**Mehmet Akif Ersoy University – Department of Information Systems and Technologies**.

---

### 📊 System Architecture

- ESP32 reads sensor data in real time  
- Data is published to a **cloud-based MQTT broker (HiveMQ)**  
- Web dashboard subscribes to MQTT topics  
- Telegram Bot API sends alert notifications  
- Web interface enables remote hardware control  

---

### ⚙️ Hardware Components

- ESP32 Microcontroller  
- SHT31 Temperature & Humidity Sensor  
- NTC Thermistor  
- LDR (Light Dependent Resistor)  

---

### 🌐 Communication Technologies

- MQTT (HiveMQ Cloud)  
- HTTP Web Server  
- Telegram Bot API  

---

### 🧠 Software Technologies

- C++ (Arduino Core for ESP32)  
- HTML5  
- CSS3  
- JavaScript  

---

### 🤖 Core Features

- Real-time sensor monitoring  
- Web-based dashboard visualization  
- Telegram alert notifications  
- Remote hardware control  

---

### ⚠️ Limitations

- Network dependency for MQTT communication  
- Internet connection required  
- Advanced security mechanisms not fully implemented  

---

### 👤 Project Author

- **Hilmi Can Ürlü**

---

### 📧 Contact

Hilmi Can Ürlü  
Email:  
Website:  

---

<hr />

<a id="türkçe-versiyon"></a>

## Türkçe Versiyon

<h3 align="center">Telegram Uyarılı Gerçek Zamanlı IoT İzleme Sistemi</h3>

### 📌 Genel Bakış

Bu proje, **ESP32 mikrodenetleyicisi** tabanlı, **MQTT protokolü** kullanarak çevresel verileri toplayan, ileten ve görselleştiren **gerçek zamanlı bir IoT izleme sistemi** sunmaktadır.

**Sıcaklık, nem ve ışık şiddeti** gibi çevresel parametreler sürekli olarak izlenmekte ve **web tabanlı bir kontrol paneli** üzerinden görüntülenmektedir.  
Tanımlanan eşik değerlerin aşılması durumunda, sistem **Telegram botu** aracılığıyla kullanıcıya **anlık uyarılar** göndermektedir.

Bu proje,  
**Mehmet Akif Ersoy Üniversitesi – Bilişim Sistemleri ve Teknolojileri Bölümü**  
kapsamında **bireysel akademik proje** olarak geliştirilmiştir.

---

### 📊 Sistem Mimarisi

- ESP32 sensör verilerini gerçek zamanlı olarak okur  
- Veriler HiveMQ üzerinden MQTT ile yayınlanır  
- Web arayüzü MQTT konularına abone olur  
- Telegram Bot API ile alarm bildirimleri gönderilir  
- Web arayüzü üzerinden donanım kontrolü sağlanır  

---

### ⚙️ Donanım Bileşenleri

- ESP32 Mikrodenetleyici  
- SHT31 Sıcaklık / Nem Sensörü  
- NTC Termistör  
- LDR (Işık Sensörü)  

---

### 🌐 Haberleşme Teknolojileri

- MQTT (HiveMQ Cloud)  
- HTTP Web Sunucusu  
- Telegram Bot API  

---

### 🧠 Yazılım Teknolojileri

- C++ (Arduino Core – ESP32)  
- HTML5  
- CSS3  
- JavaScript  

---

### 🤖 Temel Özellikler

- Gerçek zamanlı veri izleme  
- Web dashboard üzerinden görselleştirme  
- Telegram uyarı sistemi  
- Uzaktan donanım kontrolü  

---

### ⚠️ Sınırlılıklar

- MQTT için internet bağlantısı gereklidir  
- Ağ kararlılığına bağımlıdır  
- Gelişmiş güvenlik önlemleri eklenmemiştir  

---

### 👤 Proje Sahibi

- **Hilmi Can Ürlü**

---

### 📧 İletişim

Hilmi Can Ürlü  
E-posta:  
Web Sitesi:  

---

<!-- MARKDOWN LINKS & BADGES -->
[forks-shield]: https://img.shields.io/github/forks/HilmiCanUrlu/Real-time-IoT-Monitoring-with-Telegram-Alerts?style=for-the-badge
[forks-url]: https://github.com/HilmiCanUrlu/Real-time-IoT-Monitoring-with-Telegram-Alerts/network/members

[stars-shield]: https://img.shields.io/github/stars/HilmiCanUrlu/Real-time-IoT-Monitoring-with-Telegram-Alerts?style=for-the-badge
[stars-url]: https://github.com/HilmiCanUrlu/Real-time-IoT-Monitoring-with-Telegram-Alerts/stargazers

[issues-shield]: https://img.shields.io/github/issues/HilmiCanUrlu/Real-time-IoT-Monitoring-with-Telegram-Alerts?style=for-the-badge
[issues-url]: https://github.com/HilmiCanUrlu/Real-time-IoT-Monitoring-with-Telegram-Alerts/issues
