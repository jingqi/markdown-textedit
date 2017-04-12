
#include "line_number_area.h"

#include "markdown_textedit.h"

namespace organic
{

LineNumberArea::LineNumberArea(MarkdownTextEdit *editor) :
    QWidget(editor), _editor(editor)
{}

QSize LineNumberArea::sizeHint() const
{
    return QSize(_editor->line_number_area_width(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    _editor->line_number_area_paint_event(event);
}

}
