#include "sync/synchronizer.h"
#include "sync/webdavsynchronizer.h"
#include "utils/jsonutils.h"

#include <QObject>
#include <QTest>
#include <QTemporaryDir>

class SynchronizerTest : public QObject
{
  Q_OBJECT

private slots:

  void initTestCase() {}
  void init() {}
  void fromDirectory();
  void logging();
  void cleanup() {}
  void cleanupTestCase() {}
};




void SynchronizerTest::fromDirectory()
{
    for (auto serverType : {WebDAVSynchronizer::NextCloud,
            WebDAVSynchronizer::OwnCloud, WebDAVSynchronizer::Generic}) {
        QTemporaryDir dir;
        {
            WebDAVSynchronizer sync;
            sync.setServerType(serverType);
            sync.setDirectory(dir.path());
            QVERIFY(sync.save());
        }
        {
            auto sync = Synchronizer::fromDirectory(dir.path());
            Q_CHECK_PTR(sync);
            auto s = dynamic_cast<WebDAVSynchronizer*>(sync);
            QVERIFY(s != nullptr);
            QCOMPARE(s->serverType(), serverType);
            delete sync;
        }
    }
    {
        QTemporaryDir dir;
        QVariantMap map;
        map["type"] = "foo";
        JsonUtils::patchJsonFile(dir.path() + "/" + Synchronizer::SaveFileName,
                                 map);
        auto sync = Synchronizer::fromDirectory(dir.path());
        QVERIFY(sync == nullptr);
    }
}

void SynchronizerTest::logging()
{
    QTemporaryDir dir;
    {
        WebDAVSynchronizer sync;
        QCOMPARE(sync.log().length(), 0);
        sync.setDirectory(dir.path());
        sync.debug() << "I am a debug message";
        sync.warning() << "I am a warning message";
        sync.error() << "I am an error message";
        QCOMPARE(sync.log().length(), 3);
        auto log = sync.log();
        QVERIFY(log[0].time <= log[1].time);
        QVERIFY(log[1].time <= log[2].time);
        QCOMPARE(log[0].message, QString("I am a debug message "));
        QCOMPARE(log[1].message, QString("I am a warning message "));
        QCOMPARE(log[2].message, QString("I am an error message "));
        sync.saveLog();
    }
    {
        WebDAVSynchronizer sync;
        QCOMPARE(sync.log().length(), 0);
        sync.setDirectory(dir.path());
        sync.loadLog();
        QCOMPARE(sync.log().length(), 3);
        auto log = sync.log();
        QVERIFY(log[0].time <= log[1].time);
        QVERIFY(log[1].time <= log[2].time);
        QCOMPARE(log[0].message, QString("I am a debug message "));
        QCOMPARE(log[1].message, QString("I am a warning message "));
        QCOMPARE(log[2].message, QString("I am an error message "));
    }
    {
        WebDAVSynchronizer sync;
        for (int i = 0; i < Synchronizer::MaxLogEntries; ++i) {
            sync.debug() << "Foo";
        }
        QCOMPARE(sync.log().length(), Synchronizer::MaxLogEntries);
        sync.debug() << "Bar";
        QCOMPARE(sync.log().length(), Synchronizer::MaxLogEntries);
        QCOMPARE(sync.log()[0].message, QString("Foo "));
        QCOMPARE(sync.log()[Synchronizer::MaxLogEntries-1].message,
                QString("Bar "));
    }
}

QTEST_MAIN(SynchronizerTest)
#include "test_synchronizer.moc"
