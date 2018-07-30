#include <memory>
#include <QFile>
#include <QFileSystemWatcher>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include "core/localshapesfinder.h"

class LocalShapesFinderTest : public QObject
{
    Q_OBJECT

public:
    LocalShapesFinderTest();

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    void createFiles(unsigned int startIndex, unsigned int count, QString ext = "psj", QByteArray creator = "2DPlugin");
    void createFilesInPath(QString path, unsigned int startIndex, unsigned int count, QString ext = "psj", QByteArray creator = "2DPlugin");

private Q_SLOTS:
    void init();
    void cleanup();

    void testQFileSysteWatcherUsage();
    void testCreatePsjFiles();
    void loadFilesFromDirectory();
    void onlyLoadFilesWithPsjExtension();
    void skipInvalidPsjFiles();
    void skipPsjFilesWithoutACorrespondingGCodeFile();
    void skipPsjFilesWithoutACorrespondingSvgFile();
    void emitSignalWhenShapesAreFirstFound();
    void onlyLoadNewFilesWhenDirectoryChanges();
    void emitSignalWhenNewShapesAreFound();
    void removeShapesThatNoLongerExist();
    void emitSignalWhenShapesAreRemoved();
    void removeAllShapesIfDirectoryIsRemoved();
    void doNotEmitSignalIfDirectoryRemovedAndThereIsNoShape();
    void loadFilesFromDirAtAstart();
    void doNotEmitSignalsIfThereIsNoChange();
    void createDirectoryIfNotExistingAtStart();
};

LocalShapesFinderTest::LocalShapesFinderTest()
{
}

void LocalShapesFinderTest::createFiles(unsigned int startIndex, unsigned int count, QString ext, QByteArray creator)
{
    createFilesInPath(m_dir->path(), startIndex, count, ext, creator);
}

void LocalShapesFinderTest::createFilesInPath(QString path, unsigned int startIndex, unsigned int count, QString ext, QByteArray creator)
{
    for (auto i = startIndex; i < (startIndex + count); ++i) {
        QByteArray gcodeFilename = (QString("tmpTest-%1.gcode").arg(i)).toLatin1();
        QByteArray svgFilename = (QString("tmpTest-%1.svg").arg(i)).toLatin1();
        QByteArray content = R"(
{
  "version": 1,
  "svgFilename": ")" + svgFilename + R"(",
  "name": "sandman",
  "square": true,
  "machineType": "PolyShaperOranje",
  "drawToolpath": true,
  "margin": 10.0,
  "generatedBy": ")" + creator + R"(",
  "creationTime": "2018-07-26T22:56:56.931242",
  "flatness": 0.001,
  "workpieceDimX": 400.0,
  "workpieceDimY": 450.0,
  "autoClosePath": true,
  "duration": 81,
  "pointsInsideWorkpiece": true,
  "speed": 1000.0,
  "gcodeFilename": ")" + gcodeFilename + R"("
})";
        QFile file(path + QString("/tmpTest-%1.%2").arg(i).arg(ext));
        if (!file.open(QIODevice::WriteOnly)) {
            throw QString("CANNOT CREATE psj TEMPORARY FILE!!!");
        }
        file.write(content);

        // Also create gcode and svg files
        QFile gcodeFile(path + "/" + gcodeFilename);
        if (!gcodeFile.open(QIODevice::WriteOnly)) {
            throw QString("CANNOT CREATE gcode TEMPORARY FILE!!!");
        }

        QFile svgFile(path + "/" + svgFilename);
        if (!svgFile.open(QIODevice::WriteOnly)) {
            throw QString("CANNOT CREATE svg TEMPORARY FILE!!!");
        }
    }
}

void LocalShapesFinderTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
}

void LocalShapesFinderTest::cleanup()
{
    m_dir.reset();
}

