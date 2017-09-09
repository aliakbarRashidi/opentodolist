#include "webdavsynchronizer.h"

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
#include <QSqlRecord>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QTimer>


Q_LOGGING_CATEGORY(webDAVSynchronizer,
                   "net.rpdev.opentodolist.WebDAVSynchronizer",
                   QtDebugMsg)

enum class HTTPStatusCode {
    Continue = 100,
    SwitchingProtocols = 101,
    WebDAVProcessing = 102,
    OK = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    WebDAVMultiStatus = 207,
    WebDAVAlreadyReported = 208,
    IMUsed = 226,
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    Unused = 306,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    RequestEntityTooLarge = 413,
    RequestURITooLong = 414,
    UnsupportedMediaType = 415,
    RequestedRangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    ImATeapot = 418,
    TwiterEnhanceYourCalm = 420,
    WebDAVUnprocessableEntity = 422,
    WebDAVLocked = 423,
    WebDAVFailedDependency = 424,
    WebDAVReserved = 425,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    NginxNoResponse = 444,
    MicrosoftRetryWith = 449,
    MicrosoftBlockedByWindowsParentalControls = 450,
    UnavailableForLegalReasons = 451,
    NginxClientClosedRequest = 499,
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505,
    VariantAlsoNegotiates = 506,
    WebDAVInsufficientStorage = 507,
    WebDAVLoopDetected = 508,
    ApacheBandwidthLimitExceeded = 509,
    NotExtended = 510,
    NetworkAuthenticationRequired = 511,
    NetworkReadTimeoutError = 598,
    NetworkConnectTimeoutError = 599
};


bool operator ==(int lhs, HTTPStatusCode rhs) {
    return lhs == static_cast<int>(rhs);
}


bool operator ==(HTTPStatusCode lhs, int rhs) {
    return static_cast<int>(lhs) == rhs;
}


WebDAVSynchronizer::WebDAVSynchronizer(QObject* parent) :
    Synchronizer(parent),
    m_networkAccessManager(new QNetworkAccessManager(this)),
    m_remoteDirectory(),
    m_disableCertificateCheck(false),
    m_username(),
    m_password()
{

}

WebDAVSynchronizer::~WebDAVSynchronizer()
{

}

void WebDAVSynchronizer::validate()
{
    beginValidation();
    auto reply = listDirectoryRequest("/");
    reply->setParent(this);
    connect(reply, &QNetworkReply::finished, [=]() {
        auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == HTTPStatusCode::WebDAVMultiStatus) {
            endValidation(true);
        } else {
            endValidation(false);
        }
        reply->deleteLater();
    });
}

void WebDAVSynchronizer::synchronize()
{
    // TODO: Implement me
}


QVariantMap WebDAVSynchronizer::toMap() const
{
    auto result = Synchronizer::toMap();
    result["username"] = m_username;
    result["remoteDirectory"] = m_remoteDirectory;
    result["disableCertificateCheck"] = m_disableCertificateCheck;
    return result;
}

void WebDAVSynchronizer::fromMap(const QVariantMap& map)
{
    m_username = map.value("username").toString();
    m_remoteDirectory = map.value("remoteDirectory").toString();
    m_disableCertificateCheck = map.value("disableCertificateCheck").toBool();
    Synchronizer::fromMap(map);
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
        emit passwordChanged();
    }
}


/**
 * @brief Get the list of entries on the given @p directory.
 *
 * This retrieves the contents of the @p directory (which is relative to the
 * remoteDirectory set in the WebDAVSynchronizer itself).
 */
WebDAVSynchronizer::EntryList WebDAVSynchronizer::entryList(const QString& directory, bool *ok)
{
    // TODO: Implement me
    Q_UNUSED(directory);
    EntryList result;
    auto dir = this->remoteDirectory() + "/" + directory;
    dir = QDir::cleanPath(dir);
    auto reply = listDirectoryRequest(dir);
    bool status = false;
    waitForReplyToFinish(reply);
    auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (code == HTTPStatusCode::WebDAVMultiStatus) {
        result = parseEntryList(dir, reply->readAll());
        status = true;
    }
    if (ok != nullptr) {
        *ok = status;
    }
    return result;
}

