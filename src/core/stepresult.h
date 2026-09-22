#pragma once
#include <QString>

// Outcome of one write into one sink (a file, a gsettings key, a D-Bus signal).
struct StepResult {
    enum Status { Ok, Warning, Failed };

    Status status = Ok;
    QString detail;

    static StepResult ok(const QString &detail = QString())   { return {Ok, detail}; }
    static StepResult warn(const QString &detail)             { return {Warning, detail}; }
    static StepResult fail(const QString &detail)             { return {Failed, detail}; }

    bool isOk() const { return status == Ok; }
};