void LocalShapesFinderTest::testQFileSysteWatcherUsage()
{
    // A test to check how FileSystemWatcher works
    QFileSystemWatcher watcher;

    QSignalSpy spy(&watcher, &QFileSystemWatcher::directoryChanged);

    watcher.addPath(m_dir->path());

    // No signal at start
    QVERIFY(!spy.wait(500));

    // Create a file
    QFile file(m_dir->path() + "/testFile");
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(spy.wait(500));
    QCOMPARE(spy.at(0).at(0).toString(), m_dir->path());

    // Remove a file
    QVERIFY(file.remove());
    QVERIFY(spy.wait(500));
    QCOMPARE(spy.at(1).at(0).toString(), m_dir->path());

    // Create a subdirectory
    QString testDir = "testDir";
    QDir curDir(m_dir->path());
    QVERIFY(curDir.mkdir(testDir));
    QVERIFY(spy.wait(500));
    QCOMPARE(spy.at(2).at(0).toString(), m_dir->path());

    // Remove a subdirectory
    QVERIFY(curDir.rmdir(testDir));
    QVERIFY(spy.wait(500));
    QCOMPARE(spy.at(3).at(0).toString(), m_dir->path());

    // Remove watched directory
    QVERIFY(curDir.rmdir(m_dir->path()));
    QVERIFY(spy.wait(500));
    QCOMPARE(spy.at(4).at(0).toString(), m_dir->path());

    // Recreate watched directory, no signal
    QThread::sleep(1);// Needed to have test pass on windows (mkdir might fail otherwise)
    QVERIFY(curDir.mkdir(m_dir->path()));
    QVERIFY(!spy.wait(500));
}

void LocalShapesFinderTest::testCreatePsjFiles()
{
    createFiles(5, 3, "dummy");

    // Just test creation, content is tested implicitly in other tests
    QVERIFY(QFile::exists(m_dir->path() + "/tmpTest-5.dummy"));
    QVERIFY(QFile::exists(m_dir->path() + "/tmpTest-6.dummy"));
    QVERIFY(QFile::exists(m_dir->path() + "/tmpTest-7.dummy"));
    QVERIFY(QFile::exists(m_dir->path() + "/tmpTest-5.gcode"));
    QVERIFY(QFile::exists(m_dir->path() + "/tmpTest-6.gcode"));
    QVERIFY(QFile::exists(m_dir->path() + "/tmpTest-7.gcode"));
    QVERIFY(QFile::exists(m_dir->path() + "/tmpTest-5.svg"));
    QVERIFY(QFile::exists(m_dir->path() + "/tmpTest-6.svg"));
    QVERIFY(QFile::exists(m_dir->path() + "/tmpTest-7.svg"));
}

void LocalShapesFinderTest::loadFilesFromDirectory()
{
    LocalShapesFinder finder(m_dir->path());

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 3);
    QCOMPARE(finder.shapes()[m_dir->path() + "/tmpTest-0.psj"].svgFilename(), "tmpTest-0.svg");
    QCOMPARE(finder.shapes()[m_dir->path() + "/tmpTest-0.psj"].gcodeFilename(), "tmpTest-0.gcode");
    QCOMPARE(finder.shapes()[m_dir->path() + "/tmpTest-1.psj"].svgFilename(), "tmpTest-1.svg");
    QCOMPARE(finder.shapes()[m_dir->path() + "/tmpTest-1.psj"].gcodeFilename(), "tmpTest-1.gcode");
    QCOMPARE(finder.shapes()[m_dir->path() + "/tmpTest-2.psj"].svgFilename(), "tmpTest-2.svg");
    QCOMPARE(finder.shapes()[m_dir->path() + "/tmpTest-2.psj"].gcodeFilename(), "tmpTest-2.gcode");
}

