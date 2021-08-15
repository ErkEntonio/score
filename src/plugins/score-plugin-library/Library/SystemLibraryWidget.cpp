#include "SystemLibraryWidget.hpp"

#include <Library/FileSystemModel.hpp>
#include <Library/ItemModelFilterLineEdit.hpp>
#include <Library/LibrarySettings.hpp>
#include <Library/LibraryWidget.hpp>
#include <Library/RecursiveFilterProxy.hpp>

#include <score/application/GUIApplicationContext.hpp>
#include <score/widgets/MarginLess.hpp>

#include <core/presenter/DocumentManager.hpp>

#include <QVBoxLayout>

namespace Library
{
SystemLibraryWidget::SystemLibraryWidget(
    const score::GUIApplicationContext& ctx,
    QWidget* parent)
    : QWidget{parent}
    , m_model{new FileSystemModel{ctx, this}}
    , m_proxy{new QSortFilterProxyModel{this}}
    , m_preview{this}
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
  m_proxy->setRecursiveFilteringEnabled(true);
#endif

  setStatusTip(QObject::tr(
      "This panel shows the system library.\n"
      "It is present by default in your user's Documents folder, \n"
      "in a subfolder named ossia score library."
      "A user-provided library is available on : \n"
      "github.com/ossia/score-user-library"));
  auto lay = new score::MarginLess<QVBoxLayout>;

  this->setLayout(lay);

  m_proxy->setSourceModel(m_model);
  m_proxy->setFilterKeyColumn(0);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
  auto il = new ItemModelFilterLineEdit{*m_proxy, m_tv, this};
  lay->addWidget(il);
#endif
  lay->addWidget(&m_tv);
  lay->addWidget(&m_preview);
  m_tv.setModel(m_proxy);
  setup_treeview(m_tv);

  {
    auto previewLay = new score::MarginLess<QHBoxLayout>{&m_preview};
    m_preview.setLayout(previewLay);
    m_preview.hide();
  }

  auto sel = m_tv.selectionModel();

  connect(sel, &QItemSelectionModel::currentRowChanged, this, [&](const QModelIndex& idx, const QModelIndex&) {
    m_preview.hide();
    auto doc = ctx.docManager.currentDocument();
    if (!doc)
      return;

    delete m_previewChild;
    m_previewChild = nullptr;

    auto path = m_model->filePath(m_proxy->mapToSource(idx));
    for (auto lib : libraryInterface(path))
    {
      if ((m_previewChild = lib->previewWidget(path, &m_preview)))
      {
        m_preview.layout()->addWidget(m_previewChild);
        m_preview.show();
      }
    }
  });
  connect(&m_tv, &QTreeView::doubleClicked, this, [&](const QModelIndex& idx) {
    auto doc = ctx.docManager.currentDocument();
    if (!doc)
      return;

    auto path = m_model->filePath(m_proxy->mapToSource(idx));
    for (auto lib : libraryInterface(path))
    {
      if (lib->onDoubleClick(path, doc->context()))
        return;
    }
  });
  m_tv.setAcceptDrops(true);
  m_tv.setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
  QTimer::singleShot(1000, [this, il, &ctx] {
    auto& settings = ctx.settings<Library::Settings::Model>();
    il->reset = [this, &settings] { setRoot(settings.getPackagesPath()); };
    il->reset();
    con(settings, &Library::Settings::Model::RootPathChanged, this, il->reset);
    con(settings, &Library::Settings::Model::rescanLibrary, this, il->reset);
  });
#else
  QTimer::singleShot(1000, [this, &ctx] {
    auto& settings = ctx.settings<Library::Settings::Model>();
    auto reset = [this, &settings] { setRoot(settings.getPath()); };
    reset();
    con(settings, &Library::Settings::Model::RootPathChanged, this, [=] {
      reset();
    });
    con(settings, &Library::Settings::Model::rescanLibrary, this, [=] { reset(); });
  });
#endif
}

SystemLibraryWidget::~SystemLibraryWidget() { }

void SystemLibraryWidget::setRoot(QString path)
{
  auto idx = m_model->setRootPath(path);
  m_tv.setRootIndex(m_proxy->mapFromSource(idx));
  for (int i = 1; i < m_model->columnCount(); ++i)
    m_tv.hideColumn(i);
}

}
