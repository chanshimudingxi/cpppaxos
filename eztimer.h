#ifndef EZ_TIMER_H
#define EZ_TIMER_H
#include <functional>
#include <stdint.h>

#include "sys/util.h"

class EzTimer{
public:
    EzTimer(long millisecondDelay, std::function<void()> callback):
        m_period(millisecondDelay), m_lastExecuteTime(0), m_callback(callback){}
    ~EzTimer(){}
    void execute(){
        uint64_t nowMs = deps::GetMonoTimeMs();
        if(m_lastExecuteTime == 0 || m_lastExecuteTime + (uint64_t)m_period < nowMs){
            m_lastExecuteTime = nowMs;
            m_callback();
        }
    }
private:
    long m_period;
    uint64_t m_lastExecuteTime;
    std::function<void()> m_callback;
};


class EzTimerManager{
public:
    EzTimerManager():m_timers(){}
    ~EzTimerManager(){}
    bool addTimer(long millisecondDelay, std::function<void()> callback){
        m_timers.push_back(EzTimer(millisecondDelay, callback));
        return true;
    }
    void checkTimer(){
        for(auto& t: m_timers){
            t.execute();
        }
    }
private:
    std::vector<EzTimer> m_timers;
};
#endif