bool WebDAVSynchronizer::download(const QString& filename)
{
    QTemporaryFile* file = new QTemporaryFile();
    auto result = false;
    if (file->open()) {
        QNetworkRequest request;
        auto url = QUrl(urlString() +
                        mkpath(remoteDirectory() + "/" + filename));
        url.setUserName(username());
        url.setPassword(password());
        request.setUrl(url);
        auto reply = m_networkAccessManager->get(request);
        file->setParent(reply);
        connect(reply, &QNetworkReply::readyRead, [=]() {
            file->write(reply->readAll());
        });
        waitForReplyToFinish(reply);
        file->write(reply->readAll());
        auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == HTTPStatusCode::OK) {
            QFile targetFile(directory() + "/" + filename);
            if (targetFile.open(QIODevice::WriteOnly)) {
                file->seek(0);
                while (!file->atEnd()) {
                    targetFile.write(file->read(1024*1024));
                }
                targetFile.close();
                result = true;
            } else {
                qCWarning(webDAVSynchronizer) << "Failed to open destination"
                                              << "file for writing:"
                                              << targetFile.errorString();
            }
        } else {
            qCWarning(webDAVSynchronizer) << "Upload failed with code" << code;
        }
    } else {
        qCWarning(webDAVSynchronizer) << "Failed to open temporary file for"
                                      << "downloading:"
                                      << file->errorString();
    }
    return result;
}


/**
 * @brief Upload a local file to the server.
 *
 * This method uploads the file speficied by the @p filename which is relative
 * to the set local directory name to the server (relative to the set remote
 * directory name). If @ etag points to a string, the new etag of the file
 * is stored in it.
 *
 * Note: WebDAV servers might not report etags when uploading files. In this
 * case if @p etag is not a null pointer, a separate PROPFIND request is done
 * to get the etag of the file just uploaded. However, this request might
 * retrieve wrong results (e.g. if another client uploaded the same file
 * in the meantime and hence overwrote it).
 */
bool WebDAVSynchronizer::upload(const QString& filename, QString* etag)
{
    auto result = false;
    auto file = new QFile(directory() + "/" + filename);
    QString currentEtag = "no-etag-retrieved-yet";
    if (file->open(QIODevice::ReadOnly)) {
        QNetworkRequest request;
        auto url = QUrl(urlString() +
                        mkpath(remoteDirectory() + "/" + filename));
        url.setUserName(username());
        url.setPassword(password());
        request.setUrl(url);
        request.setHeader(QNetworkRequest::ContentLengthHeader,
                          file->size());
        request.setHeader(QNetworkRequest::ContentTypeHeader,
                          "application/octet-stream");
        auto reply = m_networkAccessManager->put(request, file);
        file->setParent(reply);
        waitForReplyToFinish(reply);

        auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == HTTPStatusCode::OK || code == HTTPStatusCode::Created ||
                code == HTTPStatusCode::NoContent) {
            for (auto header : reply->rawHeaderPairs()) {
                if (header.first.toLower() == "etag") {
                    currentEtag = header.second;
                }
            }
            result = true;
        } else {
            qCWarning(webDAVSynchronizer) << "Upload failed with code" << code;
        }
    } else {
        qCWarning(webDAVSynchronizer) << "Failed to open" << filename
                                      << "for reading:" << file->errorString();
        delete file;
    }

    if (etag != nullptr) {
        if (currentEtag.isNull()) {
            qDebug() << "Server did not send etag on upload - "
                        "manually getting it.";
            currentEtag = this->etag(filename);
        }
        *etag = currentEtag;
    }
    return result;
}

bool WebDAVSynchronizer::mkdir(const QString& dirname, QString *etag)
{
    auto result = false;
    auto reply = createDirectoryRequest(this->remoteDirectory() + "/" + dirname);
    QString currentEtag = "no-etag-retrieved-yet";
    waitForReplyToFinish(reply);
    auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    result = code.toInt() == HTTPStatusCode::Created;
    for (auto header : reply->rawHeaderPairs()) {
        if (header.first.toLower() == "etag") {
            currentEtag = header.second;
        }
    }
    if (etag != nullptr) {
        *etag = currentEtag;
    }
    return result;
}

bool WebDAVSynchronizer::deleteEntry(const QString& filename)
{
    auto result = false;
    QNetworkRequest request;
    auto url = QUrl(urlString() +
                    mkpath(remoteDirectory() + "/" + filename));
    url.setUserName(username());
    url.setPassword(password());
    request.setUrl(url);
    auto reply = m_networkAccessManager->deleteResource(request);
    waitForReplyToFinish(reply);
    auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (code == HTTPStatusCode::OK || code == HTTPStatusCode::NoContent) {
        result = true;
    } else {
        qCWarning(webDAVSynchronizer) << "Deleting entry failed with code"
                                      << code;
    }
    return result;
}


