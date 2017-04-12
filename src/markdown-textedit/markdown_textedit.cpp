
#include <assert.h>

#include <QKeyEvent>
#include <QGuiApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QDir>
#include <QDesktopServices>
#include <QLayout>
#include <QTimer>
#include <QTextBlock>
#include <QIcon>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QMenu>
#include <QPainter>
#include <QStyle>

#include <pmh-adapter/style_parser.h>

#include "markdown_textedit.h"
#include "highlighter/markdown_highlighter.h"
#include "line_number_area.h"

namespace organic
{

MarkdownTextEdit::MarkdownTextEdit(QWidget *parent)
    : QPlainTextEdit(parent), _line_Number_area(new LineNumberArea(this))
{
    // Tracking mouse
    setMouseTracking(true);

    init_actions();
    init_popup_menu();

    // setup the markdown highlighting
    _highlighter = new MarkdownHighlighter(document());

    // Set font
    QFont font("Monospace", 10);
    font.setStyleHint(QFont::TypeWriter);
    _line_Number_area->setFont(font);
    setFont(font);
    font = this->font();

    // set the tab stop to the width of 4 spaces in the editor
    const int tabStop = 4;
    QFontMetrics metrics(font);
    setTabStopWidth(tabStop * metrics.width(' '));

    // add shortcuts for duplicating text
//    new QShortcut( QKeySequence( "Ctrl+D" ), this, SLOT( duplicateText() ) );
//    new QShortcut( QKeySequence( "Ctrl+Alt+Down" ), this, SLOT( duplicateText() ) );

    // add a layout to the widget
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setMargin(0);
    layout->addStretch();
    this->setLayout(layout);

    // add the hidden search widget
    _search_widget = new TextEditSearchWidget(this);
    this->layout()->addWidget(_search_widget);

    QObject::connect(this, SIGNAL(textChanged()),
                     this, SLOT(adjust_right_margin()));

    connect(this, SIGNAL(blockCountChanged(int)),
            this, SLOT(update_line_number_area_width(int)));
    connect(this, SIGNAL(updateRequest(QRect, int)),
            this, SLOT(update_line_number_area(QRect, int)));

    // workaround for disabled signals up initialization
    QTimer::singleShot(300, this, SLOT(adjust_right_margin()));
    
    update_line_number_area_width(0);
}

void MarkdownTextEdit::init_actions()
{
    // Undo
    assert(NULL == _action_undo);
    _action_undo = new QAction(
        QIcon::fromTheme("edit-undo", QIcon(":/markdown-textedit/undo")),
        tr("撤销(&U)"), this);
    QString s;
    QList<QKeySequence> ks = QKeySequence::keyBindings(QKeySequence::Undo);
    if (!ks.empty())
        s = " (" + ks.at(0).toString(QKeySequence::NativeText) + ")";
    _action_undo->setStatusTip(tr("撤销") + s);
    _action_undo->setToolTip(tr("撤销") + s);
    _action_undo->setShortcut(QKeySequence::Undo);
    _action_undo->setPriority(QAction::LowPriority);
    connect(document(), SIGNAL(undoAvailable(bool)),
            _action_undo, SLOT(setEnabled(bool)));
    connect(_action_undo, SIGNAL(triggered()), this, SLOT(undo()));
    _action_undo->setEnabled(false);

    // Redo
    assert(NULL == _action_redo);
    _action_redo = new QAction(
        QIcon::fromTheme("edit-redo", QIcon(":/markdown-textedit/redo")),
        tr("重做(&R)"), this);
    s.clear();
    ks = QKeySequence::keyBindings(QKeySequence::Redo);
    if (!ks.empty())
        s = " (" + ks.at(0).toString(QKeySequence::NativeText) + ")";
    _action_redo->setStatusTip(tr("重做") + s);
    _action_redo->setToolTip(tr("撤销") + s);
    _action_redo->setShortcut(QKeySequence::Redo);
    _action_redo->setPriority(QAction::LowPriority);
    connect(document(), SIGNAL(redoAvailable(bool)),
            _action_redo, SLOT(setEnabled(bool)));
    connect(_action_redo, SIGNAL(triggered()), this, SLOT(redo()));
    _action_redo->setEnabled(false);

    // Cut
    assert(NULL == _action_cut);
    _action_cut = new QAction(
        QIcon::fromTheme("edit-cut", QIcon(":/markdown-textedit/cut")),
        tr("剪切(&X)"), this);
    s.clear();
    ks = QKeySequence::keyBindings(QKeySequence::Cut);
    if (!ks.empty())
        s = " (" + ks.at(0).toString(QKeySequence::NativeText) + ")";
    _action_cut->setStatusTip(tr("剪切到剪贴板") + s);
    _action_cut->setToolTip(tr("剪切到剪贴板") + s);
    _action_cut->setShortcut(QKeySequence::Cut);
    _action_cut->setPriority(QAction::LowPriority);
    connect(this, SIGNAL(copyAvailable(bool)), _action_cut, SLOT(setEnabled(bool)));
    connect(_action_cut, SIGNAL(triggered()), this, SLOT(cut()));
    _action_cut->setEnabled(false);

    // Copy
    assert(NULL == _action_copy);
    _action_copy = new QAction(
        QIcon::fromTheme("edit-copy", QIcon(":/markdown-textedit/copy")),
        tr("复制(&C)"), this);
    s.clear();
    ks = QKeySequence::keyBindings(QKeySequence::Copy);
    if (!ks.empty())
        s = " (" + ks.at(0).toString(QKeySequence::NativeText) + ")";
    _action_copy->setStatusTip(tr("复制到剪切板") + s);
    _action_copy->setToolTip(tr("复制到剪切板") + s);
    _action_copy->setShortcut(QKeySequence::Copy);
    _action_copy->setPriority(QAction::LowPriority);
    connect(this, SIGNAL(copyAvailable(bool)), _action_copy, SLOT(setEnabled(bool)));
    connect(_action_copy, SIGNAL(triggered()), this, SLOT(copy()));
    _action_copy->setEnabled(false);

    // Paste
    assert(NULL == _action_paste);
    _action_paste = new QAction(
        QIcon::fromTheme("edit-paste", QIcon(":/markdown-textedit/paste")),
        tr("粘贴(&V)"), this);
    s.clear();
    ks = QKeySequence::keyBindings(QKeySequence::Paste);
    if (!ks.empty())
        s = " (" + ks.at(0).toString(QKeySequence::NativeText) + ")";
    _action_paste->setStatusTip(tr("从剪贴板粘贴") + s);
    _action_paste->setToolTip(tr("从剪贴板粘贴") + s);
    _action_paste->setShortcut(QKeySequence::Paste);
    _action_paste->setPriority(QAction::LowPriority);
    connect(_action_paste, SIGNAL(triggered()), this, SLOT(paste()));
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        _action_paste->setEnabled(md->hasText());
#endif

    // Bold
    assert(NULL == _action_bold);
    _action_bold = new QAction(
        QIcon::fromTheme("format-text-bold", QIcon(":/markdown-textedit/bold")),
        tr("粗体(&B)"), this);
    s.clear();
    ks = QKeySequence::keyBindings(QKeySequence::Bold);
    if (!ks.empty())
        s = " (" + ks.at(0).toString(QKeySequence::NativeText) + ")";
    _action_bold->setStatusTip(tr("设置粗体") + s);
    _action_bold->setToolTip(tr("设置粗体") + s);
    _action_bold->setShortcut(QKeySequence::Bold);
    _action_bold->setPriority(QAction::LowPriority);
    QFont bold;
    bold.setBold(true);
    _action_bold->setFont(bold);
    connect(_action_bold, SIGNAL(triggered()), this, SLOT(text_bold()));

    // Italic
    assert(NULL == _action_italic);
    _action_italic = new QAction(
        QIcon::fromTheme("format-text-italic", QIcon(":/markdown-textedit/italic")),
        tr("斜体(&I)"), this);
    s.clear();
    ks = QKeySequence::keyBindings(QKeySequence::Italic);
    if (!ks.empty())
        s = " (" + ks.at(0).toString(QKeySequence::NativeText) + ")";
    _action_italic->setStatusTip(tr("设置斜体") + s);
    _action_italic->setToolTip(tr("设置斜体") + s);
    _action_italic->setShortcut(QKeySequence::Italic);
    _action_italic->setPriority(QAction::LowPriority);
    QFont italic;
    italic.setItalic(true);
    _action_italic->setFont(italic);
    connect(_action_italic, SIGNAL(triggered()), this, SLOT(text_italic()));

    // Strikthrough
    assert(NULL == _action_strikethrough);
    _action_strikethrough = new QAction(
        QIcon::fromTheme("format-text-strikethrough", QIcon(":/markdown-textedit/strikethrough")),
        tr("删除线(&S)"), this);
    QKeySequence k(Qt::CTRL + Qt::Key_T);
    s = " (" + k.toString(QKeySequence::NativeText) + ")";
    _action_strikethrough->setStatusTip(tr("设置删除线") + s);
    _action_strikethrough->setToolTip(tr("设置删除线") + s);
    _action_strikethrough->setShortcut(k);
    _action_strikethrough->setPriority(QAction::LowPriority);
    QFont strikethrough;
    strikethrough.setStrikeOut(true);
    _action_strikethrough->setFont(strikethrough);
    connect(_action_strikethrough, SIGNAL(triggered()), this, SLOT(text_strikethrough()));

    // Superscript
    assert(NULL == _action_superscript);
    _action_superscript = new QAction(QIcon(":/markdown-textedit/superscript"),
                                   tr("上标(&P)"), this);
    k = QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_P);
    s = " (" + k.toString(QKeySequence::NativeText) + ")";
    _action_superscript->setStatusTip(tr("设置上标") + s);;
    _action_superscript->setToolTip(tr("设置上标") + s);;
    _action_superscript->setShortcut(k);
    _action_superscript->setPriority(QAction::LowPriority);
    connect(_action_superscript, SIGNAL(triggered()), this, SLOT(text_superscript()));

