/**
* @projectName   runcat
* @brief         run cat on windows taskbar
* @author        zhouyohu
* @date          2021-11-23
*/
#include <QApplication>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QMenu>
#include <QtDebug>
#include <QSettings>
#include <QFileInfo>
#include <QActionGroup>

#include <Windows.h>

static FILETIME preIdleTime;
static FILETIME preKernelTime;
static FILETIME preUserTime;

static __int64 Filetime2Int64(const FILETIME* ftime)
{
    LARGE_INTEGER li;
    li.LowPart = ftime->dwLowDateTime;
    li.HighPart = ftime->dwHighDateTime;
    return li.QuadPart;
}

static __int64 CompareFileTime(FILETIME preTime,FILETIME nowTime)
{
    return Filetime2Int64(&nowTime) - Filetime2Int64(&preTime);
}

template<typename T> const T myMin(const T&left, const T&right){
    return left < right ? left : right;
}

template<typename T> const T myMax(const T&left, const T&right){
    return left > right ? left : right;
}

static bool GetCpuUsage(double &nCpuRate)
{
    nCpuRate = -1;
    FILETIME idleTime;
    FILETIME kernelTime;
    FILETIME userTime;
    auto ret = GetSystemTimes(&idleTime, &kernelTime, &userTime);
    if(!ret){
        qInfo()<<"GetSystemTimes failed!";
        return false;
    }

    long long idle   = CompareFileTime(preIdleTime, idleTime);
    long long kernel = CompareFileTime(preKernelTime, kernelTime);
    long long user   = CompareFileTime(preUserTime, userTime);
    nCpuRate = ceil(100.0 * (kernel + user - idle) / (kernel + user));

    preIdleTime = idleTime;
    preKernelTime = kernelTime;
    preUserTime = userTime;

    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    int current = 0;
    QString iconsCat[] = {":/resource/imgs/cat/light_cat_0.ico",
                          ":/resource/imgs/cat/light_cat_1.ico",
                          ":/resource/imgs/cat/light_cat_2.ico",
                          ":/resource/imgs/cat/light_cat_3.ico",
                          ":/resource/imgs/cat/light_cat_4.ico"};
    QString iconsParrot[] = {":/resource/imgs/parrot/light_parrot_0.ico",
                             ":/resource/imgs/parrot/light_parrot_1.ico",
                             ":/resource/imgs/parrot/light_parrot_2.ico",
                             ":/resource/imgs/parrot/light_parrot_3.ico",
                             ":/resource/imgs/parrot/light_parrot_4.ico",
                             ":/resource/imgs/parrot/light_parrot_5.ico",
                             ":/resource/imgs/parrot/light_parrot_6.ico",
                             ":/resource/imgs/parrot/light_parrot_7.ico",
                             ":/resource/imgs/parrot/light_parrot_8.ico",
                             ":/resource/imgs/parrot/light_parrot_9.ico"};
    QSystemTrayIcon tray(QPixmap(":/resource/imgs/cat/light_cat_0.ico"));

    QMenu trayMenu;
    auto runner = trayMenu.addAction(QIcon(), QObject::tr("Runner"));
    QMenu runnerMenu;
    auto acRunnerCat = runnerMenu.addAction("Cat");
    auto acRunnerParrot = runnerMenu.addAction("Parrot");
    QActionGroup group1(&a);
    group1.addAction(acRunnerCat);
    group1.addAction(acRunnerParrot);
    acRunnerCat->setCheckable(true);
    acRunnerParrot->setCheckable(true);
    acRunnerCat->setChecked(true);
    runner->setMenu(&runnerMenu);

    auto theme = trayMenu.addAction(QIcon(), QObject::tr("Theme"));
    QMenu themeMenu;
    auto acThemeDefault = themeMenu.addAction("Default");
    auto acThemeDark    = themeMenu.addAction("Dark");
    auto acThemeLight   = themeMenu.addAction("Light");
    QActionGroup group2(&a);
    group2.addAction(acThemeDefault);
    group2.addAction(acThemeDark);
    group2.addAction(acThemeLight);
    acThemeDefault->setCheckable(true);
    acThemeDark->setCheckable(true);
    acThemeLight->setCheckable(true);
    acThemeDefault->setChecked(true);
    theme->setMenu(&themeMenu);
    theme->setEnabled(false);

    auto acStartUp = trayMenu.addAction(QIcon(), QObject::tr("Starup"), &a, [&](bool checked){
        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                           QSettings::NativeFormat);
        auto appPath = QCoreApplication::applicationFilePath();
        auto appName = QFileInfo(appPath).fileName();
        if(appName.isEmpty()) return;
        appName = appName.split('0').at(0);
        qInfo()<<appPath<<appName;
        if( checked )
        {
            settings.setValue(appName, appPath.replace("/", "\\"));
        }
        else{
            settings.remove(appName);
        }
    });
    acStartUp->setCheckable(true);

    trayMenu.addAction(QIcon(":/resource/imgs/others/close_window_48px.png"), QObject::tr("exit"), &a, [&](){
        a.quit();
    });

    tray.setContextMenu(&trayMenu);
    tray.setVisible(true);

    QTimer timerAnimation;
    QObject::connect(&timerAnimation, &QTimer::timeout, [&](){
        if (5 <= current) current = 0;
        auto pix = QPixmap(group1.checkedAction() == acRunnerCat ? iconsCat[current] : iconsParrot[current]);
        tray.setIcon(pix);

        current = (current + 1) % 5;
    });
    timerAnimation.start(200);

    QTimer timerCpu;
    QObject::connect(&timerCpu, &QTimer::timeout, [&](){
        double s = 0.0;
        GetCpuUsage(s);
        tray.setToolTip(QString("CPU:%1%").arg(s, 1));
        s = 200.0f / (float)myMax<double>(1.0f, myMin<double>(20.0f, s / 5.0f));
        timerAnimation.stop();
        timerAnimation.start(s);
    });
    timerCpu.start(3000);

    return a.exec();
}
