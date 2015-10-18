#pragma once
#include <DeviceExplorer/Address/AddressSettings.hpp>

#include <iscore/tools/ModelPath.hpp>
#include <iscore/command/SerializableCommand.hpp>

class AutomationModel;
class ChangeAddress : public iscore::SerializableCommand
{
        ISCORE_COMMAND_DECL("AutomationControl", "ChangeAddress", "ChangeAddress")
    public:
        ISCORE_SERIALIZABLE_COMMAND_DEFAULT_CTOR(ChangeAddress)
        ChangeAddress(
                Path<AutomationModel>&& path,
                const iscore::Address& newval);

    public:
        void undo() const override;
        void redo() const override;

    protected:
        void serializeImpl(QDataStream &) const override;
        void deserializeImpl(QDataStream &) override;

    private:
        Path<AutomationModel> m_path;
        iscore::FullAddressSettings m_old, m_new;
};


