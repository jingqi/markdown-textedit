
#include <QDebug>
#include <QFile>
#include <QTextDocument>
#include <QTextLayout>


#ifdef __cplusplus
extern "C" {
#endif
#   include <pmh_parser.h>
#   include <pmh-adapter/definitions.h>
#ifdef __cplusplus
}
#endif

#include "markdown_highlighter.h"

using PegMarkdownHighlight::HighlightingStyle;

namespace organic
{

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document), _worker_thread(new HighlightWorkerThread(this))
{
    set_default_styles();

    connect(_worker_thread, SIGNAL(result_ready(pmh_element**, unsigned long)),
            this, SLOT(result_ready(pmh_element**, unsigned long)));

    _worker_thread->start();
}

MarkdownHighlighter::~MarkdownHighlighter()
{
    // stop background worker thread
    _worker_thread->enqueue(QString());
    _worker_thread->wait();
    delete _worker_thread;
}

void MarkdownHighlighter::reset()
{
    _previous_text.clear();
}

void MarkdownHighlighter::set_styles(const QVector<PegMarkdownHighlight::HighlightingStyle> &styles)
{
    _highlighting_styles = styles;
    reset();
}

// The initial define causes an error with Visual Studio 2015:
// "error C4576: a parenthesized type followed by an initializer list is
// a non-standard explicit type conversion syntax"
// The replacement works, probably also on other platforms (to be tested)
//#define STY(type, format) styles->append((HighlightingStyle){type, format})
#define STY(type, format) styles.append({type, format})

void MarkdownHighlighter::set_default_styles(int default_font_size)
{
    QVector<HighlightingStyle> styles;

    QTextCharFormat headers;
    headers.setForeground(QBrush(QColor(0, 49, 110)));
    headers.setBackground(QBrush(QColor(230, 230, 240)));
    headers.setFontWeight(QFont::Bold);
    headers.setFontPointSize(default_font_size * 1.2);
    STY(pmh_H1, headers);

    headers.setFontPointSize(default_font_size * 1.1);
    STY(pmh_H2, headers);

    headers.setFontPointSize(default_font_size);
    STY(pmh_H3, headers);
    STY(pmh_H4, headers);
    STY(pmh_H5, headers);
    STY(pmh_H6, headers);

    QTextCharFormat hrule;
    hrule.setForeground(QBrush(Qt::darkGray));
    hrule.setBackground(QBrush(Qt::lightGray));
    STY(pmh_HRULE, hrule);

    /* <ul> */
    QTextCharFormat list;
    list.setForeground(QBrush(QColor(163, 0, 123)));
    STY(pmh_LIST_BULLET, list);
    STY(pmh_LIST_ENUMERATOR, list);

    /* <a href> */
    QTextCharFormat link;
    link.setForeground(QBrush(QColor(255, 128, 0)));
    link.setBackground(QBrush(QColor(255, 233, 211)));
    STY(pmh_LINK, link);
    STY(pmh_AUTO_LINK_URL, link);
    STY(pmh_AUTO_LINK_EMAIL, link);

    /* <img> */
    QTextCharFormat image;
    image.setForeground(QBrush(QColor(0, 191, 0)));
    image.setBackground(QBrush(QColor(228, 255, 228)));
    STY(pmh_IMAGE, image);

    QTextCharFormat ref;
    ref.setForeground(QBrush(QColor(213, 178, 178)));
    STY(pmh_REFERENCE, ref);

    /* <pre> */
    QTextCharFormat code;
    QFont codeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    code.setFont(codeFont);
    code.setForeground(QBrush(Qt::darkGreen));
    code.setBackground(QBrush(QColor(217, 231, 217)));
    STY(pmh_CODE, code);
    STY(pmh_VERBATIM, code);

    /* <em> */
    QTextCharFormat emph;
    emph.setForeground(QBrush(QColor(0, 87, 174)));
    emph.setFontItalic(true);
    STY(pmh_EMPH, emph);

    /* <strong> */
    QTextCharFormat strong;
    strong.setForeground(QBrush(QColor(0, 66, 138)));
    strong.setFontWeight(QFont::Bold);
    STY(pmh_STRONG, strong);

    QTextCharFormat comment;
    comment.setForeground(QBrush(Qt::gray));
    STY(pmh_COMMENT, comment);

    QTextCharFormat blockquote;
    blockquote.setForeground(QBrush(Qt::darkRed));
    STY(pmh_BLOCKQUOTE, blockquote);

    QTextCharFormat html;
    html.setForeground(QBrush(QColor(0x4f, 0xa1, 0xd6)));
    STY(pmh_HTML, html);

    QTextCharFormat htmlentity;
    htmlentity.setForeground(QBrush(QColor(0x6c, 0x71, 0xc4)));
    STY(pmh_HTML_ENTITY, htmlentity);

    QTextCharFormat htmlblock;
    htmlblock.setForeground(QBrush(QColor(0x1f, 0x57, 0x45)));
    STY(pmh_HTMLBLOCK, htmlentity);

    QTextCharFormat note;
    note.setForeground(QBrush(QColor(0x8f, 0x26, 0x54)));
    STY(pmh_NOTE, note);

    set_styles(styles);
}

