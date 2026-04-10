#include "battery_model.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>
#include <algorithm>

BatteryModel::BatteryModel(QObject *parent) : QObject(parent) {}

float BatteryModel::getOCV(float soc) const {
    if (data_points.isEmpty()) {return 0.0;}

    // // From top to bottom, from small to large,Beyond the scope of the model
    if (soc <= data_points.first().soc){ return data_points.first().ocv;}
    if (soc >= data_points.last().soc){ return data_points.last().ocv;}

    return interpolate(soc,true);
}

float BatteryModel::getESR(float soc) const {
    if (data_points.isEmpty()) {return 0.0;}

    // // From top to bottom, from small to large,Beyond the scope of the model
    if (soc <= data_points.first().soc){ return data_points.first().imp;}
    if (soc >= data_points.last().soc){ return data_points.last().imp;}

    return interpolate(soc,false);
}

float BatteryModel::interpolate(float soc,bool isocv) const {
    int left = 0;
    int right = data_points.size() - 1;

    // From top to bottom, from small to large
    while (right - left > 1) {
        int mid = (left + right) / 2;
        if (data_points[mid].soc <= soc) {
            left = mid;
        } else {
            right = mid;
        }
    }

    const auto& p1 = data_points[left];
    const auto& p2 = data_points[left + 1];

    float y1 = isocv ? p1.ocv : p1.imp;
    float y2 = isocv ? p2.ocv : p2.imp;

    if (p2.soc - p1.soc < 1e-6) {return y1;}  // Avoid division by 0
    return y1 + (y2 - y1) * (soc - p1.soc) / (p2.soc - p1.soc);
}

bool BatteryModel::isValid() const {
    if (name.isEmpty() || data_points.isEmpty()) {
        qWarning() << "BatteryModel::isValid: 模型名称空或数据点空";
        return false;
    }

    double last_soc = -1.0;
    for (const auto& point : data_points) {
        if (point.soc <= last_soc) {
            qWarning() << "BatteryModel::isValid: SOC不是严格递增" << point.soc << "<=" << last_soc;
            return false;
        }

        if (point.soc < 0.0 || point.soc > 100.0) {
            qWarning() << "BatteryModel::isValid: SOC超出范围 [0,100]" << point.soc;
            return false;
        }

        if (point.ocv < 0.0 || point.ocv > 10.0) {
            qWarning() << "BatteryModel::isValid: OCV超出合理范围 [0,10]" << point.ocv;
            return false;
        }

        if (point.imp < 0.0 || point.imp > 1.0) {
            qWarning() << "BatteryModel::isValid: ESR超出合理范围 [0,1]" << point.imp;
            return false;
        }

        last_soc = point.soc;
    }

    if (data_points.first().soc > 0.001 || data_points.last().soc < 99.999) {
        qWarning() << "BatteryModel::isValid: SOC范围["<< data_points.first().soc << "% - " << data_points.last().soc << "%]";
        //return false;
    }

    return true;
}

bool BatteryModel::isOver(float soc) const{
    if (data_points.isEmpty()) {return true;}

    // From top to bottom, from small to large,Beyond the scope of the model
    if (soc <= data_points.first().soc){ return true;}
    if (soc >= data_points.last().soc){ return true;}

    return false;
}

// ---------------------------------------BatteryModelManager

BatteryModelManager::BatteryModelManager(const QString &parentPath, QObject *parent)
    : QObject(parent)
    , m_modelDirectory(parentPath + "/battery_models"){}

bool BatteryModelManager::loadAllModels() {
    m_models.clear();

    QDir modelDir(m_modelDirectory);
    if (!modelDir.exists()) {
        qWarning() << "模型目录不存在,尝试创建目录并结束加载:" << m_modelDirectory;
        modelDir.mkpath(".");
        return false;
    }

    QStringList filters;
    filters << "*.csv" << "*.CSV";
    QFileInfoList fileList = modelDir.entryInfoList(filters, QDir::Files);

    int loadedCount = 0;
    for (const auto &fileInfo : qAsConst(fileList)) {
        auto model = parseCSV(fileInfo.absoluteFilePath());

        if (model && model->isValid()) {
            m_models[model->name] = model;
            loadedCount++;
            qDebug() << "成功加载电池模型:" << model->name;
        } else {
            qWarning() << "警告: 跳过无效的电池模型文件:" << fileInfo.fileName();
        }
    }

    qDebug() << "共加载" << loadedCount << "个电池模型";
    return loadedCount > 0;
}

bool BatteryModelManager::loadModel(const QString &modelName) {
    QString filePath = QDir(m_modelDirectory).filePath(modelName + ".csv");
    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        qWarning() << "模型不存在,结束加载:" << m_modelDirectory;
        return false;
    }

    auto model = parseCSV(filePath);
    if (model && model->isValid()) {
        m_models[model->name] = model;
        qDebug() << "成功加载电池模型:" << model->name;
        return true;
    }

    qWarning() << "警告: 跳过无效的电池模型文件:" << fileInfo.fileName();
    return false;
}

