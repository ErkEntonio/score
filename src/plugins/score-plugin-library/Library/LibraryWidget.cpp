// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "LibraryWidget.hpp"

#include <Library/FileSystemModel.hpp>
#include <Library/LibrarySettings.hpp>

#include <score/tools/std/StringHash.hpp>
#include <score/tools/Bind.hpp>
#include <score/widgets/MarginLess.hpp>
#include <score/widgets/SearchLineEdit.hpp>

#include <core/presenter/DocumentManager.hpp>

#include <QLabel>
#include <QScrollArea>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include <wobjectimpl.h>

#include <unordered_map>

W_OBJECT_IMPL(Library::ProcessTreeView)
namespace Library
{
class RecursiveFilterProxy final : public QSortFilterProxyModel
{
public:
  using QSortFilterProxyModel::QSortFilterProxyModel;

private:
  bool
  filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const override
  {
    if (filterAcceptsRowItself(srcRow, srcParent))
    {
      return true;
    }

    // Accept if any of the parents is accepted on its own
    for (QModelIndex parent = srcParent; parent.isValid();
         parent = parent.parent())
      if (filterAcceptsRowItself(parent.row(), parent.parent()))
      {
        return true;
      }

    // Accept if any of the children is accepted on its own
    return hasAcceptedChildren(srcRow, srcParent);
  }

  bool filterAcceptsRowItself(int srcRow, const QModelIndex& srcParent) const
  {
    QModelIndex index = sourceModel()->index(srcRow, 0, srcParent);
    return sourceModel()->data(index).toString().contains(filterRegExp());
  }

  bool hasAcceptedChildren(int srcRow, const QModelIndex& srcParent) const
  {
    QModelIndex index = sourceModel()->index(srcRow, 0, srcParent);

    if (!index.isValid())
      return false;

    SCORE_ASSERT(index.model());
    const int childCount = index.model()->rowCount(index);

    if (childCount == 0)
      return false;

    for (int i = 0; i < childCount; ++i)
    {
      if (filterAcceptsRowItself(i, index))
        return true;

      if (hasAcceptedChildren(i, index))
        return true;
    }

    return false;
  }
};

struct ItemModelFilterLineEdit final : public score::SearchLineEdit
{
public:
  ItemModelFilterLineEdit(
      QSortFilterProxyModel& proxy,
      QTreeView& tv,
      QWidget* p)
      : score::SearchLineEdit{p}, m_proxy{proxy}, m_view{tv}
  {
    connect(this, &QLineEdit::textEdited, this, [=] { search(); });

  }

  void search() override
  {
    if (text() != m_proxy.filterRegExp().pattern())
    {
      m_proxy.setFilterRegExp(
          QRegExp(text(), Qt::CaseInsensitive, QRegExp::FixedString));

      if (text().isEmpty())
        m_view.collapseAll();
      else
        m_view.expandAll();
    }
  }

  QSortFilterProxyModel& m_proxy;
  QTreeView& m_view;
};

void ProcessTreeView::selectionChanged(
    const QItemSelection& sel,
    const QItemSelection& desel)
{
  if (sel.size() > 0)
  {
    auto idx = sel.indexes().front();
    auto proxy = (QSortFilterProxyModel*)this->model();
    auto model_idx = proxy->mapToSource(idx);
    auto data = reinterpret_cast<TreeNode<ProcessData>*>(
        model_idx.internalPointer());

    selected(*data);
  }
  else if (desel.size() > 0)
  {
    selected({});
  }
}

class InfoWidget final : public QScrollArea
{
public:
  InfoWidget(QWidget* parent)
  {
    setMinimumHeight(120);
    setMaximumHeight(160);
    auto lay = new QVBoxLayout{this};
    QFont f;
    f.setBold(true);
    m_name.setFont(f);
    lay->addWidget(&m_name);
    lay->addWidget(&m_author);
    m_author.setWordWrap(true);
    lay->addWidget(&m_io);
    lay->addWidget(&m_description);
    m_description.setWordWrap(true);
    lay->addWidget(&m_tags);
    m_tags.setWordWrap(true);
    setVisible(false);
  }

  void setData(const optional<ProcessData>& d)
  {
    if (d)
    {
      if (auto f = score::GUIAppContext()
                       .interfaces<Process::ProcessFactoryList>()
                       .get(d->key))
      {
        setVisible(true);
        auto desc = f->descriptor(d->json["Data"].toString());
        m_name.setText(desc.prettyName);
        m_author.setText(tr("Provided by ") + desc.author);
        m_description.setText(desc.description);
        QString io;
        if (desc.inlets)
        {
          io += QString::number(desc.inlets->size()) + tr(" input");
          if (desc.inlets->size() > 1)
            io += "s";
        }
        if (desc.inlets && desc.outlets)
          io += ", ";
        if (desc.outlets)
        {
          io += QString::number(desc.outlets->size()) + tr(" output");
          if (desc.outlets->size() > 1)
            io += "s";
        }
        m_io.setText(io);
        if (!desc.tags.empty())
        {
          m_tags.setText(tr("Tags: ") + desc.tags.join(", "));
        }
        else
        {
          m_tags.clear();
        }
        return;
      }
    }

    setVisible(false);
  }

