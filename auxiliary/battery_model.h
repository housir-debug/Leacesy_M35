#pragma once
#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>
#include <QSharedPointer>
#include <QDir>
#include <QDebug>

struct BatteryDataPoint {
    float soc;      // State of Charge (0-100%)
    float ocv;      // Open Circuit Voltage (V)
    float esr;      // Equivalent Series Resistance (Ω)
};

class BatteryModel : public QObject {
    Q_OBJECT
public:
    explicit BatteryModel(QObject *parent = nullptr);

    QString name;
    QVector<BatteryDataPoint> data_points;

    float getOCV(float soc) const;
    float getESR(float soc) const;
    float interpolate(float soc,bool isocv) const;

    bool isValid() const;
    bool isOver(float soc) const;
};

class BatteryModelManager : public QObject {
    Q_OBJECT
public:
    BatteryModelManager(const QString& parentPath, QObject *parent = nullptr);
    ~BatteryModelManager() override = default;

    bool loadAllModels();
    bool loadModel(const QString &modelName);
    static QSharedPointer<BatteryModel> parseCSV(const QString &filePath);

    QStringList getAvailableModels() const{return m_models.keys();};
    QSharedPointer<BatteryModel> getModel(const QString &modelName) const{return m_models.value(modelName);};

private:
    QString m_modelDirectory;
    QMap<QString, QSharedPointer<BatteryModel>> m_models;
};