QSharedPointer<BatteryModel> BatteryModelManager::parseCSV(const QString &filePath) {
    auto model = QSharedPointer<BatteryModel>::create();
    QFileInfo fileInfo(filePath);
    model->name = fileInfo.baseName();  // The file name not include extension.

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件:" << filePath;
        return nullptr;
    }

    QTextStream stream(&file);
    QString line;
    int lineNumber = 0;
    bool isFirstLine = true;
    int socCol = -1, ocvCol = -1, esrCol = -1;

    while (stream.readLineInto(&line)) {
        lineNumber++;
        if (line.trimmed().isEmpty()) {continue;}

        // read line and column
        QStringList columns;
        bool inQuotes = false;
        QString cell;

        for (int i = 0; i < line.length(); ++i) {
            QChar ch = line[i];
            if (ch == '"') {
                inQuotes = !inQuotes;
                // The commas not within the quotation marks are the separators.
            } else if (ch == ',' && !inQuotes) {
                cell = cell.trimmed();
                columns.append(cell);
                cell.clear();
            } else {
                cell += ch;
            }
        }
        columns.append(cell);

        // process line information
        if (columns.size() != 3) {
            qWarning() << "CSV行" << lineNumber << "列数错误";
            return nullptr;
        }

        if (isFirstLine) {
            for (int i = 0; i < columns.size(); ++i) {
                QString normalized = columns[i].toLower().trimmed();

                if (normalized=="soc" && socCol == -1) {
                    socCol = i;
                }
                else if (normalized=="ocv" && ocvCol == -1) {
                    ocvCol = i;
                }
                else if (normalized=="esr" && esrCol == -1) {
                    esrCol = i;
                }
                else{
                    qWarning() << "出现未知列名！";
                    return nullptr;
                }
            }

            if (socCol == -1 || ocvCol == -1 || esrCol == -1) {
                qWarning() << "CSV文件缺少必需列:"<< filePath<< "socCol=" << socCol<< "ocvCol=" << ocvCol<< "esrCol=" << esrCol;
                return nullptr;
            }

            isFirstLine = false;
            continue;
        }

        bool ok = true;
        BatteryDataPoint point;

        point.soc = columns[socCol].toFloat(&ok);
        if (!ok || point.soc<0 || point.soc>100) {
            qWarning() << "解析CSV行" << lineNumber << "SOC失败:" << columns[socCol];
            return nullptr;
        }

        point.ocv = columns[ocvCol].toFloat(&ok);
        if (!ok || point.ocv<0 || point.ocv>6) {
            qWarning() << "解析CSV行" << lineNumber << "OCV失败:" << columns[ocvCol];
            return nullptr;
        }

        point.imp = columns[esrCol].toFloat(&ok);
        if (!ok || point.imp<0 || point.imp>1) {
            qWarning() << "解析CSV行" << lineNumber << "ESR失败:" << columns[esrCol];
            return nullptr;
        }

        model->data_points.append(point);
    }

    std::sort(model->data_points.begin(), model->data_points.end(),[]
          (const BatteryDataPoint &a, const BatteryDataPoint &b) {
              // Ascending (from smallest to largest)
              return a.soc < b.soc;
          });

    return model;
}

bool BatteryModelManager::saveModel(QSharedPointer<BatteryModel> model,const QString &modelName) {
    if (!model) {
        qWarning() << "saveModelToCSV: 模型指针丢失";
        return false;
    }

    m_models[model->name] = model;
    QString filePath = QDir(m_modelDirectory).filePath(modelName + ".csv");
    if(QFile::exists(filePath)){
        qWarning() << "saveModelToCSV: 模型文件已存在";
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "saveModelToCSV: 无法创建文件:" << filePath;
        return false;
    }

    QTextStream stream(&file);
    stream << "SOC_%,OCV_V,IMP_ohm\n";
    const auto &points = model->data_points;
    for (const auto &point : points) {
        stream << QString::number(point.soc, 'f', 3) << ","
               << QString::number(point.ocv, 'f', 3) << ","
               << QString::number(point.imp, 'f', 3) << "\n";
    }

    file.close();
    qDebug() << "成功保存模型到文件:" << filePath << ", 数据点数量:" << model->data_points.size();
    return true;
}

bool BatteryModelManager::removeModel(const QString &modelName) {
    if (!m_models.contains(modelName)) {
        qWarning() << "removeModel: 找不到指定模型:" << modelName;
        return false;
    }

    QString filePath = QDir(m_modelDirectory).filePath(modelName + ".csv");
    QFile file(filePath);

    if (file.exists()) {
        if (file.remove()) {
            m_models.remove(modelName);
            qDebug() << "成功删除电池模型文件:" << filePath;
            return true;
        }

        qWarning() << "removeModel: 删除文件失败:" << filePath << ", 错误:" << file.errorString();
        return false;

    } else {
        qWarning() << "removeModel: 模型文件不存在:" << filePath;
        return false;
    }
}


