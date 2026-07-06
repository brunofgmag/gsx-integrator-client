#include <QtCore/QLocale>
#include <QtCore/QSize>
#include <QtCore/QTranslator>
#include <QtGui/QFont>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtGui/QStyleHints>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickWindow>
#include "application/IntegratorRuntime.h"
#include "application/RuntimeIntegratorService.h"
#include "infrastructure/settings/QSettingsRepository.h"
#include "infrastructure/platform/WindowsTitleBar.h"
#include "viewmodel/OperationsViewModel.h"
#include "viewmodel/SettingsViewModel.h"

int main(int argc, char* argv[])
{
    const QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("brunofgmag"));
    QGuiApplication::setApplicationName(QStringLiteral("gsx-integrator-client"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("GSX Integrator"));
    QGuiApplication::setApplicationVersion(QStringLiteral(GSXI_VERSION));

    QFont monoFont(QStringLiteral("Cascadia Mono"));
    monoFont.setStyleHint(QFont::Monospace);
    QGuiApplication::setFont(monoFont);

    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

    QSettingsRepository settingsRepository;
    const AppSettings startupSettings = settingsRepository.Load();

    QTranslator translator;
    {
        const QString uiLanguage = QString::fromStdString(startupSettings.language);
        const QLocale uiLocale = (uiLanguage.isEmpty() || uiLanguage == QStringLiteral("system"))
                                     ? QLocale()
                                     : QLocale(uiLanguage);
        if (translator.load(uiLocale, QStringLiteral("app"), QStringLiteral("_"), QStringLiteral(":/i18n")))
        {
            QCoreApplication::installTranslator(&translator);
        }
    }

    QIcon appIcon;
    for (const int size : {16, 24, 32, 48, 64, 128, 256})
    {
        appIcon.addFile(QStringLiteral(":/icons/app-icon_%1.png").arg(size),
                        QSize(size, size));
    }
    QGuiApplication::setWindowIcon(appIcon);

    IntegratorRuntime runtime;
    RuntimeIntegratorService integratorService(&runtime);
    SettingsViewModel settingsViewModel(&settingsRepository, &integratorService);
    OperationsViewModel operationsViewModel(&integratorService);

    settingsViewModel.SetSystemDarkProvider([]
    {
        return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    });
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                     &settingsViewModel, [&settingsViewModel] { settingsViewModel.RefreshEffectiveTheme(); });

    runtime.Setup();

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("integratorVm"), QVariant::fromValue(&operationsViewModel)},
        {QStringLiteral("settingsVm"), QVariant::fromValue(&settingsViewModel)},
    });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);

    QObject::connect(&settingsViewModel, &SettingsViewModel::LanguageChanged, &app,
                     [&settingsViewModel, &operationsViewModel, &translator, &engine]
                     {
                         QCoreApplication::removeTranslator(&translator);
                         const QString uiLanguage = settingsViewModel.GetLanguage();
                         const QLocale uiLocale = (uiLanguage.isEmpty() || uiLanguage == QStringLiteral("system"))
                                                      ? QLocale()
                                                      : QLocale(uiLanguage);
                         if (translator.load(uiLocale, QStringLiteral("app"), QStringLiteral("_"),
                                             QStringLiteral(":/i18n")))
                         {
                             QCoreApplication::installTranslator(&translator);
                         }
                         engine.retranslate();
                         operationsViewModel.RetranslateUi();
                         settingsViewModel.RetranslateUi();
                     });

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/GsxIntegratorClient/src/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
    {
        return EXIT_FAILURE;
    }

    if (auto* rootWindow = qobject_cast<QWindow*>(engine.rootObjects().first()))
    {
        WindowsTitleBar::Apply(rootWindow, settingsViewModel.GetEffectiveDark());

        QObject::connect(&settingsViewModel, &SettingsViewModel::EffectiveDarkChanged,
                         rootWindow, [rootWindow, &settingsViewModel]
                         {
                             WindowsTitleBar::Apply(rootWindow, settingsViewModel.GetEffectiveDark());
                         });
    }

    return QGuiApplication::exec();
}
