#pragma once
#include <iscore/command/Dispatchers/SingleOngoingCommandDispatcher.hpp>
#include <iscore/command/Dispatchers/CommandDispatcher.hpp>

#include <QState>
#include <QStateMachine>
#include <QPointF>

class ScenarioStateMachine;
// TODO refactor this one.
class GenericStateBase : public QState
{
    public:
        GenericStateBase(const ScenarioStateMachine& sm) :
            m_sm{sm}
        {
        }


    protected:
        QStateMachine m_localSM;
        const ScenarioStateMachine& m_sm;
};

class SlotModel;
class MoveSlotToolState : public GenericStateBase
{
    public:
        MoveSlotToolState(const ScenarioStateMachine &sm);

        void on_scenarioPressed();
        void on_scenarioMoved();
        void on_scenarioReleased();

        // TODO refactor this with ToolState
        void start()
        {
            if(!m_localSM.isRunning())
                m_localSM.start();
        }
        void stop()
        {
            if(m_localSM.isRunning())
                m_localSM.stop();
        }

    private:
        QState* m_waitState{};

        QPointF m_originalPoint;
};
