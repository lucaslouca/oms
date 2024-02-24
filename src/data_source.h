#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <memory>

class DataSource {
   protected:
    int m_poll_interval = 10;
    virtual void Query() = 0;

   public:
    virtual ~DataSource(){};
    int NextPollInterval() const { return m_poll_interval; }
    void Poll() { this->Query(); }
    virtual void Stop() = 0;
};

#endif