void MarkdownHighlighter::set_yaml_header_support_enabled(bool enabled)
{
    _yaml_header_support_enabled = enabled;
}

void MarkdownHighlighter::highlightBlock(const QString &textBlock)
{
    if (document()->isEmpty())
        return;

    QString text = document()->toPlainText();

    // document changed since last call?
    if (text == _previous_text)
        return;

    // cut YAML headers
    QString actualText;
    unsigned long offset = 0;
    if (_yaml_header_support_enabled)
    {
        // TODO fix this
        // YamlHeaderChecker checker(text);
        // actualText = checker.body();
        // offset = checker.bodyOffset();
        actualText = text;
    }
    else
    {
        actualText = text;
    }

    _worker_thread->enqueue(actualText, offset);

    _previous_text = text;
}

void MarkdownHighlighter::apply_format(unsigned long pos, unsigned long end,
                                      QTextCharFormat format, bool merge)
{
    // The QTextDocument contains an additional single paragraph separator (unicode 0x2029).
    // https://bugreports.qt-project.org/browse/QTBUG-4841
    unsigned long max_offset = document()->characterCount() - 1;

    if (end <= pos || max_offset < pos)
        return;

    if (max_offset < end)
        end = max_offset;

    // "The QTextLayout object can only be modified from the
    // documentChanged implementation of a QAbstractTextDocumentLayout
    // subclass. Any changes applied from the outside cause undefined
    // behavior." -- we are breaking this rule here. There might be
    // a better (more correct) way to do this.

    int startBlockNum = document()->findBlock(pos).blockNumber();
    int endBlockNum = document()->findBlock(end).blockNumber();
    for (int j = startBlockNum; j <= endBlockNum; j++)
    {
        QTextBlock block = document()->findBlockByNumber(j);

        QTextLayout *layout = block.layout();
        int blockpos = block.position();
        QTextLayout::FormatRange r;
        r.format = format;
        QList<QTextLayout::FormatRange> list;
        if (merge)
            list = layout->additionalFormats();

        if (j == startBlockNum)
        {
            r.start = pos - blockpos;
            r.length = (startBlockNum == endBlockNum) ? end - pos : block.length() - r.start;
        }
        else if (j == endBlockNum)
        {
            r.start = 0;
            r.length = end - blockpos;
        }
        else
        {
            r.start = 0;
            r.length = block.length();
        }

        list.append(r);
        layout->setAdditionalFormats(list);
    }
}

void MarkdownHighlighter::result_ready(pmh_element **elements, unsigned long base_offset)
{
    if (!elements)
    {
        qDebug() << "elements is null";
        return;
    }

    // clear any format before base_offset
    if (base_offset > 0)
        apply_format(0, base_offset - 1, QTextCharFormat(), false);

    // apply highlight results
    for (int i = 0; i < _highlighting_styles.size(); i++)
    {
        HighlightingStyle style = _highlighting_styles.at(i);
        pmh_element *elem_cursor = elements[style.type];
        while (elem_cursor != NULL)
        {
            unsigned long pos = elem_cursor->pos + base_offset;
            unsigned long end = elem_cursor->end + base_offset;

            QTextCharFormat format = style.format;
            if (/*_makeLinksClickable
                &&*/ (elem_cursor->type == pmh_LINK
                    || elem_cursor->type == pmh_AUTO_LINK_URL
                    || elem_cursor->type == pmh_AUTO_LINK_EMAIL
                    || elem_cursor->type == pmh_REFERENCE)
                && elem_cursor->address != NULL)
            {
                QString address(elem_cursor->address);
                if (elem_cursor->type == pmh_AUTO_LINK_EMAIL && !address.startsWith("mailto:"))
                    address = "mailto:" + address;
                format.setAnchor(true);
                format.setAnchorHref(address);
                format.setToolTip(address);
            }
            apply_format(pos, end, format, true);

            elem_cursor = elem_cursor->next;
        }
    }

    // mark complete document as dirty
    document()->markContentsDirty(0, document()->characterCount());

    // free highlighting elements
    ::pmh_free_elements(elements);
}

}
