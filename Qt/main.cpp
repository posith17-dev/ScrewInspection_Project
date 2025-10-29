#include <QCoreApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QLibraryInfo>
#include <QPluginLoader>
#include <QDir>
#include "server.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qInfo() << "========================================";
    qInfo() << "MySQL 드라이버 상세 디버깅";
    qInfo() << "========================================";

    QString appDir = QCoreApplication::applicationDirPath();

    // 1. 경로 정보
    qInfo() << "실행 파일 경로:" << appDir;
    qInfo() << "플러그인 경로:" << QCoreApplication::libraryPaths();

    // 2. DLL 파일 확인
    qInfo() << "\n[DLL 파일 확인]";
    qInfo() << "libmysql.dll (bin):" << QFile::exists(appDir + "/libmysql.dll");
    qInfo() << "libmysql.dll (sqldrivers):" << QFile::exists(appDir + "/sqldrivers/libmysql.dll");
    qInfo() << "qsqlmysql.dll:" << QFile::exists(appDir + "/sqldrivers/qsqlmysql.dll");
    qInfo() << "Qt6Core.dll:" << QFile::exists(appDir + "/Qt6Core.dll");
    qInfo() << "Qt6Sql.dll:" << QFile::exists(appDir + "/Qt6Sql.dll");

    // 3. 드라이버 플러그인 직접 로드 시도
    qInfo() << "\n[드라이버 플러그인 로드 시도]";
    QString driverPath = appDir + "/sqldrivers/qsqlmysql.dll";
    QPluginLoader loader(driverPath);

    qInfo() << "플러그인 경로:" << driverPath;
    qInfo() << "플러그인 로드 가능:" << loader.load();

    if (!loader.load()) {
        qCritical() << "❌ 플러그인 로드 실패:";
        qCritical() << "   오류:" << loader.errorString();
    } else {
        qInfo() << "✅ 플러그인 로드 성공";
        qInfo() << "   메타데이터:" << loader.metaData();
    }

    // 4. 사용 가능한 드라이버
    qInfo() << "\n[사용 가능한 SQL 드라이버]";
    qInfo() << QSqlDatabase::drivers();

    // 5. QMYSQL 드라이버 생성 시도
    qInfo() << "\n[QMYSQL 드라이버 생성 시도]";
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test");

    if (!db.isValid()) {
        qCritical() << "❌ 드라이버 생성 실패";
        qCritical() << "   오류:" << db.lastError().text();
    } else {
        qInfo() << "✅ 드라이버 생성 성공";
    }

    QSqlDatabase::removeDatabase("test");

    qInfo() << "========================================\n";

    qInfo() << "========================================";
    qInfo() << "나사 검사 Qt 서버";
    qInfo() << "========================================";

    // 서버 생성 및 시작
    Server server(5555);  // MFC 포트: 5555
    
    if (!server.startServer()) {
        qCritical() << "서버 시작 실패!";
        return -1;
    }

    qInfo() << "";
    qInfo() << "서버 실행 중... (Ctrl+C로 종료)";
    qInfo() << "";

    return app.exec();
}