/**
 * @brief Synchronize a single directory.
 *
 * This method synchronizes a local directory with a remote one. The
 * @p directory is relative to the local and remote one set in the
 * synchronizer. The synchronization works in both directions, i.e. local
 * changes are pushed to the server and remote changes are pulled down.
 * The procedure is implemented with a server-first approach, i.e. if there
 * are two conflicting changes the change from the server has precendence
 * over a local change. This method only syncs files which do not
 * start with a dot character (i.e. UNIX hidden files). Sub-directories are
 * synchronized non-recursively, i.e. only the directory itself is
 * synchronized but not its contents. The @p directoryFilter can be used to
 * limit synchronization to only those sub-directories matching the given
 * regular expression. By default, all directories are synchronized.
 *
 * The synchronization procedure works the following way:
 * A list of objects is created, which hold the local last modification
 * date (MD), the local last modification date from the previous
 * sync (PMD), the current etag (ETAG) and the etag from the previous
 * snyc run (PETAG). The previous (P...) values are stored in a local
 * datbase inside the local directory. This database itself is not
 * synchronized.
 *
 * MD might be null if the file does not exist locally.
 * PMD might be null if the file did not exist locally during the last
 * sync. ETAG might be null if the file does not exist on the server. PETAG
 * might be null if the file did not exist on the server during the last sync.
 *
 * The following rules are applied in order:
 *
 * - If ETAG is not null and ETAG is unequal to PETAG, then download the file.
 * - If ETAG is null and PETAG is not nnull, delete the local file.
 * - If MD is not null and PMD is null or MD is unqual to PMD,
 *   upload the file.
 * - If MD is null and PMD is not null, delete the remote file.
 *
 * If @p pushOnly is true, the method will not attempt to get the remote
 * list of directory contents. This can be used if a higher level method
 * detected that the directory is unchanged. In this case, only the local
 * files are checked and uploaded.
 */
bool WebDAVSynchronizer::syncDirectory(
        const QString& directory, QRegularExpression directoryFilter,
        bool pushOnly)
{
    bool result = false;
    auto localBase = this->directory();
    auto dir = mkpath(directory);
    QDir d(localBase + "/" + dir);
    QString dbConnName;
    if (!localBase.isEmpty() && d.exists()) {
        result = true;

        auto db = openSyncDb();
        dbConnName = db.databaseName();
        auto entries = findSyncDBEntries(db, dir);

        mergeLocalInfoWithSyncList(d, dir, entries);

        if (pushOnly) {
            for (auto key : entries.keys()) {
                auto &file = entries[key];
                file.etag = file.previousEtag;
            }
        } else {
            result = result && mergeRemoteInfoWithSyncList(entries, dir);
        }

        qCDebug(webDAVSynchronizer) << "Synchronizing" <<
                                       QDir::cleanPath(this->directory()
                                                       + "/" + directory);


        // First, pull entries:
        for (auto entry : entries) {
            if (!entry.etag.isNull() && entry.etag != entry.previousEtag) {
                if (entry.remoteType == Directory &&
                        !directoryFilter.match(entry.entry).hasMatch()) {
                    qCDebug(webDAVSynchronizer) << "Ignoring directory"
                                                << entry.path();
                    continue;
                }
                result = result && pullEntry(entry, db);
            } else if (entry.etag.isNull() && !entry.previousEtag.isNull()) {
                if (entry.remoteType == Directory &&
                        !directoryFilter.match(entry.entry).hasMatch()) {
                    qCDebug(webDAVSynchronizer) << "Ignoring directory"
                                                << entry.path();
                    continue;
                }
                result = result && removeLocalEntry(entry, db);
            } else if (!entry.lastModDate.isNull() &&
                       entry.lastModDate != entry.previousLasModtDate) {
                if (entry.localType == Directory &&
                        !directoryFilter.match(entry.entry).hasMatch()) {
                    qCDebug(webDAVSynchronizer) << "Ignoring directory"
                                                << entry.path();
                    continue;
                }
                result = result && pushEntry(entry, db);
            } else if (entry.lastModDate.isNull() &&
                       !entry.previousLasModtDate.isNull()) {
                if (entry.localType == Directory &&
                        !directoryFilter.match(entry.entry).hasMatch()) {
                    qCDebug(webDAVSynchronizer) << "Ignoring directory"
                                                << entry.path();
                    continue;
                }
                result = result && removeRemoteEntry(entry, db);
            }
        }
        findSyncDBEntries(db, dir);
        db.close();
    }
    if (!dbConnName.isNull()) {
        QSqlDatabase::removeDatabase(dbConnName);
    }
    return result;
}