  QLabel m_name;
  QLabel m_io;
  QLabel m_description;
  QLabel m_author;
  QLabel m_tags;
};

static void setup_treeview(QTreeView& tv)
{
  tv.setHeaderHidden(true);
  tv.setDragEnabled(true);
  tv.setDropIndicatorShown(true);
  tv.setAlternatingRowColors(true);
  tv.setSelectionMode(QAbstractItemView::SingleSelection);
  for (int i = 1; i < tv.model()->columnCount(); ++i)
    tv.hideColumn(i);
}

ProcessWidget::ProcessWidget(
    const score::GUIApplicationContext& ctx,
    QWidget* parent)
    : QWidget{parent}, m_model{new ProcessesItemModel{ctx, this}}
{
  setStatusTip(QObject::tr("This panel shows the available processes.\n"
                           "They can be drag'n'dropped in the score, in intervals, "
                           "and sometimes in effect chains."));
  auto lay = new score::MarginLess<QVBoxLayout>;

  this->setLayout(lay);

  auto m_proxy = new RecursiveFilterProxy{this};
  m_proxy->setSourceModel(m_model);
  m_proxy->setFilterKeyColumn(0);
  lay->addWidget(new ItemModelFilterLineEdit{*m_proxy, m_tv, this});
  lay->addWidget(&m_tv);
  m_tv.setModel(m_proxy);
  setup_treeview(m_tv);

  auto widg = new InfoWidget{this};
  lay->addWidget(widg);
  m_tv.setStatusTip(statusTip());
  widg->setStatusTip(statusTip());

  con(m_tv, &ProcessTreeView::selected, this, [=](const auto& pdata) {
    widg->setData(pdata);
  });
}

ProcessWidget::~ProcessWidget() {}

std::vector<LibraryInterface*> libraryInterface(const QString& path)
{
  static auto matches = [] {
    std::unordered_multimap<QString, LibraryInterface*> exp;
    const auto& libs
        = score::GUIAppContext().interfaces<LibraryInterfaceList>();
    for (auto& lib : libs)
    {
      for (const auto& ext : lib.acceptedFiles())
      {
        exp.insert({ext, &lib});
      }
    }
    return exp;
  }();

  std::vector<LibraryInterface*> libs;
  auto [begin, end] = matches.equal_range(QFileInfo(path).suffix());

  for (auto it = begin; it != end; ++it)
  {
    libs.push_back(it->second);
  }
  return libs;
}

SystemLibraryWidget::SystemLibraryWidget(
    const score::GUIApplicationContext& ctx,
    QWidget* parent)
    : QWidget{parent}
    , m_model{new FileSystemModel{ctx, this}}
    , m_proxy{new RecursiveFilterProxy{this}}
{
  setStatusTip(QObject::tr("This panel shows the system library.\n"
                           "It is present by default in your user's Documents folder, \n"
                           "in a subfolder named ossia score library."
                           "A user-provided library is available on : \n"
                           "github.com/OSSIA/score-user-library"));
  auto lay = new score::MarginLess<QVBoxLayout>;

  this->setLayout(lay);

  m_proxy->setSourceModel(m_model);
  m_proxy->setFilterKeyColumn(0);
  lay->addWidget(new ItemModelFilterLineEdit{*m_proxy, m_tv, this});
  lay->addWidget(&m_tv);
  m_tv.setModel(m_proxy);
  setup_treeview(m_tv);
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

  setRoot(ctx.settings<Library::Settings::Model>().getPath());
}

SystemLibraryWidget::~SystemLibraryWidget() {}

void SystemLibraryWidget::setRoot(QString path)
{
  auto idx = m_model->setRootPath(path);
  m_tv.setRootIndex(m_proxy->mapFromSource(idx));
  for (int i = 1; i < m_model->columnCount(); ++i)
    m_tv.hideColumn(i);
}

ProjectLibraryWidget::ProjectLibraryWidget(
    const score::GUIApplicationContext& ctx,
    QWidget* parent)
    : QWidget{parent}
    , m_model{new FileSystemModel{ctx, this}}
    , m_proxy{new RecursiveFilterProxy{this}}
{
  auto lay = new score::MarginLess<QVBoxLayout>;
  setStatusTip(QObject::tr("This panel shows the project library.\n"
                           "It lists the files in the folder where the score is saved."));

  this->setLayout(lay);

  m_proxy->setSourceModel(m_model);
  m_proxy->setFilterKeyColumn(0);
  lay->addWidget(new ItemModelFilterLineEdit{*m_proxy, m_tv, this});
  lay->addWidget(&m_tv);
  m_tv.setModel(m_proxy);
  setup_treeview(m_tv);
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
}

ProjectLibraryWidget::~ProjectLibraryWidget() {}

void ProjectLibraryWidget::setRoot(QString path)
{
  if (!path.isEmpty())
  {
    auto idx = m_model->setRootPath(path);

    m_tv.setModel(m_proxy);
    m_tv.setRootIndex(m_proxy->mapFromSource(idx));
    for (int i = 1; i < m_model->columnCount(); ++i)
      m_tv.hideColumn(i);
  }
  else
  {
    m_tv.setModel(nullptr);
  }
}
}
