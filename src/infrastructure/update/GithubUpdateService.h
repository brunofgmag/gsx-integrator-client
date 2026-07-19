#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GITHUBUPDATESERVICE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GITHUBUPDATESERVICE_H

#include <vector>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtNetwork/QNetworkAccessManager>
#include "../../application/ports/UpdateService.h"

class QNetworkReply;
class QProcess;

class GithubUpdateService final : public QObject, public UpdateService
{
    Q_OBJECT

public:
    explicit GithubUpdateService(QString clientFeedUrl,
                                 QString commbusFeedUrl,
                                 QString currentVersion,
                                 QObject* parent = nullptr);

    void CheckForUpdates() override;
    void DownloadAndStage(const UpdateInfo& info) override;
    void DiscardStaged() override;
    [[nodiscard]] bool HasStagedUpdate() const override;
    bool LaunchApplyHelper(bool relaunch) override;

    void AddObserver(UpdateServiceObserver* observer) override;
    void RemoveObserver(UpdateServiceObserver* observer) override;

private:
    void OnClientCheckHttpFinished();
    void OnCommbusCheckHttpFinished();
    void OnShaHttpFinished();
    void OnZipHttpFinished();
    void OnExtractFinished(int exitCode);

    [[nodiscard]] QNetworkReply* StartGet(const QString& url);
    [[nodiscard]] static QString UpdatesRootDir();
    [[nodiscard]] static QString StagedAppDir();
    [[nodiscard]] bool VerifyChecksum(const QString& zipPath) const;
    void StartExtraction(const QString& zipPath, const QString& stagedRoot);
    [[nodiscard]] QStringList BuildApplyArguments(const QString& scriptPath, const QString& exeName,
                                                  bool relaunch) const;
    void NotifyCheckFinished(bool ok, bool available, const UpdateInfo& info, const QString& error) const;
    void NotifyCommbusCheckFinished(bool ok, const QString& installedVersion,
                                    const QString& latestVersion, const QString& releaseUrl) const;
    void NotifyStageFinished(bool ok, const QString& error);

    QNetworkAccessManager network_;
    QString clientFeedUrl_;
    QString commbusFeedUrl_;
    QString currentVersion_;
    std::vector<UpdateServiceObserver*> observers_;

    QNetworkReply* clientCheckReply_ = nullptr;
    QNetworkReply* commbusCheckReply_ = nullptr;
    QNetworkReply* shaReply_ = nullptr;
    QNetworkReply* zipReply_ = nullptr;
    QProcess* extractProcess_ = nullptr;

    UpdateInfo pendingDownload_;
    QString expectedSha256_;
    QString stagedVersion_;
    bool helperLaunched_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GITHUBUPDATESERVICE_H