    // Inline code
    assert(NULL == _action_inline_code);
    _action_inline_code = new QAction(QIcon(":/markdown-textedit/code"),
                                   tr("行内代码(&N)"), this);
    k = QKeySequence(Qt::CTRL + Qt::Key_N);
    s = " (" + k.toString(QKeySequence::NativeText) + ")";
    _action_inline_code->setStatusTip(tr("行内代码") + s);;
    _action_inline_code->setToolTip(tr("行内代码") + s);;
    _action_inline_code->setShortcut(k);
    _action_inline_code->setPriority(QAction::LowPriority);
    connect(_action_inline_code, SIGNAL(triggered()), this, SLOT(text_inline_code()));

    // Align center
    _action_align_center = new QAction(
        QIcon::fromTheme("format-justify-center", QIcon(":/markdown-textedit/align_center")),
        tr("居中对齐(&E)"), this);
    k = QKeySequence(Qt::CTRL + Qt::Key_E);
    s = " (" + k.toString(QKeySequence::NativeText) + ")";
    _action_align_center->setStatusTip(tr("居中对齐") + s);;
    _action_align_center->setToolTip(tr("居中对齐") + s);;
    _action_align_center->setShortcut(k);
    _action_align_center->setPriority(QAction::LowPriority);
    connect(_action_align_center, SIGNAL(triggered()), this, SLOT(text_align_center()));