/**
 * @brief Merge a sync entry list with local file data.
 */
void WebDAVSynchronizer::mergeLocalInfoWithSyncList(
        QDir &d, const QString& dir, SyncEntryMap& entries)
{
    for (auto entry : d.entryList(
             QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        QFileInfo fi(d.absoluteFilePath(entry));
        auto file = entries.value(entry, SyncEntry());
        file.entry = entry;
        file.parent = dir;
        file.localType = fi.isDir() ? Directory : File;
        file.lastModDate = fi.lastModified();
        entries[file.entry] = file;
    }
}


/**
 * @brief Merge a sync entry list with remote file data.
 */
bool WebDAVSynchronizer::mergeRemoteInfoWithSyncList(
        SyncEntryMap& entries, const QString& dir)
{
    bool ok;
    auto remoteEntries = entryList(dir, &ok);
    if (ok) {
        for (auto entry : remoteEntries) {
            if (entry.name != ".") {
                auto file = entries[entry.name];
                file.parent = dir;
                file.entry = entry.name;
                file.remoteType = entry.type == Directory ?
                            Directory : File;
                file.etag = entry.etag;
                entries[file.entry] = file;
            }
        }
    } else {
        qCWarning(webDAVSynchronizer) << "Failed to get entry list for"
                                      << dir;
    }
    return ok;
}


/**
 * @brief Pull an entry from the server.
 */
bool WebDAVSynchronizer::pullEntry(
        WebDAVSynchronizer::SyncEntry &entry, QSqlDatabase &db)
{
    qDebug(webDAVSynchronizer) << "Pulling" << entry.path();
    bool result = false;
    if (entry.remoteType == File) {
        // Pull a file
        if (entry.localType == Directory) {
            qCWarning(webDAVSynchronizer)
                    << "Pull conflict: Cannot pull file"
                    << entry.entry << "from" << entry.parent
                    << "because a local directory with that "
                       "name already exists";
        } else {
            if (download(entry.parent + "/" + entry.entry)) {
                QFileInfo fi(this->directory() + "/" + entry.parent
                             + "/" + entry.entry);
                entry.lastModDate = fi.lastModified();
                insertSyncDBEntry(db, entry);
                result = true;
            }
        }
    } else if (entry.remoteType == Directory) {
        // Pull a directory
        if (entry.localType == File) {
            qCWarning(webDAVSynchronizer)
                    << "Pull conflict: Cannot pull directory"
                    << entry.entry << "from" << entry.parent
                    << "because a file with that name already "
                       "exists locally";
        } else if (entry.localType == Invalid) {
            if (QDir(this->directory() + "/" + entry.parent).mkdir(
                        entry.entry)) {
                QFileInfo fi(this->directory() + "/" + entry.parent
                             + "/" + entry.entry);
                entry.lastModDate = fi.lastModified();
                insertSyncDBEntry(db, entry);
                result = true;
            }
        } else if (entry.localType == Directory) {
            QFileInfo fi(this->directory() + "/" + entry.parent
                         + "/" + entry.entry);
            entry.lastModDate = fi.lastModified();
            insertSyncDBEntry(db, entry);
            result = true;
        }
    } else {
        // Should not happen...
        qCWarning(webDAVSynchronizer) << "Cannot pull remote entry "
                                         "of type Unknown";
        result = false;
    }
    return result;
}


/**
 * @brief Remove a local file or directory.
 */
bool WebDAVSynchronizer::removeLocalEntry(
        WebDAVSynchronizer::SyncEntry &entry, QSqlDatabase &db)
{
    qDebug(webDAVSynchronizer) << "Removing" << entry.path() << "locally";
    bool result = false;
    if (entry.localType == File) {
        if (!QDir(directory() + "/" + entry.parent).remove(entry.entry)) {
            qCWarning(webDAVSynchronizer) << "Failed to remove local file"
                                          << entry.entry << "from"
                                          << entry.parent;
        } else {
            removeFileFromSyncDB(db, entry);
            result = true;
        }
    } else if (entry.localType == Directory) {
        if (rmLocalDir(directory() + "/" + entry.path(), 2)) {
            removeDirFromSyncDB(db, entry);
            result = true;
        } else {
            qCWarning(webDAVSynchronizer) << "Failed to remove local"
                                          << "directory" << entry.path();
        }
    } else {
        // Should not happen
        qCWarning(webDAVSynchronizer) << "Bad sync entry type of entry"
                                      << entry.entry << "in" << entry.parent
                                      << "in removeLocalEntry";
    }
    return result;
}


/**
 * @brief Push an entry to the server.
 */
bool WebDAVSynchronizer::pushEntry(
        WebDAVSynchronizer::SyncEntry &entry, QSqlDatabase &db)
{
    qDebug(webDAVSynchronizer) << "Pushing" << entry.path();
    bool result = false;
    if (entry.localType == Directory) {
        if (entry.remoteType == File) {
            qCWarning(webDAVSynchronizer)
                    << "Conflict: Cannot push directory" << entry.path()
                    << "as a file with that name exists on the remote";
        } else if (entry.remoteType == Directory) {
            insertSyncDBEntry(db, entry);
            result = true;
        } else if (entry.remoteType == Invalid) {
            if (mkdir(entry.path(), &entry.etag)) {
                insertSyncDBEntry(db, entry);
                result = true;
            }
        }
    } else if (entry.localType == File) {
        if (entry.remoteType == Directory) {
            qCWarning(webDAVSynchronizer)
                    << "Conflict: Cannot push local file" << entry.path()
                    << "because a directory with that name exists remotely";
        } else {
            if (upload(entry.path(), &entry.etag)) {
                insertSyncDBEntry(db, entry);
                result = true;
            }
        }
    } else if (entry.localType == Invalid) {
        qCWarning(webDAVSynchronizer) << "Unexpected local type of entry"
                                      << entry.path();
    }
    return result;
}


/**
 * @brief Remove an entry on the server.
 */
bool WebDAVSynchronizer::removeRemoteEntry(
        WebDAVSynchronizer::SyncEntry &entry, QSqlDatabase &db)
{
    qDebug(webDAVSynchronizer) << "Removing" << entry.path() << "remotely";
    bool result = false;
    if (deleteEntry(entry.path())) {
        if (entry.localType == Directory) {
            removeDirFromSyncDB(db, entry);
        } else {
            removeFileFromSyncDB(db, entry);
        }
        result = true;
    }
    return result;
}


/**
 * @brief Get the current etag of a file or directory.
 *
 * Get the etag of the file or directory identified by the @p filename.
 * The file name is relative to the remote directory set.
 * If the etag cannot be retrieved, an empty string is returned.
 */
QString WebDAVSynchronizer::etag(const QString& filename)
{
    QString result;
    auto path = m_remoteDirectory + "/" + filename;
    auto reply = etagRequest(path);
    waitForReplyToFinish(reply);
    auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (code.toInt() == HTTPStatusCode::WebDAVMultiStatus) {
        auto entryList = parseEntryList(path, reply->readAll());
        if (entryList.length() == 1) {
            auto entry = entryList.at(0);
            if (entry.name == ".") {
                result = entry.etag;
            }
        }
    }
    return result;
}


/**
 * @brief Create a proper path relative to the set directory.
 *
 * Create a path from the given @p path. The returned path will be relative
 * to the currently set local/remote path.
 */
QString WebDAVSynchronizer::mkpath(const QString &path)
{
    auto result = QDir::cleanPath(path);
    if (result.startsWith("/")) {
        result = result.mid(1);
    }
    return result;
}


/**
 * @brief Split a path.
 *
 * This helper method splits up a path into a parent path and the trailing
 * file (or directory) name part and return it as a tuple.
 */
std::tuple<QString, QString> WebDAVSynchronizer::splitpath(const QString &path)
{
    std::tuple<QString, QString> result;
    auto p = mkpath(path);
    auto list = p.split("/");
    if (list.length() == 1) {
        result = std::make_tuple(QString(""), list[0]);
    } else if (list.length() > 1) {
        auto last = list.takeLast();
        result = std::make_tuple(list.join("/"), last);
    }
    return result;
}


/**
 * @brief Get the URL as a string with a trailing slash.
 */
QString WebDAVSynchronizer::urlString() const
{
    auto result = baseUrl().toString();
    if (!result.endsWith("/")) {
        result += "/";
    }
    return result;
}




QNetworkReply* WebDAVSynchronizer::listDirectoryRequest(
        const QString& directory)
{
    /*
     curl -i -X PROPFIND http://admin:admin@localhost:8080/remote.php/webdav/ \
        --upload-file - -H "Depth: 1" <<END
     <?xml version="1.0"?>
     <a:propfind xmlns:a="DAV:">
     <a:prop><a:resourcetype/></a:prop>
     </a:propfind>
     END
    */

    QByteArray requestData = "<?xml version=\"1.0\"?>"
                             "<a:propfind xmlns:a=\"DAV:\">"
                             "<a:prop>"
                             "<a:resourcetype/>"
                             "<a:getetag/>"
                             "</a:prop>"
                             "</a:propfind>";
    QNetworkRequest request;
    auto url = QUrl(urlString() + mkpath(directory));
    url.setUserName(m_username);
    url.setPassword(m_password);
    request.setUrl(url);
    request.setRawHeader("Depth", "1");
    request.setHeader(QNetworkRequest::ContentLengthHeader,
                      requestData.size());
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "text/xml; charset=utf-8");
    auto buffer = new QBuffer;
    buffer->setData(requestData);
    auto reply = m_networkAccessManager->sendCustomRequest(
                request, "PROPFIND", buffer);
    buffer->setParent(reply);
    prepareReply(reply);
    return reply;
}

