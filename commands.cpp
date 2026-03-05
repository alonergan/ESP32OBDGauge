#include "commands.h"
#include <algorithm>

enum FormulaType {
    FORMULA_ENGINE_LOAD = 0,
    FORMULA_TEMP_F,
    FORMULA_FUEL_TRIM,
    FORMULA_FUEL_PRESSURE,
    FORMULA_PRESSURE_PSI,
    FORMULA_RPM,
    FORMULA_SPEED_MPH,
    FORMULA_TIMING_ADVANCE,
    FORMULA_MAF,
    FORMULA_RUNTIME_SECONDS,
    FORMULA_FUEL_PRESSURE_REL_VAC,
    FORMULA_FUEL_PRESSURE_DIRECT,
    FORMULA_ABSOLUTE_LOAD,
    FORMULA_EQUIV_RATIO,
    FORMULA_MAX_MAF,
    FORMULA_EVAP_ABS_PRESSURE,
    FORMULA_EVAP_VAPOR_PA,
    FORMULA_FUEL_RATE_GPH,
    FORMULA_TORQUE_PERCENT,
    FORMULA_REFERENCE_TORQUE_LBFT,
    FORMULA_FUEL_INJECTION_TIMING,
    FORMULA_INSTANT_MPG,
    FORMULA_AVERAGE_MPG
};

const pidCommandDefinition Commands::commandConfig[] = {
    {"Calculated engine load", "%", "0104", 0.0, 100.0, 1, FORMULA_ENGINE_LOAD, "0"},
    {"Engine coolant temp", "F", "0105", -40.0, 419.0, 1, FORMULA_TEMP_F, "0"},
    {"Short fuel trim B1", "%", "0106", -100.0, 99.22, 1, FORMULA_FUEL_TRIM, "0"},
    {"Long fuel trim B1", "%", "0107", -100.0, 99.22, 1, FORMULA_FUEL_TRIM, "0"},
    {"Short fuel trim B2", "%", "0108", -100.0, 99.22, 1, FORMULA_FUEL_TRIM, "0"},
    {"Long fuel trim B2", "%", "0109", -100.0, 99.22, 1, FORMULA_FUEL_TRIM, "0"},
    {"Fuel pressure", "psi", "010A", 0.0, 111.0, 1, FORMULA_FUEL_PRESSURE, "0"},
    {"Intake MAP", "psi", "010B", 0.0, 37.0, 1, FORMULA_PRESSURE_PSI, "0"},
    {"Engine RPM", "rpm", "010C", 0.0, 16383.75, 2, FORMULA_RPM, "0"},
    {"Vehicle speed", "mph", "010D", 0.0, 158.0, 1, FORMULA_SPEED_MPH, "0"},
    {"Timing advance", "deg", "010E", -64.0, 63.5, 1, FORMULA_TIMING_ADVANCE, "0"},
    {"Intake air temp", "F", "010F", -40.0, 419.0, 1, FORMULA_TEMP_F, "0"},
    {"MAF air flow", "g/s", "0110", 0.0, 655.35, 2, FORMULA_MAF, "0"},
    {"Throttle position", "%", "0111", 0.0, 100.0, 1, FORMULA_ENGINE_LOAD, "0"},
    {"Run time", "s", "011F", 0.0, 65535.0, 2, FORMULA_RUNTIME_SECONDS, "0"},
    {"Fuel pressure rel vac", "psi", "0122", 0.0, 750.0, 2, FORMULA_FUEL_PRESSURE_REL_VAC, "0"},
    {"Fuel pressure direct", "psi", "0123", 0.0, 95025.75, 2, FORMULA_FUEL_PRESSURE_DIRECT, "0"},
    {"Barometric pressure", "psi", "0133", 0.0, 37.0, 1, FORMULA_PRESSURE_PSI, "0"},
    {"Absolute load", "%", "0143", 0.0, 25700.0, 2, FORMULA_ABSOLUTE_LOAD, "0"},
    {"Command equiv ratio", "", "0144", 0.0, 2.0, 2, FORMULA_EQUIV_RATIO, "0"},
    {"Relative throttle", "%", "0145", 0.0, 100.0, 1, FORMULA_ENGINE_LOAD, "0"},
    {"Ambient air temp", "F", "0146", -40.0, 419.0, 1, FORMULA_TEMP_F, "0"},
    {"Max air flow", "g/s", "0150", 0.0, 2550.0, 1, FORMULA_MAX_MAF, "0"},
    {"Ethanol fuel", "%", "0152", 0.0, 100.0, 1, FORMULA_ENGINE_LOAD, "0"},
    {"Abs evap vapor", "psi", "0153", 0.0, 47.5, 2, FORMULA_EVAP_ABS_PRESSURE, "0"},
    {"Evap vapor pressure", "Pa", "0154", -32767.0, 32768.0, 2, FORMULA_EVAP_VAPOR_PA, "0"},
    {"Fuel rail pressure", "psi", "0159", 0.0, 95025.75, 2, FORMULA_FUEL_PRESSURE_DIRECT, "0"},
    {"Relative accel pedal", "%", "015A", 0.0, 100.0, 1, FORMULA_ENGINE_LOAD, "0"},
    {"Engine oil temp", "F", "015C", -40.0, 419.0, 1, FORMULA_TEMP_F, "0"},
    {"Fuel injection timing", "deg", "015D", -210.0, 301.992, 2, FORMULA_FUEL_INJECTION_TIMING, "0"},
    {"Engine fuel rate", "Gal/h", "015E", 0.0, 848.7, 2, FORMULA_FUEL_RATE_GPH, "0"},
    {"Driver demand torque", "%", "0161", -125.0, 125.0, 1, FORMULA_TORQUE_PERCENT, "0"},
    {"Actual engine torque", "%", "0162", -125.0, 125.0, 1, FORMULA_TORQUE_PERCENT, "0"},
    {"Engine ref torque", "lb-ft", "0163", 0.0, 48338.0, 2, FORMULA_REFERENCE_TORQUE_LBFT, "0"},
    {"Instant MPG", "mpg", "0110", 0.0, 100.0, 2, FORMULA_INSTANT_MPG, "0"},
    {"Average MPG", "mpg", "0110", 0.0, 100.0, 2, FORMULA_AVERAGE_MPG, "0"},
    {"Trans temperature", "F", "0105", -40.0, 500.0, 1, FORMULA_TEMP_F, "TCM"},
    {"Trans temperature (1)", "F", "01B4", -40.0, 500.0, 1, FORMULA_TEMP_F, "0"}
};

