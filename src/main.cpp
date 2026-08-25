#include <windows.h>
#include <cstring>
#include <QtCore/QDir>
#include <QtCore/QLocale>
#include <QtCore/QSettings>
#include <QtCore/QSize>
#include <QtCore/QTimer>
#include <QtCore/QTranslator>
#include <QtGui/QFont>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtGui/QStyleHints>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickWindow>
#include "application/IntegratorRuntime.h"
#include "application/RuntimeIntegratorService.h"
#include "infrastructure/aircraft/AircraftFactory.h"
#include "infrastructure/settings/QSettingsRepository.h"
#include "infrastructure/platform/GraphicsBackend.h"
#include "infrastructure/probe/ProbeLog.h"
#include "infrastructure/platform/ShowWindowMessageFilter.h"
#include "infrastructure/platform/WindowForeground.h"
#include "infrastructure/platform/WindowsTitleBar.h"
#include "infrastructure/update/GithubUpdateService.h"
#include "infrastructure/efb/EfbStatePublisher.h"
#include "infrastructure/efb/EfbCommandReceiver.h"
#include "viewmodel/OperationsViewModel.h"
#include "viewmodel/SettingsViewModel.h"
#include "viewmodel/UpdateViewModel.h"

namespace
{
    bool UpdatesEnabled()
    {
        if (qEnvironmentVariableIsSet("GSXI_NO_UPDATES"))
        {
            return false;
        }
        QDir dir(QCoreApplication::applicationDirPath());
        for (int level = 0; level < 4; ++level)
        {
            if (dir.exists(QStringLiteral("CMakeCache.txt")))
            {
                return false;
            }
            if (!dir.cdUp())
            {
                break;
            }
        }
        return true;
    }

    bool LightTaskbar()
    {
        const QSettings personalize(
            QStringLiteral(
                R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)"),
            QSettings::NativeFormat);
        return personalize.value(QStringLiteral("SystemUsesLightTheme"), 0).toInt() == 1;
    }

    QtMessageHandler defaultMessageHandler = nullptr;

    void FilteredMessageHandler(const QtMsgType type, const QMessageLogContext& context, const QString& message)
    {
        if (message.contains(QLatin1String("Component is not ready")))
        {
            return;
        }
        probe::Sink(message);
        defaultMessageHandler(type, context, message);
    }

    bool HasTrayArg(const int argc, char* argv[])
    {
        for (int i = 1; i < argc; ++i)
        {
            if (std::strcmp(argv[i], "--tray") == 0)
            {
                return true;
            }
        }

        return false;
    }

    bool SecondaryInstance()
    {
        CreateMutexW(nullptr, FALSE, L"Local\\gsx-integrator-client.single-instance");

        return GetLastError() == ERROR_ALREADY_EXISTS;
    }

    void ConfigureApplication()
    {
        QGuiApplication::setQuitOnLastWindowClosed(false);
        QGuiApplication::setOrganizationName(QStringLiteral("brunofgmag"));
        QGuiApplication::setApplicationName(QStringLiteral("gsx-integrator-client"));
        QGuiApplication::setApplicationDisplayName(QStringLiteral("GSX Integrator"));
        QGuiApplication::setApplicationVersion(QStringLiteral(GSXI_VERSION));

        QFont monoFont(QStringLiteral("Cascadia Mono"));
        monoFont.setStyleHint(QFont::Monospace);
        QGuiApplication::setFont(monoFont);

        QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
    }

    void InstallAppTranslator(QTranslator& translator, const QString& language)
    {
        QCoreApplication::removeTranslator(&translator);
        const QLocale uiLocale = (language.isEmpty() || language == QStringLiteral("system"))
                                     ? QLocale()
                                     : QLocale(language);
        if (translator.load(uiLocale, QStringLiteral("app"), QStringLiteral("_"), QStringLiteral(":/i18n")))
        {
            QCoreApplication::installTranslator(&translator);
        }
    }

    enum class StartupWindow { Foreground, Minimized, Hidden };

    StartupWindow ResolveStartupWindow(const bool trayArg, const AppSettings& settings)
    {
        if (!trayArg)
        {
            return StartupWindow::Foreground;
        }

        return settings.closeToTray || settings.minimizeToTray
                   ? StartupWindow::Hidden
                   : StartupWindow::Minimized;
    }

    QIcon BuildAppIcon()
    {
        QIcon appIcon;
        for (const int size : {16, 24, 32, 48, 64, 128, 256})
        {
            appIcon.addFile(QStringLiteral(":/icons/app-icon_%1.png").arg(size),
                            QSize(size, size));
        }

        return appIcon;
    }
}

