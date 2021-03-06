#include "webdavsynchronizer.h"

#include "webdavclient.h"

#include "datamodel/library.h"

#include <QBuffer>
#include <QDir>
#include <QDomDocument>
#include <QEventLoop>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryFile>
#include <QTimer>


Q_LOGGING_CATEGORY(webDAVSynchronizer,
                   "net.rpdev.opentodolist.WebDAVSynchronizer",
                   QtDebugMsg)



const QString WebDAVSynchronizer::SyncLockFileName = ".webdav-sync-running";


WebDAVSynchronizer::WebDAVSynchronizer(QObject* parent) :
    Synchronizer(parent),
    m_remoteDirectory(),
    m_disableCertificateCheck(false),
    m_username(),
    m_password(),
    m_createDirs(false),
    m_stopRequested(false),
    m_findExistingEntriesWatcher()
{
    connect(&m_findExistingEntriesWatcher,
            &QFutureWatcher<QVariantList>::finished, [=]() {
        setExistingLibraries(m_findExistingEntriesWatcher.result());
        setFindingLibraries(false);
    });
    connect(this, &WebDAVSynchronizer::passwordChanged,
            this, &WebDAVSynchronizer::secretChanged);
}

WebDAVSynchronizer::~WebDAVSynchronizer()
{
    if (m_findExistingEntriesWatcher.isRunning()) {
        m_findExistingEntriesWatcher.waitForFinished();
    }
}

QUrl WebDAVSynchronizer::baseUrl() const
{
    switch (m_serverType) {
    case NextCloud:
    case OwnCloud:
    {
        auto url = m_url.toString();
        if (!url.endsWith("/")) {
            url += "/";
        }
        url += "remote.php/webdav/";
        return QUrl(url);
    }
    default:
        return m_url;
        break;
    }
}

void WebDAVSynchronizer::validate()
{
    beginValidation();
    auto dav = createDAVClient(this);
    auto reply = dav->listDirectoryRequest("/");
    reply->setParent(this);
    connect(reply, &QNetworkReply::finished, [=]() {
        auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == HTTPStatusCode::WebDAVMultiStatus) {
            endValidation(true);
        } else {
            endValidation(false);
        }
        reply->deleteLater();
        dav->deleteLater();
    });
}