const int Commands::commandCount = sizeof(Commands::commandConfig) / sizeof(Commands::commandConfig[0]);

Commands::Commands() :
    A(0),
    B(0),
    lastQueryError(QUERY_OK),
    lastQueryCommandIndex(-1),
    lastQueryPid(""),
    lastQueryHeader(""),
    lastQueryResponse("") {}

String Commands::normalizeHeader(const char* header) const {
    String rawHeader = String(header);
    rawHeader.trim();
    rawHeader.toUpperCase();

    if (rawHeader == "" || rawHeader == "0") {
        return "7E0";
    }

    if (rawHeader == "TCM") {
        return "7E1";
    }

    return rawHeader;
}

String Commands::sendCommand(String pid, String header) {
    if (!connected || pWriteChar == nullptr) {
        Serial.println("Cannot send command: No OBD connection");
        lastQueryError = QUERY_NOT_CONNECTED;
        return "";
    }

    String selectedHeader = normalizeHeader(header.c_str());
    bool isATCommand = pid.startsWith("AT");

    if (!isATCommand) {
        responseBuffer = "";
        pWriteChar->writeValue("AT SH " + selectedHeader + "\r");

        bool headerReady = false;
        for (int i = 0; i < 250; i++) {
            delay(1);
            if (responseBuffer.indexOf(">") != -1) {
                headerReady = true;
                break;
            }
        }

        if (!headerReady) {
            Serial.println("Header set timed out for " + selectedHeader + ". Response buffer: " + responseBuffer);
            lastQueryError = QUERY_TIMEOUT;
            return "";
        }
    }

    responseBuffer = "";
    String fullCmd = pid + "\r";
    unsigned long start = millis();
    Serial.println("Sending command: " + pid + " on header: " + selectedHeader);
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
    lastQueryError = QUERY_TIMEOUT;
    return "";
}

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

