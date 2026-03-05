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
    enum QueryError {
        QUERY_OK = 0,
        QUERY_NOT_CONNECTED,
        QUERY_INVALID_INDEX,
        QUERY_TIMEOUT,
        QUERY_PARSE_FAILED
    };

    int A;
    int B;
    QueryError lastQueryError;
    int lastQueryCommandIndex;
    String lastQueryPid;
    String lastQueryHeader;
    String lastQueryResponse;
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
    String getLastQueryDiagnostic() const;
};

#endif // COMMANDS_H