void WebDAVSynchronizer::synchronize()
{
    if (!directory().isEmpty() && !synchronizing()) {
        debug() << tr("Starting synchronization");
        auto dav = createDAVClient(this);
        connect(this, &WebDAVSynchronizer::stopRequested,
                dav, &WebDAVClient::stopSync);
        connect(dav, &WebDAVClient::debug, [=](const QString& message) {
            debug() << message;
        });
        connect(dav, &WebDAVClient::warning, [=](const QString& message) {
            warning() << message;
        });
        connect(dav, &WebDAVClient::error, [=](const QString& message) {
            error() << message;
        });
        connect(dav, &WebDAVClient::syncError,
                this, &WebDAVSynchronizer::syncError);
        setSynchronizing(true);
        m_stopRequested = false;
        bool fullSync = false;
        {
            QDir syncDir(directory());
            if (syncDir.exists(SyncLockFileName)) {
                // If the last sync did not run through, do a full sync,
                // i.e. request etags for all directories.
                fullSync = true;
                qCWarning(webDAVSynchronizer) << "The last sync did not "
                                                 "complete - doing a full "
                                                 "sync";
                warning() << tr("The last sync did not run through - "
                                "doing a full sync");
            }
            QFile file(syncDir.absoluteFilePath(SyncLockFileName));
            if (!file.open(QIODevice::WriteOnly)) {
                qCWarning(webDAVSynchronizer) << "Failed to create sync lock:"
                                              << file.errorString();
                error() << tr("Failed to create sync lock:")
                        << file.errorString();
            } else {
                file.close();
            }
        }
        // Sync the top level directory:
        bool dirsOkay = true;
        if (m_createDirs) {
            debug() << tr("Creating the remote top level directory");
            dav->setRemoteDirectory("");
            auto parts = QDir::cleanPath(m_remoteDirectory).split("/");
            QString rpath;
            for (auto part : parts) {
                rpath += "/" + part;
                if (dav->etag(rpath) == "") {
                    auto ok = dav->mkdir(rpath);
                    if (!ok) {
                        qCWarning(webDAVSynchronizer)
                                << "Failed to prepare remote dir" << rpath
                                << "for sync.";
                        error() << tr("Failed to prepare remote directory '%1' "
                                      "for sync").arg(rpath);
                        dirsOkay = false;
                        break;
                    } else {
                        QThread::sleep(1);
                    }
                }
            }
            if (dirsOkay) {
                m_createDirs = false;
                save();
            }
            dav->setRemoteDirectory(remoteDirectory());
        }

        if (dirsOkay) {
            QSet<QString> changedYearDirs;
            if (!dav->syncDirectory("/",
                               QRegularExpression("\\d\\d\\d\\d"),
                               false,
                                    &changedYearDirs)) {
                warning() << tr("Failed to synchronize top level "
                                "directory!");
            }
            // Sync the year directory:
            QDir dir(directory());
            for (auto yearDir : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (m_stopRequested) {
                    break;
                }
                QSet<QString> changedMonthDirs;
                if (!dav->syncDirectory("/" + yearDir,
                                   QRegularExpression("\\d\\d?"),
                                   fullSync || !changedYearDirs.contains(yearDir),
                                        &changedMonthDirs)) {
                    warning() << tr("Failed to synchronize '%1'").arg(
                                     "/" + yearDir);
                }
                QDir ydir(dir.absoluteFilePath(yearDir));
                for (auto monthDir : ydir.entryList(
                         QDir::Dirs | QDir::NoDotAndDotDot)) {
                    if (m_stopRequested) {
                        break;
                    }
                    if (!dav->syncDirectory("/" + yearDir + "/" + monthDir,
                                       QRegularExpression(),
                                            fullSync || !changedMonthDirs.contains(monthDir))) {
                        warning() << tr("Failed to synchronize '%1'").arg(
                                         "/" + yearDir + "/" + monthDir);
                    }
                }
            }
        }
        if (!m_stopRequested) {
            QDir syncDir(directory());
            syncDir.remove(SyncLockFileName);
        }
        setSynchronizing(false);
        delete dav;

        debug() << tr("Synchronization finished");
    }
}

void WebDAVSynchronizer::stopSync()
{
    qCWarning(webDAVSynchronizer) << "Stopping WebDAV sync";
    m_stopRequested = true;
    emit stopRequested();
}



/**
 * @brief Search for existing libraries.
 *
 * This method will search for existing libraries in the current
 * remote directory and the OpenTodoList sub-folder (relative to the
 * remote directory). The
 */
void WebDAVSynchronizer::findExistingLibraries()
{
    setFindingLibraries(true);
    auto baseUrl = this->baseUrl();
    auto username = this->username();
    auto password = this->password();
    auto disableCertificateCheck = this->disableCertificateCheck();
    auto remoteDirectory = this->remoteDirectory();
    auto directory = this->directory();

    m_findExistingEntriesWatcher.setFuture(
                QtConcurrent::run([=]() -> QVariantList {
        WebDAVClient client;
        client.setBaseUrl(baseUrl);
        client.setUsername(username);
        client.setPassword(password);
        client.setDisableCertificateCheck(disableCertificateCheck);
        client.setRemoteDirectory(remoteDirectory);
        client.setDirectory(directory);

        QStringList dirsToCheck;
        for (auto baseDir : QStringList({"/", "/OpenTodoList"})) {
            auto entryList = client.entryList(baseDir);
            for (auto entry : entryList) {
                if (entry.type == WebDAVClient::Directory &&
                        entry.name.endsWith(".otl")) {
                    dirsToCheck.append(client.mkpath(baseDir + "/" +
                                                     entry.name));
                }
            };
        }
        QVariantList result;
        for (auto dir : dirsToCheck) {
            QByteArray json;
            QBuffer buffer(&json);
            buffer.open(QIODevice::WriteOnly);
            if (client.download(dir + "/" + Library::LibraryFileName,
                                &buffer)) {
                if (!json.isEmpty()) {
                    auto doc = QJsonDocument::fromJson(json);
                    if (doc.isObject()) {
                        auto map = doc.toVariant().toMap();
                        SynchronizerExistingLibrary library;
                        library.setName(map.value("name").toString());
                        auto path = dir;
                        if (!path.startsWith("/")) {
                            path.prepend("/");
                        }
                        library.setPath(path);
                        library.setUid(map.value("uid").toUuid());
                        result << QVariant::fromValue(library);
                    }
                }
            }
        }
        return result;
    }));
}