    // Break line
    assert(NULL == _action_break_line);
    _action_break_line = new QAction(QIcon(":/markdown-textedit/key_return"),
                                  tr("换行(&R)"), this);
    k = QKeySequence(Qt::CTRL + Qt::Key_Enter);
    s = " (" + k.toString(QKeySequence::NativeText) + ")";
    _action_break_line->setStatusTip(tr("段落内换行") + s);;
    _action_break_line->setToolTip(tr("段落内换行") + s);;
    _action_break_line->setShortcut(k);
    _action_break_line->setPriority(QAction::LowPriority);
    connect(_action_break_line, SIGNAL(triggered()), this, SLOT(break_line()));

    // Quote
    assert(NULL == _action_quote);
    _action_quote = new QAction(QIcon(":/markdown-textedit/quote"), tr("引用(&Q)"), this);
    k = QKeySequence(Qt::CTRL + Qt::Key_R);
    s = " (" + k.toString(QKeySequence::NativeText) + ")";
    _action_quote->setStatusTip(tr("引用段落") + s);;
    _action_quote->setToolTip(tr("引用段落") + s);;
    _action_quote->setShortcut(k);
    _action_quote->setPriority(QAction::LowPriority);
    connect(_action_quote, SIGNAL(triggered()), this, SLOT(quote()));

    // Insert table
    assert(NULL == _action_insert_table);
    _action_insert_table = new QAction(QIcon(":/markdown-textedit/insert_table"),
                                    tr("插入表格(&T)"), this);
    k = QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_T);
    s = " (" + k.toString(QKeySequence::NativeText) + ")";
    _action_insert_table->setStatusTip(tr("插入表格") + s);;
    _action_insert_table->setToolTip(tr("插入表格") + s);;
    _action_insert_table->setShortcut(k);
    _action_insert_table->setPriority(QAction::LowPriority);
    connect(_action_insert_table, SIGNAL(triggered()), this, SLOT(insert_table()));

    // Insert image
    assert(NULL == _action_insert_image);
    _action_insert_image = new QAction(QIcon(":/markdown-textedit/insert_image"),
                                    tr("插入图片(&P)"), this);
    k = QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_P);
    s = " (" + k.toString(QKeySequence::NativeText) + ")";
    _action_insert_image->setStatusTip(tr("插入图片") + s);;
    _action_insert_image->setToolTip(tr("插入图片") + s);;
    _action_insert_image->setShortcut(k);
    _action_insert_image->setPriority(QAction::LowPriority);
    connect(_action_insert_image, SIGNAL(triggered()), this, SLOT(insert_image()));

    // Insert hyperlink
    assert(NULL == _action_insert_hyperlink);
    _action_insert_hyperlink = new QAction(QIcon(":/markdown-textedit/hyperlink"),
                                        tr("插入超链接(&L)"), this);
    k = QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L);
    s = " (" + k.toString(QKeySequence::NativeText) + ")";
    _action_insert_hyperlink->setStatusTip(tr("插入超链接") + s);;
    _action_insert_hyperlink->setToolTip(tr("插入超链接") + s);;
    _action_insert_hyperlink->setShortcut(k);
    _action_insert_hyperlink->setPriority(QAction::LowPriority);
    connect(_action_insert_hyperlink, SIGNAL(triggered()), this, SLOT(insert_hyperlink()));
}

