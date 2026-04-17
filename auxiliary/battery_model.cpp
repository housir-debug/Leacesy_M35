#include "battery_model.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>
#include <algorithm>

Q_LOGGING_CATEGORY(battery, "BATTERY:")

BatteryModel::BatteryModel(QObject *parent) : QObject(parent) {}

bool BatteryModel::isOver(float soc) const{
    if (!data_points.isEmpty()) {
        // From top to bottom, from small to large,Beyond the scope of the model
        if (soc >= data_points.first().soc && soc <= data_points.last().soc){
            return false;
        }
    }
    return true;
}

float BatteryModel::getOCV(float soc) const {
    if (!data_points.isEmpty()) {
        // // From top to bottom, from small to large,Beyond the scope of the model
        if (soc <= data_points.first().soc){
            return data_points.first().ocv;
        }

        if (soc >= data_points.last().soc){
            return data_points.last().ocv;
        }

        return interpolate(soc,true);
    }

    qCWarning(battery)<<"[getOCV]:data point is empty";
    return 0.0;
}

float BatteryModel::getESR(float soc) const {
    if (!data_points.isEmpty()) {
        // // From top to bottom, from small to large,Beyond the scope of the model
        if (soc <= data_points.first().soc){
            return data_points.first().imp;
        }

        if (soc >= data_points.last().soc){
            return data_points.last().imp;
        }

        return interpolate(soc,false);
    }

    qCWarning(battery)<<"[getESR]:data point is empty";
    return 0.0;
}

float BatteryModel::interpolate(float soc,bool isocv) const {
    int left = 0; int right = data_points.size() - 1;

    // From top to bottom, from small to large
    while (right - left > 1) {
        int mid = (left + right) / 2;
        (data_points[mid].soc <= soc) ? (left = mid) : (right = mid);
    }

    const auto& p1 = data_points[left];
    float y1 = isocv ? p1.ocv : p1.imp;
    const auto& p2 = data_points[left + 1];
    float y2 = isocv ? p2.ocv : p2.imp;

    float ad = (y2 - y1) * (soc - p1.soc) / (p2.soc - p1.soc);
    return y1 + ad;
}

// -------------------------------------------------------------------------------------------------

BatteryModelManager::BatteryModelManager(const QString &parentPath, QObject *parent): QObject(parent)
    , m_modelDirectory(parentPath + "/battery_models"){}

bool BatteryModelManager::loadAllModels() {
    QDir modelDir(m_modelDirectory);
    if (!modelDir.exists()) {
        qCDebug(battery) <<"[loadAllModels]:The model folder does not exist. Create this folder.";
        modelDir.mkpath(".");
    }

    m_models.clear();
    QStringList filters = {"*.csv", "*.CSV"};
    QFileInfoList fileList = modelDir.entryInfoList(filters, QDir::Files);

    for (const auto &fileInfo : qAsConst(fileList)) {
        auto model = parseCSV(fileInfo.absoluteFilePath());
        if (!model) {
            qCWarning(battery)<<"[loadAllModels]:Interrupted loading, failed load the file: "<< fileInfo.fileName();
            m_models.clear();
            return false;
        }

        m_models[model->name] = model;
    }

    return true;
}

bool BatteryModelManager::loadModel(const QString &modelName) {
    QString filePath = QDir(m_modelDirectory).filePath(modelName + ".csv");
    QFileInfo fileInfo(filePath);

    if (fileInfo.exists()) {
        auto model = parseCSV(filePath);
        if (model) {
            m_models[model->name] = model;
            return true;
        }
    }

    qCWarning(battery) <<"[loadModel]: Not exist or Failure processing. The model file" << fileInfo.fileName();
    return false;
}

