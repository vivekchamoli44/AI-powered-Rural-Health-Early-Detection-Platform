# AI-Powered Rural Health Early Detection Platform

**Team ID:** DSCPP-III-2026-T009 | **Academic Session:** 2026–27  
**Department:** Computer Science & Engineering, Graphic Era (Deemed to be University), Dehradun

## Table of Contents
- [About the Project](#about-the-project)
- [Built With](#built-with)
- [Core Features](#core-features)
- [Setup and Running](#setup-and-running)
    - [Prerequisites](#prerequisites)
    - [Configure](#configure)
    - [Build & Run](#build--run)
- [Team Contribution](#team-contribution)
- [Disclaimer](#disclaimer)
  
## About the Project
In rural areas, many people lack access to timely healthcare services due to multiple factors. As a result, common chronic diseases (like Diabetes and Hypertension) and respiratory disorders (like Asthma) remain undetected until they become serious. This project is a **C++ and AI-driven early risk detection system** designed for rural communities. It collects basic patient details, symptoms, and vital data, then processes them to calculate a real-time health risk score. A **triage engine** prioritizes and classifies patients into High, Medium, and Low risk levels so that local ASHA workers can take immediate action for critical cases.

## Built With
The platform is developed using the following core technologies:
* **Programming Languages:** C / C++
* **Backend Web Framework:** Crow Framework (C++ HTTP Microframework)
* **Database:** MySQL (For secure profiles and history logs)
* **AI Integration:** Google Gemini AI API (For health assessment analysis)
* **Development Tools:** MinGW Compiler Suite & Git/GitHub

## Core Features
* **Role-Based Authentication:** Secure Login/Signup module separating Patient and ASHA worker profiles.
* **Area-Restricted Dashboard:** ASHA workers only see data and updates for patients within their specific geographic sector.
* **Intelligent Triage:** Automates structural prioritization so the most critically ill patients rise to the top of the queue instantly.
* **AI Assessment:** Processes patient-submitted symptoms to predict potential critical disorders (e.g., Diabetes, Asthma, Hypertension).
* **Real-time Alerts:** A dedicated sidebar layout displaying emergency alerts and next-step diagnostic suggestions.

## Setup and Running

### Prerequisites
* Windows with a MinGW-w64 g++ toolchain (the project was built against WinLibs POSIX/UCRT g++)
* MySQL 8.0 (optional — the server falls back to in-memory storage if it can't connect)
* A Google Gemini API key (ai.google.dev)

### Configure
Copy the example env file and fill in your own values — `backend/.env` holds real secrets, so it's git-ignored and must never be committed:
cd backend
cp .env.example .env
# then edit .env and set your GEMINI_API_KEY, MySQL creds, etc.

GEMINI_API_KEY=your_key_here
SERVER_PORT=8080
MYSQL_HOST=127.0.0.1
MYSQL_PORT=3306
MYSQL_USER=root
MYSQL_PASSWORD=
MYSQL_DATABASE=ai_health_center

### Build & Run
cd backend
build.bat   # compiles src/main.cpp -> server.exe
run.bat     # builds (if needed) and starts the server

Then open **http://localhost:8080** in a browser.

### Team Contribution

- YASHWARDHAN ROHILLA (LEADER): Login and Signup and ASHA worker module - Send notification feature
- VIVEK CHAMOLI (DEVELOPER): AI integration with structured response and Patient module - Start new assessment feature
- ANJALI RAI (DATABASE AND TESTING): Database and testing and Patient module - Show previous assessment feature
- NANDINI YADAV: Patient and ASHA worker dashboard and ASHA worker module

### Disclaimer
This is a triage and early-warning support tool, not a diagnostic device. AI-generated risk assessments are meant to help ASHA workers prioritize outreach and should always be followed up with a qualified clinician.