void MarkdownTextEdit::init_popup_menu()
{
    assert(NULL == _popup_menu);
    _popup_menu = new QMenu(this);

    _popup_menu->addAction(_action_undo);
    _popup_menu->addAction(_action_redo);
    _popup_menu->addSeparator(); // 分割线

    _popup_menu->addAction(_action_cut);
    _popup_menu->addAction(_action_copy);
    _popup_menu->addAction(_action_paste);
    _popup_menu->addSeparator();

    _popup_menu->addAction(_action_bold);
    _popup_menu->addAction(_action_italic);
    _popup_menu->addAction(_action_strikethrough);
    _popup_menu->addAction(_action_superscript);
    _popup_menu->addAction(_action_inline_code);
    _popup_menu->addAction(_action_align_center);
    _popup_menu->addAction(_action_break_line);
    _popup_menu->addAction(_action_quote);
    _popup_menu->addAction(_action_insert_table);
    _popup_menu->addAction(_action_insert_image);
    _popup_menu->addAction(_action_insert_hyperlink);
}

/**
 * Leave a little space on the right side if the document is too long, so
 * that the search buttons don't get visually blocked by the scroll bar
 */
void MarkdownTextEdit::adjust_right_margin()
{
    QMargins margins = layout()->contentsMargins();
    int rightMargin = document()->size().height() >
                      viewport()->size().height() ? 24 : 0;
    margins.setRight(rightMargin);
    layout()->setContentsMargins(margins);
}

void MarkdownTextEdit::paintEvent(QPaintEvent *e)
{
    QPlainTextEdit::paintEvent(e);
    
    // Draw line end markers if enabled
    if (_show_hard_line_breaks)
        draw_line_end_marker(e);
    
    // Draw column ruler
    if (_ruler_enabled)
        draw_ruler(e);
}

void MarkdownTextEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    // update line number area
    QRect cr = contentsRect();
    _line_Number_area->setGeometry(QStyle::visualRect(layoutDirection(), cr,
        QRect(cr.left(), cr.top(), line_number_area_width(), cr.height())));
}

void MarkdownTextEdit::draw_line_end_marker(QPaintEvent *e)
{
    QPainter painter(viewport());

    int leftMargin = qRound(fontMetrics().width(" ") / 2.0);
    int lineEndCharWidth = fontMetrics().width("\u00B6");
    int fontHeight = fontMetrics().height();

    QTextBlock block = firstVisibleBlock();
    while (block.isValid())
    {
        QRectF blockGeometry = blockBoundingGeometry(block).translated(contentOffset());
        if (blockGeometry.top() > e->rect().bottom())
            break;

        if (block.isVisible() && blockGeometry.toRect().intersects(e->rect()))
        {
            QString text = block.text();
            if (text.endsWith("  "))
            {
                painter.drawText(blockGeometry.left() + fontMetrics().width(text) + leftMargin,
                                 blockGeometry.top(),
                                 lineEndCharWidth,
                                 fontHeight,
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 "\u00B6");
            }
        }

        block = block.next();
    }
}

void MarkdownTextEdit::draw_ruler(QPaintEvent *e)
{
    const QRect rect = e->rect();
    const QFont font = currentCharFormat().font();

    // calculate vertical offset corresponding given
    // column margin in font metrics
    int verticalOffset = qRound(QFontMetricsF(font).averageCharWidth() * _ruler_pos)
            + contentOffset().x()
            + document()->documentMargin();

    // draw a ruler with color invert to background color (better readability)
    // and with 50% opacity
    QPainter p(viewport());
    p.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
    p.setPen(QColor(0xff, 0xff, 0xff));
    p.setOpacity(0.5);

    p.drawLine(verticalOffset, rect.top(), verticalOffset, rect.bottom());
}