QSharedPointer<BatteryModel> BatteryModelManager::parseCSV(const QString &filePath) {
    QFileInfo fileInfo(filePath);

    auto model = QSharedPointer<BatteryModel>::create();
    model->name = fileInfo.baseName();  // The file name not include extension.

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString line;
        QTextStream stream(&file);

        int socCol = -1, ocvCol = -1, esrCol = -1;
        //bool isFirstLine = true;
        int lineNumber = 0;

        // Read each line of the file sequentially
        while (stream.readLineInto(&line)) { // line ? /n
            lineNumber++;

            // read line and column
            bool inQuotes = false;
            QStringList columns;
            QString cell;

            for (int i = 0; i < line.length(); ++i) {
                if (line[i] == '"') {
                    inQuotes = !inQuotes;
                    // The commas not within the quotation marks are the separators.
                } else if (line[i] == ',' && !inQuotes) {
                    cell = cell.trimmed();
                    columns.append(cell);
                    cell.clear();
                } else {
                    cell += line[i];
                }
            }
            // add last baecase not have ,
            columns.append(cell);

            // process line information
            if (columns.size() == 3) {
                // first line
                if (socCol == -1 || ocvCol == -1 || esrCol == -1) {
                    for (int i = 0; i < columns.size(); ++i) {
                        QString normalized = columns[i].trimmed();
                        if (normalized=="SOC_%")        {socCol = i;}
                        else if (normalized=="OCV_V")   {ocvCol = i;}
                        else if (normalized=="IMP_ohm") {esrCol = i;}
                    }

                    continue; // access next line
                }

                // Other line
                BatteryDataPoint point;

                point.soc = columns[socCol].toFloat();
                if (0.0f <= point.soc <= 100.0f) {
                    point.ocv = columns[ocvCol].toFloat();
                    if (0.0f <= point.ocv <= 6.0f) {
                        point.imp = columns[esrCol].toFloat();
                        if (0.0f <= point.imp <= 1.0f) {
                            model->data_points.append(point);
                            continue; // access next line
                        }
                    }
                }

                qCWarning(battery)<<"[parseCSV]: line failed: "<<lineNumber;
                return nullptr;
            }

            qCWarning(battery)<<"[parseCSV]: format failed!";
            return nullptr;
        }

        std::sort(model->data_points.begin(), model->data_points.end(),[]
              (const BatteryDataPoint &a, const BatteryDataPoint &b) {
                  // Ascending (from smallest to largest)
                  return a.soc < b.soc;
              });

        return model;
    }

    qCWarning(battery)<<"[parseCSV]:failed open file!" << filePath;
    return nullptr;
}

bool BatteryModelManager::removeModel(const QString &modelName) {
    if (m_models.contains(modelName)) {
        QString filePath = QDir(m_modelDirectory).filePath(modelName + ".csv");
        QFile file(filePath);

        if (file.exists()) {
            if (file.remove()) {
                m_models.remove(modelName);
                return true;
            }

            qCWarning(battery)<<"[removeModel]: failed delete file: "<<filePath<<" | error:" << file.errorString();
            return false;
        }
    }

    qCWarning(battery) <<"[removeModel]: model not exist: "<<modelName;
    return false;
}

bool BatteryModelManager::saveModel(QSharedPointer<BatteryModel> model,const QString &modelName) {
    std::sort(model->data_points.begin(), model->data_points.end(),[]
          (const BatteryDataPoint &a, const BatteryDataPoint &b){
              // Ascending (from smallest to largest)
              return a.soc < b.soc;
          });

    QString filePath = QDir(m_modelDirectory).filePath(modelName + ".csv");
    if(!QFile::exists(filePath)){
        QDir dir(m_modelDirectory);
        if (!dir.exists()) {
            qCDebug(battery) <<"[saveModel]:The model folder does not exist. Create this folder.";
            dir.mkpath(".");
        }

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << "SOC_%,OCV_V,IMP_ohm\n";

            const auto &points = model->data_points;
            for (const auto &point : points) {
                stream << QString::number(point.soc, 'f', 3) << ","
                       << QString::number(point.ocv, 'f', 3) << ","
                       << QString::number(point.imp, 'f', 3) << "\n";
            }

            m_models[model->name] = model;
            file.close();
            return true;
        }

        qCWarning(battery)<<"[saveModel]: not create model file: "<<filePath;
        return false;
    }

    qCWarning(battery)<<"[saveModel]: model file already exist!";
    return false;
}