void LocalShapesFinderTest::onlyLoadFilesWithPsjExtension()
{
    LocalShapesFinder finder(m_dir->path());

    createFiles(0, 3);
    createFiles(6, 2, "dummy"); // These are not loaded

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 3);
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-1.psj"));
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::skipInvalidPsjFiles()
{
    LocalShapesFinder finder(m_dir->path());

    createFiles(0, 3);

    // Create another invalid (empty) psj file
    QFile gcodeFile(m_dir->path() + QString("/tmpTest-100.psj"));
    QVERIFY(gcodeFile.open(QIODevice::WriteOnly));

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 3);
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-1.psj"));
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::skipPsjFilesWithoutACorrespondingGCodeFile()
{
    LocalShapesFinder finder(m_dir->path());

    createFiles(0, 3);

    // Remove a GCode file
    QVERIFY(QFile::remove(m_dir->path() + QString("/tmpTest-1.gcode")));

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 2);
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::skipPsjFilesWithoutACorrespondingSvgFile()
{
    LocalShapesFinder finder(m_dir->path());

    createFiles(0, 3);

    // Remove a GCode file
    QVERIFY(QFile::remove(m_dir->path() + QString("/tmpTest-1.svg")));

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 2);
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::emitSignalWhenShapesAreFirstFound()
{
    LocalShapesFinder finder(m_dir->path());

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QSet<QString> expectedNewShapes{m_dir->path() + "/tmpTest-0.psj", m_dir->path() + "/tmpTest-1.psj", m_dir->path() + "/tmpTest-2.psj"};
    // Collect all new shapes (on windows we might receive multiple signals)
    QSet<QString> newShapes;
    for (auto s: spy) {
        newShapes.unite(s.at(0).value<QSet<QString>>());
        QVERIFY(s.at(1).value<QSet<QString>>().isEmpty());
    }
    QCOMPARE(newShapes, expectedNewShapes);
}

void LocalShapesFinderTest::onlyLoadNewFilesWhenDirectoryChanges()
{
    LocalShapesFinder finder(m_dir->path());

    createFiles(0, 3, "psj", "first");

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 3);

    // New files and rescan
    createFiles(1, 3, "psj", "another"); // 1 and 2 not reloaded, 3 added

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 4);

    QCOMPARE(finder.shapes()[m_dir->path() + "/tmpTest-1.psj"].generatedBy(), "first");
    QCOMPARE(finder.shapes()[m_dir->path() + "/tmpTest-2.psj"].generatedBy(), "first");
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-3.psj"));
    QCOMPARE(finder.shapes()[m_dir->path() + "/tmpTest-3.psj"].generatedBy(), "another");
}

void LocalShapesFinderTest::emitSignalWhenNewShapesAreFound()
{
    LocalShapesFinder finder(m_dir->path());

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QVERIFY(spy.count() > 0);
    const auto oldSignals = spy.count();

    // New files added
    createFiles(50, 2);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QSet<QString> expectedNewShapes{m_dir->path() + "/tmpTest-50.psj", m_dir->path() + "/tmpTest-51.psj"};
    // Collect all new shapes (on windows we might receive multiple signals)
    QSet<QString> newShapes;
    for (auto i = oldSignals; i < spy.count(); ++i) {
        newShapes.unite(spy.at(i).at(0).value<QSet<QString>>());
        QVERIFY(spy.at(i).at(1).value<QSet<QString>>().isEmpty());
    }
    QCOMPARE(newShapes, expectedNewShapes);
}

void LocalShapesFinderTest::removeShapesThatNoLongerExist()
{
    LocalShapesFinder finder(m_dir->path());

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 3);

    // Remove files and rescan
    QVERIFY(QFile::remove(m_dir->path() + "/tmpTest-1.psj"));

    // This is needed to process events from QFileWatcher
    QThread::sleep(1); // This is needed on windows
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 2);
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::emitSignalWhenShapesAreRemoved()
{
    LocalShapesFinder finder(m_dir->path());

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QVERIFY(spy.count() > 0);
    const auto oldSignals = spy.count();

    // Remove files and rescan
    QVERIFY(QFile::remove(m_dir->path() + "/tmpTest-1.psj"));
    QVERIFY(QFile::remove(m_dir->path() + "/tmpTest-2.psj"));

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QSet<QString> expectedRemovedShapes{m_dir->path() + "/tmpTest-1.psj", m_dir->path() + "/tmpTest-2.psj"};
    // Collect all removed shapes (on windows we might receive multiple signals)
    QSet<QString> removedShapes;
    for (auto i = oldSignals; i < spy.count(); ++i) {
        QVERIFY(spy.at(i).at(0).value<QSet<QString>>().isEmpty());
        removedShapes.unite(spy.at(i).at(1).value<QSet<QString>>());
    }
    QCOMPARE(removedShapes, expectedRemovedShapes);
}