QNetworkReply* WebDAVSynchronizer::etagRequest(const QString& filename)
{
    /*
     curl -i -X PROPFIND http://admin:admin@localhost:8080/remote.php/webdav/ \
        --upload-file - -H "Depth: 0" <<END
     <?xml version="1.0"?>
     <a:propfind xmlns:a="DAV:">
     <a:prop><a:getetag/></a:prop>
     </a:propfind>
     END
    */

    QByteArray requestData = "<?xml version=\"1.0\"?>"
                             "<a:propfind xmlns:a=\"DAV:\">"
                             "<a:prop>"
                             "<a:getetag/>"
                             "</a:prop>"
                             "</a:propfind>";
    QNetworkRequest request;
    auto url = QUrl(urlString() + mkpath(filename));
    url.setUserName(m_username);
    url.setPassword(m_password);
    request.setUrl(url);
    request.setRawHeader("Depth", "0");
    request.setHeader(QNetworkRequest::ContentLengthHeader,
                      requestData.size());
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "text/xml; charset=utf-8");
    auto buffer = new QBuffer;
    buffer->setData(requestData);
    auto reply = m_networkAccessManager->sendCustomRequest(
                request, "PROPFIND", buffer);
    buffer->setParent(reply);
    prepareReply(reply);
    return reply;
}

