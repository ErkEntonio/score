#pragma once
#include <score/plugins/settingsdelegate/SettingsDelegateView.hpp>

#include <QGridLayout>
#include <QNetworkAccessManager>
#include <QProgressBar>
#include <QPushButton>
#include <QTabWidget>
#include <QTableView>
#include <QWidget>

#include <PackageManager/PluginItemModel.hpp>

#include <verdigris>
class QObject;

namespace PM
{
class PluginSettingsPresenter;
class PluginSettingsView : public score::GlobalSettingsView
{
  W_OBJECT(PluginSettingsView)
public:
  PluginSettingsView();

  QTableView* localView() { return m_addonsOnSystem; }
  QTableView* remoteView() { return m_remoteAddons; }

  QPushButton& installButton() const { return *m_install; }

  QWidget* getWidget() override;

private:
  void handleAddonList(const QJsonObject&);
  void handleAddon(const QJsonObject&);

  void installAddon(const RemotePackage& addon);
  void installLibrary(const RemotePackage& addon);
  void installSDK(const RemotePackage& addon);

  void install();
  void on_message(QNetworkReply* rep);

  void on_packageInstallSuccess(const RemotePackage& addon, const QDir& destination, const std::vector<QString>& res);
  void on_packageInstallFailure(const RemotePackage& addon);

  QTabWidget* m_widget = new QTabWidget;

  QTableView* m_addonsOnSystem{new QTableView};

  QTableView* m_remoteAddons{new QTableView};
  QPushButton* m_refresh{new QPushButton{tr("Refresh")}};
  QPushButton* m_install{new QPushButton{tr("Install")}};

  QProgressBar* m_progress{new QProgressBar};
  QNetworkAccessManager mgr;

  int m_addonsToRetrieve = 0;
};
}
