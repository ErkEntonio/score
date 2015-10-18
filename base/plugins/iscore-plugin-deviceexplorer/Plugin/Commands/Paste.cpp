#include "Paste.hpp"

using namespace DeviceExplorer::Command;

Paste::Paste(const iscore::NodePath &parentPath, int row,
                                const QString& text,
                                Path<DeviceExplorerModel> &&Path):
    iscore::SerializableCommand{"DeviceExplorerControl",
                                commandName(),
                                description()}
{
    m_model = Path;
    m_row = row;
    m_parentPath = parentPath;
    setText(text);
}


void
Paste::undo() const
{
    auto& model = m_model.find();

    QModelIndex parentIndex = model.convertPathToIndex(m_parentPath);

    QModelIndex index = parentIndex.child(m_row + 1, 0);  //+1 because pasteAfter
    const DeviceExplorer::Result result = model.cut_aux(index);
    model.setCachedResult(result);

}

void
Paste::redo() const
{
    auto& model = m_model.find();
    QModelIndex parentIndex = model.convertPathToIndex(m_parentPath);

    QModelIndex index = parentIndex.child(m_row, 0);
    const DeviceExplorer::Result result = model.pasteAfter_aux(index);
    model.setCachedResult(result);

}

void
Paste::serializeImpl(QDataStream& d) const
{
    d << m_model << m_parentPath;
    d << (qint32) m_row;

    d << (qint32) m_data.size();
    d.writeRawData(m_data.data(), m_data.size());

}

void
Paste::deserializeImpl(QDataStream& d)
{
    d >> m_model >> m_parentPath;
    qint32 v;
    d >> v;
    m_row = v;

    d >> v;
    int size = v;
    m_data.resize(size);
    d.readRawData(m_data.data(), size);
}