QNetworkReply*WebDAVSynchronizer::createDirectoryRequest(
        const QString& directory)
{
    /*
     curl -X MKCOL 'http://admin:admin@localhost:8080/remote.php/webdav/example'
    */
    QNetworkRequest request;
    auto url = QUrl(urlString() + mkpath(directory));
    url.setUserName(m_username);
    url.setPassword(m_password);
    request.setUrl(url);
    auto reply = m_networkAccessManager->sendCustomRequest(
                request, "MKCOL");
    prepareReply(reply);
    return reply;
}

WebDAVSynchronizer::EntryList WebDAVSynchronizer::parseEntryList(
        const QString& directory, const QByteArray& reply)
{
    EntryList result;
    QDomDocument doc;
    QString errorMsg;
    int errorLine;
    if (doc.setContent(reply, &errorMsg, &errorLine)) {
        result = parsePropFindResponse(doc, directory);
    } else {
        qCWarning(webDAVSynchronizer) << "Failed to parse WebDAV response:"
                                      << errorMsg << "in line" << errorLine;
    }
    return result;
}

WebDAVSynchronizer::EntryList WebDAVSynchronizer::parsePropFindResponse(
        const QDomDocument& response, const QString& directory)
{
    EntryList result;
    auto baseUrl = this->baseUrl();
    auto baseDir = QDir::cleanPath(baseUrl.path() + "/" + directory);
    auto root = response.documentElement();
    auto rootTagName = root.tagName();
    if (rootTagName == "d:multistatus") {
        auto resp = root.firstChildElement("d:response");
        while (resp.isElement()) {
            auto entry = parseResponseEntry(resp, baseDir);
            if (entry.type != Invalid) {
                result << entry;
            }
            resp = resp.nextSiblingElement("d:response");
        }
    } else {
        qCWarning(webDAVSynchronizer) << "Received invalid WebDAV response from"
                                         "server starting with element"
                                      << rootTagName;
    }
    return result;
}

