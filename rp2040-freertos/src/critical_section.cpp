//
// Created by Keijo Länsikunnas on 30.8.2024.
//
#include "FreeRTOS.h"
#include "task.h"

class CriticalSection {
public:
    void lock();
    void unlock();
};

void CriticalSection::lock() {
    taskENTER_CRITICAL();
}
void CriticalSection::unlock() {
    taskEXIT_CRITICAL();
}


#include <mutex>

void function(bool condition) {
    if(condition) {
        CriticalSection cs;
        std::lock_guard<CriticalSection> crs(cs);
        // this is inside critical section
    }
}