int MarkdownTextEdit::line_number_area_width()
{
    int digits = 2;
    int max = qMax(1, blockCount());
    while (max >= 100)
    {
        max /= 10;
        ++digits;
    }

    QFont font = _line_Number_area->font();
    const QFontMetrics linefmt(font);

    int space = 10 + linefmt.width(QLatin1Char('9')) * digits;
    return space;
}

void MarkdownTextEdit::line_number_area_paint_event(QPaintEvent *event)
{
    QPainter painter(_line_Number_area);

    int selStart = textCursor().selectionStart();
    int selEnd = textCursor().selectionEnd();

    QPalette palette = _line_Number_area->palette();
    palette.setCurrentColorGroup(QPalette::Active);

    painter.fillRect(event->rect(), palette.color(QPalette::Background));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
    qreal bottom = top;

    while (block.isValid() && top <= event->rect().bottom())
    {
        top = bottom;

        const qreal height = blockBoundingRect(block).height();
        bottom = top + height;

        if (block.isVisible() && bottom >= event->rect().top())
        {
            painter.setPen(palette.windowText().color());

            bool selected = (
                                (selStart < block.position() + block.length() && selEnd > block.position())
                                || (selStart == selEnd && selStart == block.position())
                            );

            if (selected)
            {
                painter.save();
                painter.setPen(palette.highlight().color());
            }

            const QString number = QString::number(blockNumber + 1);
            painter.drawText(0, top, _line_Number_area->width() - 4, height, Qt::AlignRight, number);

            if (selected)
                painter.restore();
        }

        block = block.next();
        ++blockNumber;
    }
}

void MarkdownTextEdit::update_line_number_area_width(int new_block_count)
{
    setViewportMargins(line_number_area_width(), 0, 0, 0);
}

void MarkdownTextEdit::update_line_number_area(const QRect &rect, int dy)
{
    if (dy)
        _line_Number_area->scroll(0, dy);
    else
        _line_Number_area->update(0, rect.y(), _line_Number_area->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        update_line_number_area_width(0);
}

void MarkdownTextEdit::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->matches(QKeySequence::Save))
    {
        // ctrl-s 保存
        emit trigger_saving();
        return;
    }

    if ((keyEvent->key() == Qt::Key_Escape) && _search_widget->isVisible())
    {
        _search_widget->deactivate();
        return;
    }
    else if ((keyEvent->key() == Qt::Key_Tab) ||
             (keyEvent->key() == Qt::Key_Backtab))
    {
        // handle entered tab and reverse tab keys
        handle_tab_entered(keyEvent->key() == Qt::Key_Backtab);
        return;
    }
    else if ((keyEvent->key() == Qt::Key_F) &&
             keyEvent->modifiers().testFlag(Qt::ControlModifier))
    {
        _search_widget->activate();
        return;
    }
    else if ((keyEvent->key() == Qt::Key_R) &&
             keyEvent->modifiers().testFlag(Qt::ControlModifier))
    {
        _search_widget->activateReplace();
        return;
    }
    else if ((keyEvent->key() == Qt::Key_Down) &&
             keyEvent->modifiers().testFlag(Qt::ControlModifier) &&
             keyEvent->modifiers().testFlag(Qt::AltModifier))
    {
        // duplicate text with `Ctrl + Alt + Down`
        duplicate_text();
        return;
    }
    else if ((keyEvent->key() == Qt::Key_Down) &&
            keyEvent->modifiers().testFlag(Qt::NoModifier))
    {
        // if you are in the last line and press cursor down the cursor will
        // jump to the end of the line
        QTextCursor c = textCursor();
        if (c.position() >= document()->lastBlock().position())
        {
            c.movePosition(QTextCursor::EndOfLine);
            setTextCursor(c);
            return;
        }
    }
    else if ((keyEvent->key() == Qt::Key_Up) &&
               keyEvent->modifiers().testFlag(Qt::NoModifier))
    {
        // if you are in the first line and press cursor up the cursor will
        // jump to the start of the line
        QTextCursor c = textCursor();
        QTextBlock block = document()->firstBlock();
        int endOfFirstLinePos = block.position() + block.length();

        if (c.position() <= endOfFirstLinePos)
        {
            c.movePosition(QTextCursor::StartOfLine);
            setTextCursor(c);
            return;
        }
    }
    else if (keyEvent->key() == Qt::Key_Return)
    {
        if (handle_return_entered())
            return;
    }
    else if ((keyEvent->key() == Qt::Key_F3))
    {
        _search_widget->doSearch(!keyEvent->modifiers().testFlag(Qt::ShiftModifier));
        return;
    }
    
    QPlainTextEdit::keyPressEvent(keyEvent);
}