double Commands::calculateValue(int commandIndex) const {
    switch (commandConfig[commandIndex].formula) {
        case FORMULA_ENGINE_LOAD:
            return ((double)A * 100.00) / 255.00;
        case FORMULA_TEMP_F:
            return (((double)A - 40.0) * 9.0 / 5.0) + 32.0;
        case FORMULA_FUEL_TRIM:
            return (((double)A - 128.0) * 100.0 / 128.0);
        case FORMULA_FUEL_PRESSURE:
            return ((double)A * 3.0 * 0.145);
        case FORMULA_PRESSURE_PSI:
            return ((double)A * 0.145);
        case FORMULA_RPM:
            return ((((double)A * 256.0) + (double)B) / 4.0);
        case FORMULA_SPEED_MPH:
            return ((double)A * 0.621371);
        case FORMULA_TIMING_ADVANCE:
            return (((double)A / 2.0) - 64.0);
        case FORMULA_MAF:
            return ((((double)A * 256.0) + (double)B) / 100.0);
        case FORMULA_RUNTIME_SECONDS:
            return (((double)A * 256.0) + (double)B);
        case FORMULA_FUEL_PRESSURE_REL_VAC:
            return ((((double)A * 256.0 + (double)B) * 0.079) * 0.145);
        case FORMULA_FUEL_PRESSURE_DIRECT:
            return ((((double)A * 256.0 + (double)B) * 10.0) * 0.145);
        case FORMULA_ABSOLUTE_LOAD:
            return ((((double)A * 256.0) + (double)B) * 100.0 / 255.0);
        case FORMULA_EQUIV_RATIO:
            return ((((double)A * 256.0) + (double)B) / 32768.0);
        case FORMULA_MAX_MAF:
            return ((double)A * 10.0);
        case FORMULA_EVAP_ABS_PRESSURE:
            return (((((double)A * 256.0) + (double)B) / 200.0) * 0.145);
        case FORMULA_EVAP_VAPOR_PA:
            return ((((double)A * 256.0) + (double)B) - 32768.0);
        case FORMULA_FUEL_RATE_GPH:
            return ((((double)A * 256.0) + (double)B) * 0.05 * 0.264172);
        case FORMULA_TORQUE_PERCENT:
            return ((double)A - 125.0);
        case FORMULA_REFERENCE_TORQUE_LBFT:
            return ((((double)A * 256.0) + (double)B) * 0.7376);
        case FORMULA_FUEL_INJECTION_TIMING:
            return (((((double)A * 256.0) + (double)B) - 26880.0) / 128.0);
        default:
            return 0.0;
    }
}

double Commands::queryCommand(int commandIndex) {
    lastQueryCommandIndex = commandIndex;
    lastQueryResponse = "";

    if (!connected || pWriteChar == nullptr) {
        lastQueryError = QUERY_NOT_CONNECTED;
        return 0.0;
    }

    if (commandIndex < 0 || commandIndex >= commandCount) {
        lastQueryError = QUERY_INVALID_INDEX;
        return 0.0;
    }

    String command = commandConfig[commandIndex].pid;
    int numBytes = commandConfig[commandIndex].numBytes;
    String header = commandConfig[commandIndex].header;

    lastQueryPid = command;
    lastQueryHeader = normalizeHeader(header.c_str());
    lastQueryError = QUERY_OK;

    String response = sendCommand(command, header);
    lastQueryResponse = response;
    if (response == "") {
        if (lastQueryError == QUERY_OK) {
            lastQueryError = QUERY_TIMEOUT;
        }
        return 0.0;
    }

    parsePIDResponse(response, command, numBytes);
    if (A < 0 || (numBytes > 1 && B < 0)) {
        lastQueryError = QUERY_PARSE_FAILED;
        return 0.0;
    }

    if (commandConfig[commandIndex].formula == FORMULA_INSTANT_MPG ||
        commandConfig[commandIndex].formula == FORMULA_AVERAGE_MPG) {
        return 0.0;
    }

    lastQueryError = QUERY_OK;
    return calculateValue(commandIndex);
}