WebDAVSynchronizer::Entry WebDAVSynchronizer::parseResponseEntry(
        const QDomElement& element, const QString& baseDir)
{
    auto type = File;
    QString etag;

    auto propstats = element.elementsByTagName("d:propstat");
    for (int i = 0; i < propstats.length(); ++i) {
        auto propstat = propstats.at(i).toElement();
        auto status = propstat.firstChildElement("d:status");
        if (status.text().endsWith("200 OK")) {
            auto prop = propstat.firstChildElement("d:prop");
            auto child = prop.firstChildElement();
            while (child.isElement()) {
                if (child.tagName() == "d:resourcetype") {
                    if (child.firstChildElement().tagName() == "d:collection") {
                        type = Directory;
                    }
                } else if (child.tagName() == "d:getetag") {
                    etag = child.text();
                } else {
                    qCWarning(webDAVSynchronizer) << "Unknown DAV Property:"
                                                  << child.tagName();
                }
                child = child.nextSiblingElement();
            }
        } else {
            qCWarning(webDAVSynchronizer) << "Properties not retrieved -"
                                          << status.text();
        }
    }


    QString path = QByteArray::fromPercentEncoding(
                element.firstChildElement("d:href").text().toUtf8());

    path = QDir(baseDir).relativeFilePath(path);
    Entry result;
    result.type = type;
    result.etag = etag;
    result.name = path;
    return result;
}

void WebDAVSynchronizer::prepareReply(QNetworkReply* reply) const
{
    if (m_disableCertificateCheck) {
        connect(reply, &QNetworkReply::sslErrors,
                [=](QList<QSslError> errors) {
            for (auto error : errors) {
                qCWarning(webDAVSynchronizer) << error.errorString();
            }
            reply->ignoreSslErrors(errors);
        });
    }
    connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            [=](QNetworkReply::NetworkError error) {
        qCWarning(webDAVSynchronizer) << "Network error:" << error;
    });
}


/**
 * @brief Waits for the @p reply to finish.
 *
 * This is a helper method which waits for the network reply to finish, i.e.
 * either the reply's finished() signal is emitted or there was no up- or
 * download progress within a given interval.
 */
void WebDAVSynchronizer::waitForReplyToFinish(QNetworkReply* reply) const
{
    Q_CHECK_PTR(reply);
    QEventLoop loop;
    QTimer timer;
    timer.setInterval(30000);
    auto restartTimer = [&](qint64, qint64) { timer.start(); };
    auto c1 = connect(reply, &QNetworkReply::uploadProgress, restartTimer);
    auto c2 = connect(reply, &QNetworkReply::uploadProgress, restartTimer);
    auto c3 = connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    auto c4 = connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();
    disconnect(c1);
    disconnect(c2);
    disconnect(c3);
    disconnect(c4);
}


/**
 * @brief Open the SyncDB database.
 */
QSqlDatabase WebDAVSynchronizer::openSyncDb() {
    const QString& localDir = this->directory();
    auto dbPath = QDir::cleanPath(localDir + "/.otlwebdavsync.db");
    auto db = QSqlDatabase::addDatabase("QSQLITE", dbPath);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qWarning(webDAVSynchronizer) << "Failed to open database:"
                                     << db.lastError().text();
        return db;
    }
    QSqlQuery query(db);
    query.prepare("CREATE TABLE IF NOT EXISTS "
                  "version (key string PRIMARY KEY, value);");
    if (!query.exec()) {
        qWarning(webDAVSynchronizer) << "Failed to create version table:"
                                     << query.lastError().text();
    }
    query.prepare("SELECT value FROM version WHERE key == 'version';");
    int version = 0;
    if (query.exec()) {
        if (query.first()) {
            auto record = query.record();
            version = record.value("value").toInt();
        }
    } else {
        qCWarning(webDAVSynchronizer) << "Failed to get version of sync DB:"
                                      << query.lastError().text();
    }
    if (version == 0) {
        query.prepare("CREATE TABLE files ("
                      "`parent` string, "
                      "`entry` string NOT NULL, "
                      "`modificationDate` date not null, "
                      "`etag` string not null, "
                      "PRIMARY KEY(`parent`, `entry`)"
                      ");");
        if (!query.exec()) {
            qWarning(webDAVSynchronizer) << "Failed to create files table:"
                                         << query.lastError().text();
        }
        query.prepare("INSERT OR REPLACE INTO version(key, value) "
                      "VALUES ('version', 1);");
        if (!query.exec()) {
            qWarning(webDAVSynchronizer) << "Failed to insert version into DB:"
                                         << query.lastError().text();
        }
    }
    return db;
}


