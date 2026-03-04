#ifndef COMMANDS_H
#define COMMANDS_H

#include "bluetooth.h"

struct dualGaugeReading {
    double readings[2];
};

struct pidCommandDefinition {
    const char* label;
    const char* units;
    const char* pid;
    double minValue;
    double maxValue;
    int numBytes;
    int formula;
    const char* header;
};

class Commands {
private:
    int A;
    int B;
    static const pidCommandDefinition commandConfig[];
    static const int commandCount;
    void parsePIDResponse(String response, String pid, int numBytes);
    String normalizeHeader(const char* header) const;
    double queryCommand(int commandIndex);
    double calculateValue(int commandIndex) const;

public:
    Commands();
    double getReading(int selectedGauge);
    struct dualGaugeReading getDualReading();
    void initializeOBD();
    String sendCommand(String pid, String header = "0");

    int getCommandCount() const;
    String getCommandLabel(int commandIndex) const;
    String getCommandUnits(int commandIndex) const;
    double getValueByCommandIndex(int commandIndex);
};

#endif // COMMANDS_H
