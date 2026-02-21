#include "commands.h"

// Define the static commandConfig array
const String Commands::commandConfig[6][8] = {
    {"RPM", "rpm", "010C", "0", "0", "7000", "2", "0"},
    {"Engine Load", "%", "0104", "1", "0", "100", "1", "0"},
    {"Barometric Pressure", "psi", "0133", "2", "0", "37", "1", "14.7"},
    {"Reference Torque", "lbft", "0163", "3", "0", "500", "2", "445"},
    {"Actual Torque", "%", "0162", "4", "-125", "125", "1", "0"},
    {"Speed", "mph", "010D", "5", "0", "158", "1", "0"}
};

// Constructor initializes A and B
Commands::Commands() : A(0), B(0) {}

// Send a command to the ELM327 and return the response
String Commands::sendCommand(String pid) {
    if (!connected || pWriteChar == nullptr) {
        Serial.println("Cannot send command: No OBD connection");
        return "";
    }
    responseBuffer = "";
    String fullCmd = pid + "\r";
    unsigned long start = millis();
    Serial.println("Sending command: " + pid);
    pWriteChar->writeValue(fullCmd);
    for (int i = 0; i < 1000; i++) {
        delay(1);
        if (responseBuffer.indexOf(">") != -1) {
            unsigned long end = millis();
            unsigned long duration = end - start;
            Serial.println("Response time: " + String(duration) + " | ResponseBuffer: " + responseBuffer);
            String fullResponse = responseBuffer;
            responseBuffer = "";
            return fullResponse;
        }
    }
    Serial.println("Command " + pid + " timed out. Response buffer: " + responseBuffer);
    return "";
}

// Parse the response and set A and B
void Commands::parsePIDResponse(String response, String pid, int numBytes) {
    String compact = "";
    for (int i = 0; i < response.length(); i++) {
        char c = toupper(response.charAt(i));
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
            compact += c;
        }
    }

    String searchStr = "41" + pid.substring(2);
    int idx = compact.indexOf(searchStr);
    A = -1;
    B = -1;

    if (idx == -1) {
        Serial.println("PID not found in response: " + compact);
        return;
    }

    int start = idx + searchStr.length();
    int neededChars = numBytes * 2;
    if (start + neededChars > compact.length()) {
        Serial.println("Incomplete PID response: " + compact);
        return;
    }

    String payload = compact.substring(start, start + neededChars);

    if (numBytes >= 1) {
        A = strtol(payload.substring(0, 2).c_str(), NULL, 16);
    }
    if (numBytes >= 2) {
        B = strtol(payload.substring(2, 4).c_str(), NULL, 16);
    }

    Serial.printf("Parsed PID %s => A: %d, B: %d from %s\n", pid.c_str(), A, B, compact.c_str());
}

// Query a command and return the calculated value
double Commands::query(int commandIndex) {
    if (!connected || pWriteChar == nullptr) {
        return 0.0; // Return default value if not connected
    }
    String command = commandConfig[commandIndex][2];
    int numBytes = commandConfig[commandIndex][6].toInt();
    int formula = commandConfig[commandIndex][3].toInt();
    String response = sendCommand(command);

    if (response == "") {
        return 0.0;
    }

    parsePIDResponse(response, command, numBytes);
    if (A < 0 || (numBytes > 1 && B < 0)) {
        return 0.0;
    }

    double val = 0.0;
    switch (formula) {
        case 0:
            val = ((((double)A * 256.00) + (double)B) / 4.0);
            return val;
        case 1:
            val = ((double)A * 100.00) / 255.00;
            return val;
        case 2:
            val = ((double)A * 0.145);
            return val;
        case 3:
            val = (((double)A * 256.00) + (double)B) * 0.7376;
            return val;
        case 4:
            val = (double)A - 125.00;
            return val;
        case 5:
            val = A * 0.621371;
            return val;
        default:
            return -1.0;
    }
}

double Commands::getReading(int selectedReading) {
    if (!connected || pWriteChar == nullptr) {
        return 0.0; // Return default value if not connected
    }
    switch (selectedReading) {
        case 0:
            // RPM
            return query(0);
        case 1: {
            // Boost
            double rpm = query(0);
            double load = query(1);
            double baro = query(2);
            double fRpm = std::min(1.0, (rpm - 1500) / (4000 - 1500));
            fRpm = std::max(0.0, fRpm);

            double boost = 22.0 * (load / 100.0) * fRpm;
            return boost;
        }
        case 2: {
            // Torque
            double ref = query(3);
            double actual = query(4);
            return (ref * (actual / 100.0));
        }
        case 3: {
            // Horsepower
            double rpm = query(0);
            double refTorque = query(3);
            double actualTorque = query(4);
            return ((refTorque * (actualTorque / 100.0)) * rpm) / 5252.0;
        }
        case 5: {
            // Speed
            return query(5);
        }
        default:
            return 0.0;
    }
}

struct dualGaugeReading Commands::getDualReading() {
    struct dualGaugeReading x;
    x.readings[0] = 0.0;
    x.readings[1] = 0.0;

    if (!connected || pWriteChar == nullptr) {
        return x;
    }

    // Get RPM, Reference Torque, and Torque
    double rpm = query(0);
    double refTorque = query(3);
    double actTorque = query(4);
    double torque = (refTorque * (actTorque / 100.0));
    double hp = ((refTorque * (actTorque / 100.0)) * rpm) / 5252.0;
    x.readings[0] = torque;
    x.readings[1] = hp;
    return x;
}