/**
 * @brief Insert a single entry into the SyncDB.
 *
 * This inserts the entry into the SyncDB. The current modification date and
 * etag will be stored in the DB.
 */
void WebDAVSynchronizer::insertSyncDBEntry(
        QSqlDatabase &db, const WebDAVSynchronizer::SyncEntry &entry) {
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO files "
                  "(parent, entry, modificationDate, etag) "
                  "VALUES (?, ?, ?, ?);");
    query.addBindValue(entry.parent);
    query.addBindValue(entry.entry);
    query.addBindValue(entry.lastModDate);
    query.addBindValue(entry.etag);
    if (!query.exec()) {
        qCWarning(webDAVSynchronizer) << "Failed to insert SyncDB entry:"
                                      << query.lastError().text();
    }
}


/**
 * @brief Get entries for given directory.
 *
 * This retrieves a list of entries from the last sync run
 * for the given parent directory. The returned entries
 * will have their previousLastModDate and previousEtag entries
 * set.
 */
WebDAVSynchronizer::SyncEntryMap WebDAVSynchronizer::findSyncDBEntries(
        QSqlDatabase &db, const QString &parent) {
    QMap<QString, SyncEntry> result;
    QSqlQuery query(db);
    query.prepare("SELECT parent, entry, modificationDate, etag "
                  "FROM files WHERE parent = ?;");
    query.addBindValue(parent);
    if (query.exec()) {
        while (query.next()) {
            SyncEntry entry;
            auto record = query.record();
            entry.parent = record.value("parent").toString();
            entry.entry = record.value("entry").toString();
            entry.previousLasModtDate = record.value("modificationDate")
                    .toDateTime();
            entry.previousEtag = record.value("etag").toString();
            result[entry.entry] = entry;
        }
    } else {
        qCWarning(webDAVSynchronizer) << "Failed to get sync entries from DB:"
                                      << query.lastError().text();
    }
    return result;
}


/**
 * @brief Remove a directory from the SyncDB.
 */
void WebDAVSynchronizer::removeDirFromSyncDB(
        QSqlDatabase &db, const SyncEntry& entry) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM files "
                  "WHERE parent LIKE '%' || ? OR (parent = ? AND entry = ?);");
    query.addBindValue(entry.path());
    query.addBindValue(entry.parent);
    query.addBindValue(entry.entry);
    if (!query.exec()) {
        qCWarning(webDAVSynchronizer) << "Failed to delete directory from "
                                         "sync DB:"
                                      << query.lastError().text();
    }
}


/**
 * @brief Remove a single entry from the SyncDB.
 */
void WebDAVSynchronizer::removeFileFromSyncDB(
        QSqlDatabase &db, const WebDAVSynchronizer::SyncEntry &entry) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM files WHERE parent = ? AND entry = ?;");
    query.addBindValue(entry.parent);
    query.addBindValue(entry.entry);
    if (!query.exec()) {
        qCWarning(webDAVSynchronizer) << "Failed to remove entry from sync DB:"
                                      << query.lastError().text();
    }
}


/**
 * @brief Remove a directory.
 *
 * This removes the directory @p dir. All files and directories in that
 * directory are removed first, before the actual directory is removed.
 * If @p maxDepth greater than zero, rmLocalDir() is called for all
 * sub-directories with a maxDepth - 1. Hence, this method can be used to
 * reduce the total depth up to which the deletion is executed.
 */
bool WebDAVSynchronizer::rmLocalDir(const QString &dir, int maxDepth) {
    if (!dir.isEmpty()) {
        QDir d(dir);
        if (d.exists()) {
            for (auto entry : d.entryList(QDir::Dirs | QDir::Files |
                                          QDir::NoDotAndDotDot)) {
                QFileInfo fi(d.absoluteFilePath(entry));
                if (fi.isDir() && maxDepth > 0) {
                    rmLocalDir(fi.absoluteFilePath(), maxDepth - 1);
                }
                if (fi.isDir()) {
                    d.rmdir(entry);
                } else {
                    d.remove(entry);
                }
            }
            auto dirName = d.dirName();
            if (d.cdUp()) {
                return d.rmdir(dirName);
            }
        }
    }
    return false;
}
