#ifndef APPLICATION_H
#define APPLICATION_H

#include "library.h"

#include <QLoggingCategory>
#include <QObject>
#include <QQmlListProperty>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>
#include <QVector>


class Migrator_2_x_to_3_x;
class KeyStore;

/**
 * @brief The main class of the application
 *
 * The Application class is used as entry point into the OpenTodoList application. It is used
 * as contained class and provides references to other objects. Basically, the Application class
 * models the application, i.e. it is created when the application starts and destroyed once
 * the application is to be closed.
 */
class Application : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Library> libraries READ libraryList NOTIFY librariesChanged)
    Q_PROPERTY(Library* defaultLibrary READ defaultLibrary NOTIFY defaultLibraryChanged)
    Q_PROPERTY(QString librariesLocation READ librariesLocation CONSTANT)

    friend class Migrator_2_x_to_3_x;
public:

    explicit Application(QObject *parent = 0);
    explicit Application(QSettings *settings, QObject *parent = 0);

    virtual ~Application();

    /**
   * @brief The list of all Library objects present in the application.
   */
    QList<Library*> libraries() const               { return m_libraries; }

    QStringList libraryTypes() const;
    QQmlListProperty<Library> libraryList();

    Q_INVOKABLE Library* addLibrary(const QVariantMap& parameters);

    Q_INVOKABLE void saveValue(const QString &name, const QVariant &value);
    Q_INVOKABLE QVariant loadValue(const QString &name, const QVariant &defaultValue = QVariant());

    Q_INVOKABLE QString readFile(const QString &fileName) const;
    Q_INVOKABLE QVariant find3rdPartyInfos() const;

    Q_INVOKABLE QString urlToLocalFile(const QUrl &url) const;
    Q_INVOKABLE QUrl localFileToUrl(const QString &localFile) const;
    Q_INVOKABLE QUrl cleanPath(const QUrl &url) const;

    Q_INVOKABLE bool fileExists(const QString &filename) const;
    Q_INVOKABLE bool directoryExists(const QString &directory) const;
    Q_INVOKABLE QString basename(const QString &filename) const;
    Q_INVOKABLE bool isLibraryDir(const QUrl &url) const;

    Library *defaultLibrary();

    QString librariesLocation() const;
    static QString defaultLibrariesLocation();

    Q_INVOKABLE QUrl homeLocation() const;
    Q_INVOKABLE bool folderExists(const QUrl &url) const;

    Q_INVOKABLE QString secretForSynchronizer(Synchronizer* sync);

public slots:

    void syncLibrary(Library *library);
    void saveSynchronizerSecrets(Synchronizer *sync);
    void copyToClipboard(const QString &text);

signals:

    void librariesChanged();
    void defaultLibraryChanged();

private:

    QList<Library*>         m_libraries;
    Library                *m_defaultLibrary;
    QSettings              *m_settings;
    bool                    m_loadingLibraries;
    KeyStore               *m_keyStore;
    QVariantMap             m_secrets;

    void saveLibraries();
    void loadLibraries();

    static Library* librariesAt(QQmlListProperty<Library> *property, int index);
    static int librariesCount(QQmlListProperty<Library> *property);

    QString defaultLibraryLocation() const;
    void runMigrations();

    void appendLibrary(Library* library);


    void initialize();

private slots:

    void onLibraryDeleted(Library *library);
    void onLibrarySyncFinished(Library *library);

};

Q_DECLARE_LOGGING_CATEGORY(application)

#endif // APPLICATION_H
