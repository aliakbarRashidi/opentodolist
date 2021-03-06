#include "jsonutils.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>


namespace JsonUtils {

Q_LOGGING_CATEGORY(jsonUtils, "net.rpdev.opentodolist.JsonUtils", QtWarningMsg)

/**
 * @brief Write a JSON file, keeping existing properties in the file.
 *
 * This function can be used to write the properties contained in @p data to the
 * filename. If filename points to an existing file, any properties that exist in the
 * file but not in data will be preserved.
 *
 * The intention of this function is to keep a set of files backwards compatible (e.g. if
 * users switch between versions of the application).
 *
 * The function returns true on success or false otherwise.
 */
bool patchJsonFile(const QString& filename, const QVariantMap& data)
{
    bool result = false;
    QFile file(filename);
    QVariantMap properties;
    QByteArray existingFileContent;
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            QJsonParseError error;
            existingFileContent = file.readAll();
            auto doc = QJsonDocument::fromJson(existingFileContent, &error);
            if (error.error == QJsonParseError::NoError) {
                properties = doc.toVariant().toMap();
            } else {
                qCWarning(jsonUtils) << "Failed to parse" << filename << ":"
                                     << error.errorString();
            }
            file.close();
        } else {
            qCWarning(jsonUtils) << "Failed to open" << filename << "for reading:"
                                 << file.errorString();
        }
    }
    for (auto key : data.keys()) {
        properties[key] = data[key];
    }
    auto doc = QJsonDocument::fromVariant(properties);
    auto newFileContent = doc.toJson(QJsonDocument::Indented);
    if (newFileContent != existingFileContent) {
        if (file.open(QIODevice::WriteOnly)) {
            result = newFileContent.length() == file.write(newFileContent);
            file.close();
        } else {
            qCWarning(jsonUtils) << "Failed to open" << filename << "for writing:"
                                 << file.errorString();
        }
    } else {
        qCDebug(jsonUtils) << "File" << filename << "was not changed - "
                           << "skipping rewrite";
        result = true;
    }
    return result;
}


/**
 * @brief Load a variant map from a JSON file.
 */
QVariantMap loadMap(const QString& filename, bool* ok)
{
    bool success = false;
    QVariantMap result;
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error == QJsonParseError::NoError) {
            result = doc.toVariant().toMap();
            success = true;
        } else {
            qCWarning(jsonUtils) << "Failed to parse" << filename << ":"
                                 << error.errorString();
        }
        file.close();
    } else {
        qCWarning(jsonUtils) << "Failed to open" << filename << "for reading:"
                             << file.errorString();
    }
    if (ok != nullptr) {
        *ok = success;
    }
    return result;
}

}

