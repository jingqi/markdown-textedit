
#ifndef ___HEADFILE_C2F68B4B_234B_4309_A132_9617DDC41119_
#define ___HEADFILE_C2F68B4B_234B_4309_A132_9617DDC41119_

#include <QWidget>

namespace mdtextedit
{

class MarkdownTextEdit;

class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberArea(MarkdownTextEdit *editor);

    virtual QSize sizeHint() const override;

protected:
    virtual void paintEvent(QPaintEvent *event) override;

private:
    MarkdownTextEdit *_editor = NULL;
};

}

#endif
