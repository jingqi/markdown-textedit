
#ifndef ___HEADFILE_2D0FD370_132C_49EA_ACE1_D0A506433D2A_
#define ___HEADFILE_2D0FD370_132C_49EA_ACE1_D0A506433D2A_

#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

#include <pmh_definitions.h>

namespace mdtextedit
{

struct Task
{
    QString text;
    unsigned long offset;
};

class HighlightWorkerThread : public QThread
{
    Q_OBJECT

private:
    QQueue<Task> _tasks;
    QMutex _tasks_mutex;
    QWaitCondition _buffer_not_empty;

public:
    explicit HighlightWorkerThread(QObject *parent = 0);

    void enqueue(const QString &text, unsigned long offset = 0);

signals:
    void result_ready(pmh_element **elements, unsigned long offset);

protected:
    virtual void run();
};

}

#endif
