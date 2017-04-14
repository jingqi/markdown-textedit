
#ifdef __cplusplus
extern "C" {
#endif
#   include <pmh_parser.h>
#ifdef __cplusplus
}
#endif

#include "highlight_worker_thread.h"

namespace mdtextedit
{

HighlightWorkerThread::HighlightWorkerThread(QObject *parent)
    : QThread(parent)
{}

void HighlightWorkerThread::enqueue(const QString &text, unsigned long offset)
{
    QMutexLocker locker(&_tasks_mutex);
    _tasks.enqueue(Task {text, offset});
    _buffer_not_empty.wakeOne();
}

void HighlightWorkerThread::run()
{
    forever {
        Task task;

        {
            // wait for new task
            QMutexLocker locker(&_tasks_mutex);
            while (_tasks.count() == 0)
            {
                _buffer_not_empty.wait(&_tasks_mutex);
            }

            // get last task from queue and skip all previous tasks
            while (!_tasks.isEmpty())
                task = _tasks.dequeue();
        }

        // end processing?
        if (task.text.isNull())
            return;

        // delay processing by 500 ms to see if more tasks are coming
        // (e.g. because the user is typing fast)
        this->msleep(500);

        // no more new tasks?
        if (_tasks.isEmpty())
        {
            // parse markdown and generate syntax elements
            pmh_element **elements = NULL;
            ::pmh_markdown_to_elements(task.text.toUtf8().data(), pmh_EXT_NONE, &elements);

            emit result_ready(elements, task.offset);
        }
    }
}

}