QVariantMap WebDAVSynchronizer::toMap() const
{
    auto result = Synchronizer::toMap();
    result["username"] = m_username;
    result["remoteDirectory"] = m_remoteDirectory;
    result["disableCertificateCheck"] = m_disableCertificateCheck;
    result["url"] = m_url;
    result["serverType"] = QVariant::fromValue(m_serverType);
    result["createDirs"] = m_createDirs;
    return result;
}

void WebDAVSynchronizer::fromMap(const QVariantMap& map)
{
    m_username = map.value("username").toString();
    m_remoteDirectory = map.value("remoteDirectory").toString();
    m_disableCertificateCheck = map.value("disableCertificateCheck").toBool();
    m_url = map.value("url").toUrl();
    m_serverType = map.value("serverType").value<WebDAVServerType>();
    m_createDirs = map.value("createDirs", false).toBool();
    Synchronizer::fromMap(map);
}

QString WebDAVSynchronizer::secretsKey() const
{
    return this->uid().toString();
}

QString WebDAVSynchronizer::secret() const
{
    return password();
}

void WebDAVSynchronizer::setSecret(const QString &secret)
{
    setPassword(secret);
}

QString WebDAVSynchronizer::remoteDirectory() const
{
    return m_remoteDirectory;
}

void WebDAVSynchronizer::setRemoteDirectory(const QString& remoteDirectory)
{
    if (m_remoteDirectory != remoteDirectory) {
        m_remoteDirectory = remoteDirectory;
        emit remoteDirectoryChanged();
    }
}

bool WebDAVSynchronizer::disableCertificateCheck() const
{
    return m_disableCertificateCheck;
}

void WebDAVSynchronizer::setDisableCertificateCheck(bool disableCertificateCheck)
{
    if (m_disableCertificateCheck != disableCertificateCheck) {
        m_disableCertificateCheck = disableCertificateCheck;
        setValid(false);
        emit disableCertificateCheckChanged();
    }
}

QString WebDAVSynchronizer::username() const
{
    return m_username;
}

void WebDAVSynchronizer::setUsername(const QString& username)
{
    if (m_username != username) {
        m_username = username;
        setValid(false);
        emit usernameChanged();
    }
}

QString WebDAVSynchronizer::password() const
{
    return m_password;
}

void WebDAVSynchronizer::setPassword(const QString& password)
{
    if (m_password != password) {
        m_password = password;
        setValid(false);
        emit passwordChanged();
    }
}


/**
 * @brief The URL to connect to.
 */
QUrl WebDAVSynchronizer::url() const
{
    return m_url;
}


/**
 * @brief Set the server URL.
 */
void WebDAVSynchronizer::setUrl(const QUrl &url)
{
    if (m_url != url) {
        m_url = url;
        setValid(false);
        emit urlChanged();
    }
}


/**
 * @brief Create a DAV client.
 *
 * This creates and returns a new WebDAVClient which belongs to the
 * @p parent. The client will have the settings currently set in the
 * WebDAVSynchronizer.
 *
 * @note Ownership of the object goes to the caller.
 */
WebDAVClient* WebDAVSynchronizer::createDAVClient(QObject *parent)
{
    auto result = new WebDAVClient(parent);
    result->setBaseUrl(baseUrl());
    result->setUsername(username());
    result->setPassword(password());
    result->setDisableCertificateCheck(disableCertificateCheck());
    result->setRemoteDirectory(remoteDirectory());
    result->setDirectory(directory());
    return result;
}


/**
 * @brief The type of WebDAV server to connect to.
 */
WebDAVSynchronizer::WebDAVServerType WebDAVSynchronizer::serverType() const
{
    return m_serverType;
}

void WebDAVSynchronizer::setServerType(const WebDAVServerType &serverType)
{
    if (m_serverType != serverType) {
        m_serverType = serverType;
        setValid(false);
        emit serverTypeChanged();
    }
}

bool WebDAVSynchronizer::createDirs() const
{
    return m_createDirs;
}

void WebDAVSynchronizer::setCreateDirs(bool createDirs)
{
    m_createDirs = createDirs;
}