void MarkdownTextEdit::mouseMoveEvent(QMouseEvent *mouseEvent)
{
    // Toggle cursor when control key has been pressed or released
    viewport()->setCursor(mouseEvent->modifiers().testFlag(Qt::ControlModifier) ?
                              Qt::PointingHandCursor : Qt::IBeamCursor);

    QPlainTextEdit::mouseMoveEvent(mouseEvent);
}

void MarkdownTextEdit::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
    // track `Ctrl + Click` in the text edit
    if ((mouseEvent->button() == Qt::LeftButton) &&
        (QGuiApplication::keyboardModifiers() == Qt::ExtraButton24))
    {
        // open the link (if any) at the current position
        // in the noteTextEdit
        open_link_at_cursor_position();
        return;
    }
    
    QPlainTextEdit::mouseReleaseEvent(mouseEvent);
}

void MarkdownTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
   if (NULL == _popup_menu)
       return;
   _popup_menu->exec(e->globalPos());
}

/**
 * Increases (or decreases) the indention of the selected text
 * (if there is a text selected) in the noteTextEdit
 * @return
 */
bool MarkdownTextEdit::increase_selected_text_indention(bool reverse)
{
    QTextCursor c = this->textCursor();
    QString selectedText = c.selectedText();

    if (selectedText != "")
    {
        // we need this strange newline character we are getting in the
        // selected text for newlines
        QString newLine = QString::fromUtf8(QByteArray::fromHex("e280a9"));
        QString newText;

        if (reverse)
        {
            // un-indent text

            // remove strange newline characters
            newText = selectedText.replace(QRegularExpression(newLine + "[\\t ]"), "\n");

            // remove leading \t or space
            newText.remove(QRegularExpression("^[\\t ]"));
        }
        else
        {
            // indent text
            newText = selectedText.replace(newLine, "\n\t").prepend("\t");

            // remove trailing \t
            newText.replace(QRegularExpression("\\t$"), "");
        }

        // insert the new text
        c.insertText(newText);

        // update the selection to the new text
        c.setPosition(c.position() - newText.size(), QTextCursor::KeepAnchor);
        this->setTextCursor(c);

        return true;
    }
    else if (reverse)
    {
        // if nothing was selected but we want to reverse the indention check
        // if there is a \t in front or after the cursor and remove it if so
        int pos = c.position();
        // get character in front of cursor
        c.setPosition(pos - 1, QTextCursor::KeepAnchor);

        // check for \t or space in front of cursor
        QRegularExpression re("[\\t ]");
        QRegularExpressionMatch match = re.match(c.selectedText());

        if (!match.hasMatch())
        {
            // (select to) check for \t or space after the cursor
            c.setPosition(pos);
            c.setPosition(pos + 1, QTextCursor::KeepAnchor);
        }

        match = re.match(c.selectedText());

        if (match.hasMatch())
            c.removeSelectedText();

        return true;
    }

    return false;
}

/**
 * @brief Opens the link (if any) at the current cursor position
 */
bool MarkdownTextEdit::open_link_at_cursor_position()
{
    QTextCursor c = this->textCursor();
    int clickedPosition = c.position();

    // select the text in the clicked block and find out on
    // which position we clicked
    c.movePosition(QTextCursor::StartOfBlock);
    int positionFromStart = clickedPosition - c.position();
    c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    QString selectedText = c.selectedText();

    // find out which url in the selected text was clicked
    QString urlString = get_markdown_url_at_position(selectedText, positionFromStart);
    QUrl url = QUrl(urlString);
    bool isRelativeFileUrl = urlString.startsWith("file://..");

    if (url.isValid() || isRelativeFileUrl)
    {
        qDebug() << __func__ << " - 'emit urlClicked( urlString )': " << urlString;

        emit url_clicked(urlString);

        // ignore some schemata
        if (!(_ignored_click_url_schemata.contains(url.scheme()) || isRelativeFileUrl))
        {
            // open the url
            open_url(urlString);
        }

        return true;
    }

    return false;
}

/**
 * Handles clicked urls
 *
 * examples:
 * - <http://www.qownnotes.org> opens the webpage
 * - <file:///path/to/my/file/QOwnNotes.pdf> opens the file
 *   "/path/to/my/file/QOwnNotes.pdf" if the operating system supports that
 *  handler
 */
void MarkdownTextEdit::open_url(QString urlString)
{
    qDebug() << "MarkdownTextEdit " << __func__ << " - 'urlString': " << urlString;

    QDesktopServices::openUrl(QUrl(urlString));
}

/**
 * @brief Returns the highlighter instance
 * @return
 */
MarkdownHighlighter* MarkdownTextEdit::highlighter()
{
    return _highlighter;
}