void LocalShapesFinderTest::removeAllShapesIfDirectoryIsRemoved()
{
    LocalShapesFinder finder(m_dir->path());

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QVERIFY(spy.count() > 0);
    const auto oldSignals = spy.count();

    // Remove directory (save path to use it later)
    auto path = m_dir->path();
    m_dir.reset();

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QSet<QString> expectedRemovedShapes{path + "/tmpTest-0.psj", path + "/tmpTest-1.psj", path + "/tmpTest-2.psj"};
    // Collect all removed shapes (on windows we might receive multiple signals)
    QSet<QString> removedShapes;
    for (auto i = oldSignals; i < spy.count(); ++i) {
        QVERIFY(spy.at(i).at(0).value<QSet<QString>>().isEmpty());
        removedShapes.unite(spy.at(i).at(1).value<QSet<QString>>());
    }
    QCOMPARE(removedShapes, expectedRemovedShapes);

    QCOMPARE(finder.shapes().size(), 0);
}

void LocalShapesFinderTest::doNotEmitSignalIfDirectoryRemovedAndThereIsNoShape()
{
    LocalShapesFinder finder(m_dir->path());

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    // Discard initial signals we might receive (especially on Windows)
    QThread::sleep(1);
    QCoreApplication::processEvents();

    m_dir.reset();

    // No signal emitted
    QVERIFY(!spy.wait(500));
}

void LocalShapesFinderTest::loadFilesFromDirAtAstart()
{
    createFiles(0, 3);

    LocalShapesFinder finder(m_dir->path());

    QCOMPARE(finder.shapes().size(), 3);
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-1.psj"));
    QVERIFY(finder.shapes().contains(m_dir->path() + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::doNotEmitSignalsIfThereIsNoChange()
{
    // Here we add and remove a fiel, we should receive no signal (on Windows a
    // notification might arrive, we must discard it)

    LocalShapesFinder finder(m_dir->path());

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    // Create and remove a file
    createFiles(5, 1);
    QFile::remove(m_dir->path() + "/tmpTest-5.psj");

    // There must be no notification
    QVERIFY(!spy.wait(500));
}

void LocalShapesFinderTest::createDirectoryIfNotExistingAtStart()
{
    // Save path and remove dir
    const auto path = m_dir->path();
    m_dir.reset();

    LocalShapesFinder finder(path);

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    // Path is created
    QDir dir(path);
    QVERIFY(dir.exists());

    // Discarding initial notifications
    spy.wait(500);
    const auto oldSignals = spy.count();

    // Also check we get signals on directory change
    createFilesInPath(path, 0, 1);

    QVERIFY(spy.wait(500));

    QSet<QString> expectedNewShapes{path + "/tmpTest-0.psj"};
    // Collect all new shapes (on windows we might receive multiple signals)
    QSet<QString> newShapes;
    for (auto i = oldSignals; i < spy.count(); ++i) {
        newShapes.unite(spy.at(i).at(0).value<QSet<QString>>());
        QVERIFY(spy.at(i).at(1).value<QSet<QString>>().isEmpty());
    }
    QCOMPARE(newShapes, expectedNewShapes);

    // Remove everything
    dir.removeRecursively();
}

QTEST_GUILESS_MAIN(LocalShapesFinderTest)

#include "localshapesfinder_test.moc"