double Commands::getReading(int selectedReading) {
    if (!connected || pWriteChar == nullptr) {
        return 0.0;
    }

    const int RPM_INDEX = 8;
    const int ENGINE_LOAD_INDEX = 0;
    const int BARO_INDEX = 17;
    const int REF_TORQUE_INDEX = 33;
    const int ACTUAL_TORQUE_INDEX = 32;
    const int SPEED_INDEX = 9;

    switch (selectedReading) {
        case 0:
            return queryCommand(RPM_INDEX);
        case 1: {
            double rpm = queryCommand(RPM_INDEX);
            double load = queryCommand(ENGINE_LOAD_INDEX);
            double baro = queryCommand(BARO_INDEX);
            double fRpm = std::min(1.0, (rpm - 1500.0) / (4000.0 - 1500.0));
            fRpm = std::max(0.0, fRpm);

            double boost = 22.0 * (load / 100.0) * fRpm;
            (void)baro;
            return boost;
        }
        case 2: {
            double ref = queryCommand(REF_TORQUE_INDEX);
            double actual = queryCommand(ACTUAL_TORQUE_INDEX);
            return (ref * (actual / 100.0));
        }
        case 3: {
            double rpm = queryCommand(RPM_INDEX);
            double refTorque = queryCommand(REF_TORQUE_INDEX);
            double actualTorque = queryCommand(ACTUAL_TORQUE_INDEX);
            return ((refTorque * (actualTorque / 100.0)) * rpm) / 5252.0;
        }
        case 5:
            return queryCommand(SPEED_INDEX);
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

    const int RPM_INDEX = 8;
    const int REF_TORQUE_INDEX = 33;
    const int ACTUAL_TORQUE_INDEX = 32;

    double rpm = queryCommand(RPM_INDEX);
    double refTorque = queryCommand(REF_TORQUE_INDEX);
    double actTorque = queryCommand(ACTUAL_TORQUE_INDEX);
    double torque = (refTorque * (actTorque / 100.0));
    double hp = ((refTorque * (actTorque / 100.0)) * rpm) / 5252.0;
    x.readings[0] = torque;
    x.readings[1] = hp;
    return x;
}

int Commands::getCommandCount() const {
    return commandCount;
}

String Commands::getCommandLabel(int commandIndex) const {
    if (commandIndex < 0 || commandIndex >= commandCount) {
        return "";
    }
    return String(commandConfig[commandIndex].label);
}

String Commands::getCommandUnits(int commandIndex) const {
    if (commandIndex < 0 || commandIndex >= commandCount) {
        return "";
    }
    return String(commandConfig[commandIndex].units);
}

double Commands::getValueByCommandIndex(int commandIndex) {
    if (commandIndex < 0 || commandIndex >= commandCount) {
        return 0.0;
    }

    static double averageMpg = 0.0;
    static bool averageInit = false;

    int formula = commandConfig[commandIndex].formula;
    if (formula == FORMULA_INSTANT_MPG || formula == FORMULA_AVERAGE_MPG) {
        double speedMph = queryCommand(9);
        double maf = queryCommand(12);

        if (maf <= 0.01) {
            return 0.0;
        }

        double instantMpg = speedMph * 710.7 / maf;
        instantMpg = constrain(instantMpg, 0.0, 100.0);

        if (!averageInit) {
            averageMpg = instantMpg;
            averageInit = true;
        } else {
            averageMpg = (averageMpg * 0.95) + (instantMpg * 0.05);
        }

        if (formula == FORMULA_INSTANT_MPG) {
            return instantMpg;
        }

        return constrain(averageMpg, 0.0, 100.0);
    }

    return queryCommand(commandIndex);
}

String Commands::getLastQueryDiagnostic() const {
    String status = "OK";
    switch (lastQueryError) {
        case QUERY_OK:
            status = "OK";
            break;
        case QUERY_NOT_CONNECTED:
            status = "NOT_CONNECTED";
            break;
        case QUERY_INVALID_INDEX:
            status = "INVALID_INDEX";
            break;
        case QUERY_TIMEOUT:
            status = "TIMEOUT";
            break;
        case QUERY_PARSE_FAILED:
            status = "PARSE_FAILED";
            break;
    }

    String label = "n/a";
    if (lastQueryCommandIndex >= 0 && lastQueryCommandIndex < commandCount) {
        label = String(commandConfig[lastQueryCommandIndex].label);
    }

    String rawResponse = lastQueryResponse;
    rawResponse.replace("\r", " ");
    rawResponse.replace("\n", " ");
    rawResponse.trim();

    return "Status=" + status +
           " | Index=" + String(lastQueryCommandIndex) +
           " | Label=" + label +
           " | PID=" + lastQueryPid +
           " | Header=" + lastQueryHeader +
           " | Raw='" + rawResponse + "'";
}

void Commands::initializeOBD() {}