void MarkdownTextEdit::load_style_from_stylesheet(const QString &file_name)
{
    QFile f(file_name);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream ts(&f);
    QString input = ts.readAll();

    // parse the stylesheet
    PegMarkdownHighlight::StyleParser parser(input);
    QVector<PegMarkdownHighlight::HighlightingStyle> styles = parser.highlightingStyles(this->font());

    // set new style & rehighlight markdown document
    _highlighter->set_styles(styles);
    _highlighter->rehighlight();

    // update color palette
    setPalette(parser.editorPalette());
    viewport()->setPalette(palette());
}

/**
 * @brief Returns the searchWidget instance
 * @return
 */
TextEditSearchWidget *MarkdownTextEdit::search_widget()
{
    return _search_widget;
}

/**
 * @brief Sets url schemata that will be ignored when clicked on
 * @param urlSchemes
 */
void MarkdownTextEdit::set_ignored_click_url_schemata(QStringList ignoredUrlSchemata)
{
    _ignored_click_url_schemata = ignoredUrlSchemata;
}

/**
 * @brief Returns a map of parsed markdown urls with their link texts as key
 *
 * @param text
 * @return parsed urls
 */
QMap<QString, QString> MarkdownTextEdit::parse_markdown_urls_from_text(QString text)
{
    QMap<QString, QString> urlMap;

    // match urls like this: [this url](http://mylink)
    QRegularExpression re("(\\[.*?\\]\\((.+?://.+?)\\))");
    QRegularExpressionMatchIterator i = re.globalMatch(text);
    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        QString linkText = match.captured(1);
        QString url = match.captured(2);
        urlMap[linkText] = url;
    }

    // match urls like this: <http://mylink>
    re = QRegularExpression("(<(.+?://.+?)>)");
    i = re.globalMatch(text);
    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        QString linkText = match.captured(1);
        QString url = match.captured(2);
        urlMap[linkText] = url;
    }

    return urlMap;
}

/**
 * @brief Returns the markdown url at position
 * @param text
 * @param position
 * @return url string
 */
QString MarkdownTextEdit::get_markdown_url_at_position(QString text, int position)
{
    QString url;

    // get a map of parsed markdown urls with their link texts as key
    QMap<QString, QString> urlMap = parse_markdown_urls_from_text(text);

    QMapIterator<QString, QString> i(urlMap);
    while (i.hasNext())
    {
        i.next();
        QString linkText = i.key();
        QString urlString = i.value();

        int foundPositionStart = text.indexOf(linkText);

        if (foundPositionStart >= 0)
        {
            // calculate end position of found linkText
            int foundPositionEnd = foundPositionStart + linkText.size();

            // check if position is in found string range
            if ((position >= foundPositionStart) &&
                (position <= foundPositionEnd))
            {
                url = urlString;
            }
        }
    }

    return url;
}

/**
 * @brief Duplicates the text in the text edit
 */
void MarkdownTextEdit::duplicate_text()
{
    QTextCursor c = this->textCursor();
    QString selectedText = c.selectedText();

    // duplicate line if no text was selected
    if (selectedText == "")
    {
        int position = c.position();

        // select the whole line
        c.movePosition(QTextCursor::StartOfLine);
        c.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

        int positionDiff = c.position() - position;
        selectedText = "\n" + c.selectedText();

        // insert text with new line at end of the selected line
        c.setPosition(c.selectionEnd());
        c.insertText(selectedText);

        // set the position to same position it was in the duplicated line
        c.setPosition(c.position() - positionDiff);
    }
    else
    {
        // duplicate selected text
        c.setPosition(c.selectionEnd());
        int selectionStart = c.position();

        // insert selected text
        c.insertText(selectedText);
        int selectionEnd = c.position();

        // select the inserted text
        c.setPosition(selectionStart);
        c.setPosition(selectionEnd, QTextCursor::KeepAnchor);
    }

    this->setTextCursor(c);
}

void MarkdownTextEdit::set_content(const QString & text)
{
    QPlainTextEdit::setPlainText(text);
    adjust_right_margin();
}

/**
 * Uses an other widget as parent for the search widget
 */
void MarkdownTextEdit::init_search_frame(QWidget *searchFrame)
{
    _search_frame = searchFrame;

    // remove the search widget from our layout
    layout()->removeWidget(_search_widget);

    QLayout *layout = _search_frame->layout();

    // create a grid layout for the frame and add the search widget to it
    if (layout == NULL)
    {
        layout = new QVBoxLayout();
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
    }

    layout->addWidget(_search_widget);
    _search_frame->setLayout(layout);
}

/**
 * Hides the text edit and the search widget
 */
void MarkdownTextEdit::hide()
{
    _search_widget->hide();
    QWidget::hide();
}

