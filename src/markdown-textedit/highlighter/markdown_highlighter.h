
#ifndef ___HEADFILE_ABB9D205_6349_40DB_AB8A_29D2E8000DE6_
#define ___HEADFILE_ABB9D205_6349_40DB_AB8A_29D2E8000DE6_

#include <QSyntaxHighlighter>

#include <pmh_definitions.h>
#include <pmh-adapter/definitions.h>

#include "highlight_worker_thread.h"

namespace mdtextedit
{

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

private:
    HighlightWorkerThread *_worker_thread = NULL;
    QVector<PegMarkdownHighlight::HighlightingStyle> _highlighting_styles;
    QString _previous_text;
    bool _yaml_header_support_enabled = false;

public:
    MarkdownHighlighter(QTextDocument *document);
    ~MarkdownHighlighter();

    void reset();
    void set_default_styles(int default_font_size = 12);
    void set_styles(const QVector<PegMarkdownHighlight::HighlightingStyle> &styles);
    void set_spelling_check_enabled(bool enabled);
    void set_yaml_header_support_enabled(bool enabled);

protected:
    virtual void highlightBlock(const QString &textBlock) override;

private slots:
    void result_ready(pmh_element **elements, unsigned long base_offset);

private:
    void apply_format(unsigned long pos, unsigned long end, QTextCharFormat format, bool merge);
    void check_spelling(const QString &textBlock);

};

}

#endif