int main(int argc, char* argv[])
{
    const bool trayArg = HasTrayArg(argc, argv);

    if (SecondaryInstance())
    {
        if (!trayArg)
        {
            PostMessageW(HWND_BROADCAST, ShowWindowMessageFilter::MessageId(), 0, 0);
        }

        return 0;
    }

    defaultMessageHandler = qInstallMessageHandler(FilteredMessageHandler);

    const QGuiApplication app(argc, argv);
    ConfigureApplication();

    QSettingsRepository settingsRepository;
    const AppSettings startupSettings = settingsRepository.Load();
    const StartupWindow startupWindow = ResolveStartupWindow(trayArg, startupSettings);

    GraphicsBackend::Apply(QString::fromStdString(startupSettings.renderer));

    QTranslator translator;
    InstallAppTranslator(translator, QString::fromStdString(startupSettings.language));

    QGuiApplication::setWindowIcon(BuildAppIcon());

    IntegratorRuntime runtime;
    RuntimeIntegratorService integratorService(&runtime);
    SettingsViewModel settingsViewModel(&settingsRepository, &integratorService,
                                        SupportedAircraftProfiles());
    OperationsViewModel operationsViewModel(&integratorService, &settingsViewModel);

    EfbStatePublisher efbStatePublisher(runtime.Bridge(), &operationsViewModel,
                                        [&runtime] { return runtime.GetSimVersion(); });
    efbStatePublisher.Setup();
    QObject::connect(&operationsViewModel, &OperationsViewModel::SnapshotChanged, &operationsViewModel,
                     [&efbStatePublisher] { efbStatePublisher.Publish(); });

    EfbCommandReceiver efbCommandReceiver(runtime.Bridge(), &operationsViewModel);
    efbCommandReceiver.Setup();

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &operationsViewModel,
                     [&efbStatePublisher] { efbStatePublisher.PublishDeparture(); });

    GithubUpdateService updateService(
        qEnvironmentVariable(
            "GSXI_UPDATE_FEED",
            QStringLiteral(
                "https://api.github.com/repos/brunofgmag/gsx-integrator-client/releases/latest")),
        qEnvironmentVariable(
            "GSXI_COMMBUS_UPDATE_FEED",
            QStringLiteral(
                "https://api.github.com/repos/brunofgmag/gsx-integrator-commbus/releases/latest")),
        QGuiApplication::applicationVersion());
    UpdateViewModel updateViewModel(&updateService,
                                    startupSettings.updateMode,
                                    UpdatesEnabled());

    QObject::connect(&settingsViewModel, &SettingsViewModel::UpdateModeChanged, &updateViewModel,
                     [&settingsViewModel, &updateViewModel]
                     {
                         updateViewModel.SetMode(settingsViewModel.GetUpdateMode());
                     });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &updateViewModel,
                     [&updateViewModel, &updateService]
                     {
                         if (updateViewModel.ShouldApplyOnExit())
                         {
                             updateService.LaunchApplyHelper(false);
                         }
                     });

    settingsViewModel.SetSystemDarkProvider([]
    {
        return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    });
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                     &settingsViewModel, [&settingsViewModel] { settingsViewModel.RefreshEffectiveTheme(); });

    QObject::connect(&runtime, &IntegratorRuntime::SimulatorQuit,
                     &app, [] { QCoreApplication::exit(0); }, Qt::QueuedConnection);

    runtime.Setup();

    const QString trayIconSource = LightTaskbar()
                                       ? QStringLiteral("qrc:/icons/tray_dark_32.png")
                                       : QStringLiteral("qrc:/icons/tray_white_32.png");

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("integratorVm"), QVariant::fromValue(&operationsViewModel)},
        {QStringLiteral("settingsVm"), QVariant::fromValue(&settingsViewModel)},
        {QStringLiteral("updateVm"), QVariant::fromValue(&updateViewModel)},
        {QStringLiteral("startHidden"), startupWindow == StartupWindow::Hidden},
        {QStringLiteral("startMinimized"), startupWindow == StartupWindow::Minimized},
        {QStringLiteral("trayIconSource"), trayIconSource},
    });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);

    QObject::connect(&settingsViewModel, &SettingsViewModel::LanguageChanged, &app,
                     [&settingsViewModel, &operationsViewModel, &translator, &engine]
                     {
                         InstallAppTranslator(translator, settingsViewModel.GetLanguage());
                         engine.retranslate();
                         operationsViewModel.RefreshDisplayText();
                         settingsViewModel.RetranslateUi();
                     });

    const auto refreshOperationsText = [&operationsViewModel] { operationsViewModel.RefreshDisplayText(); };
    QObject::connect(&settingsViewModel, &SettingsViewModel::WeightUnitDisplayChanged, &app,
                     refreshOperationsText);
    QObject::connect(&settingsViewModel, &SettingsViewModel::FuelRateTextChanged, &app,
                     refreshOperationsText);
    QObject::connect(&settingsViewModel, &SettingsViewModel::AutoStartFlowChanged, &app,
                     refreshOperationsText);
    QObject::connect(&settingsViewModel, &SettingsViewModel::AutoStartLoadingChanged, &app,
                     refreshOperationsText);

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/GsxIntegratorClient/src/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
    {
        return EXIT_FAILURE;
    }

    auto* rootWindow = qobject_cast<QWindow*>(engine.rootObjects().first());
    ShowWindowMessageFilter showWindowFilter(rootWindow);
    if (rootWindow != nullptr)
    {
        QCoreApplication::instance()->installNativeEventFilter(&showWindowFilter);
        WindowsTitleBar::Apply(rootWindow, settingsViewModel.GetEffectiveDark());

        if (auto* quickWindow = qobject_cast<QQuickWindow*>(rootWindow))
        {
            const auto publishActiveRenderer = [quickWindow, &settingsViewModel]
            {
                if (const auto* renderer = quickWindow->rendererInterface())
                {
                    settingsViewModel.SetActiveRenderer(
                        GraphicsBackend::ToName(renderer->graphicsApi()));
                }
            };

            publishActiveRenderer();
            QObject::connect(quickWindow, &QQuickWindow::sceneGraphInitialized, quickWindow,
                             publishActiveRenderer);
        }

        if (startupWindow == StartupWindow::Foreground)
        {
            QTimer::singleShot(0, rootWindow, [rootWindow] {
                WindowForeground::Bring(rootWindow);
            });
        }

        QObject::connect(&settingsViewModel, &SettingsViewModel::EffectiveDarkChanged,
                         rootWindow, [rootWindow, &settingsViewModel]
                         {
                             WindowsTitleBar::Apply(rootWindow, settingsViewModel.GetEffectiveDark());
                         });
    }

    return QGuiApplication::exec();
}