/**
 * Handles an entered return key
 */
bool MarkdownTextEdit::handle_return_entered()
{
    QTextCursor c = this->textCursor();
    int position = c.position();

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    QString currentLineText = c.selectedText();

    // if return is pressed and there is just a list symbol then we want to
    // remove the list symbol
    QRegularExpression re("^\\s*[+\\-\\*]\\s*$");
    QRegularExpressionMatchIterator i = re.globalMatch(currentLineText);
    if (i.hasNext())
    {
        c.removeSelectedText();
        return true;
    }

    // if the current line starts with a list character (possibly after
    // whitespaces) add the whitespaces at the next line too
    re = QRegularExpression("^(\\s*)([+\\-\\*])(\\s?)");
    i = re.globalMatch(currentLineText);
    if (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        QString whitespaces = match.captured(1);
        QString listCharacter = match.captured(2);
        QString whitespaceCharacter = match.captured(3);

        c.setPosition(position);
        c.insertText("\n" + whitespaces + listCharacter + whitespaceCharacter);

        // scroll to the cursor if we are at the bottom of the document
        ensureCursorVisible();
        return true;
    }

    return false;
}

/**
 * Handles entered tab or reverse tab keys
 */
bool MarkdownTextEdit::handle_tab_entered(bool reverse)
{
    QTextCursor c = this->textCursor();

    // only check for lists if we haven't a text selected
    if (c.selectedText().isEmpty())
    {
        c.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        QString currentLineText = c.selectedText();

        // check if we want to indent or un-indent a list
        QRegularExpression re("^(\\s*)([+\\-\\*])(\\s?)$");
        QRegularExpressionMatchIterator i = re.globalMatch(currentLineText);

        if (i.hasNext())
        {
            QRegularExpressionMatch match = i.next();
            QString whitespaces = match.captured(1);
            QString listCharacter = match.captured(2);
            QString whitespaceCharacter = match.captured(3);

            // add or remove one tabulator key
            if (reverse)
            {
                whitespaces.chop(1);
            }
            else
            {
                whitespaces += "\t";
            }

            c.insertText(whitespaces + listCharacter + whitespaceCharacter);
            return true;
        }
    }

    // check if we want to intent the whole text
    return increase_selected_text_indention(reverse);
}

void MarkdownTextEdit::text_bold()
{
    wrap_tags("**", "**");
}

void MarkdownTextEdit::text_italic()
{
    wrap_tags("*", "*");
}

void MarkdownTextEdit::text_strikethrough()
{
    wrap_tags("~~", "~~");
}

void MarkdownTextEdit::text_superscript()
{
    wrap_tags("^", "");
}

void MarkdownTextEdit::text_inline_code()
{
    wrap_tags("`", "`");
}

void MarkdownTextEdit::text_align_center()
{
    wrap_tags("->", "<-");
}

void MarkdownTextEdit::wrap_tags(const QString& tag1, const QString& tag2)
{
    QTextCursor cursor = textCursor();
    QString text = tag1;
    if (cursor.hasSelection())
        text += cursor.selectedText();
    text += tag2;
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, tag2.length());
    setTextCursor(cursor);
}

void MarkdownTextEdit::break_line()
{
    QTextCursor cursor = textCursor();
    cursor.insertText("  \n");
    setTextCursor(cursor);
}

void MarkdownTextEdit::quote()
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection())
    {
        QString text = cursor.selectedText();
        QStringList lines = text.split((QChar)8233); // XXX 奇怪的换行符(至少mac下是这样的)
        text = "\n> \n";
        foreach (QString line, lines)
        {
            text += "> ";
            text += line;
            text += "\n";
        }
        text += "> \n";
        cursor.insertText(text);
    }
    else
    {
        cursor.insertText("\n>\n> \n");
    }
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
    setTextCursor(cursor);
}

void MarkdownTextEdit::insert_table()
{
    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    cursor.insertText("\n\n"
                      "|A|B|C|\n"
                      "|-|-|-|\n"
                      "|a|b|c|\n"
                      "|d|e|f|\n\n");
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
    setTextCursor(cursor);
}

void MarkdownTextEdit::insert_image()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    const QString format("![%1](http://refer \"title\")");
    QString text("text");
    if (cursor.hasSelection())
        text = cursor.selectedText();
    cursor.insertText(format.arg(text));

    cursor.endEditBlock();
}

void MarkdownTextEdit::insert_hyperlink()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    const QString format("[%1](http://refer)");
    QString text("text");
    if (cursor.hasSelection())
        text = cursor.selectedText();
    cursor.insertText(format.arg(text));

    cursor.endEditBlock();

}

}
