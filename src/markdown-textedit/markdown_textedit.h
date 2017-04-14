
#ifndef ___HEADFILE_A1034E6D_6AD9_4C26_A96D_D392A894ED5C_
#define ___HEADFILE_A1034E6D_6AD9_4C26_A96D_D392A894ED5C_

#include <QPlainTextEdit>

#include "markdown_textedit_config.h"
#include "textedit_search_widget.h"

namespace mdtextedit
{

class MarkdownHighlighter;


class MDTE_API MarkdownTextEdit : public QPlainTextEdit
{
    Q_OBJECT

protected:
    MarkdownHighlighter *_highlighter = NULL;
    QStringList _ignored_click_url_schemata;
    TextEditSearchWidget *_search_widget = NULL;
    QWidget *_search_frame = NULL;
    QWidget *_line_Number_area = NULL;

    QAction *_action_undo = NULL, *_action_redo = NULL,
        *_action_cut = NULL, *_action_copy = NULL, *_action_paste = NULL,
        *_action_bold = NULL, *_action_italic = NULL, *_action_strikethrough = NULL,
        *_action_superscript = NULL,
        *_action_inline_code = NULL,
        *_action_align_center = NULL,
        *_action_break_line = NULL,
        *_action_quote = NULL,
        *_action_insert_table = NULL,
        *_action_insert_image = NULL, *_action_insert_hyperlink = NULL;
    QMenu *_popup_menu = NULL;

    bool _show_hard_line_breaks = true;
    bool _ruler_enabled = true;
    int _ruler_pos = 80;

private:
    void init_actions();
    void init_popup_menu();

    void draw_line_end_marker(QPaintEvent *e);
    void draw_ruler(QPaintEvent *e);

public:
    explicit MarkdownTextEdit(QWidget *parent = 0);

    MarkdownHighlighter *highlighter();
    void load_style_from_stylesheet(const QString& file_path);
    TextEditSearchWidget *search_widget();
    void set_ignored_click_url_schemata(QStringList ignoredUrlSchemata);
    QString get_markdown_url_at_position(QString text, int position);
    void init_search_frame(QWidget *searchFrame);

    virtual void open_url(QString urlString);
    
signals:
    // 保存动作被触发
    void trigger_saving();

public:
    int line_number_area_width();
    void line_number_area_paint_event(QPaintEvent *event);

public slots:
    void duplicate_text();
    void set_content(const QString & text);
    void adjust_right_margin();
    void hide();
    bool open_link_at_cursor_position();

    void text_bold();
    void text_italic();
    void text_strikethrough();
    void text_superscript();
    void text_inline_code();
    void text_align_center();
    void break_line();
    void quote();
    void insert_table();
    void insert_image();
    void insert_hyperlink();
    
private slots:
    void update_line_number_area_width(int new_block_count);
    void update_line_number_area(const QRect &rect, int dy);

protected:
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void contextMenuEvent(QContextMenuEvent *e) override;
    virtual void paintEvent(QPaintEvent *e) override;
    virtual void resizeEvent(QResizeEvent *event) override;

    bool increase_selected_text_indention(bool reverse);
    bool handle_tab_entered(bool reverse);
    QMap<QString, QString> parse_markdown_urls_from_text(QString text);
    bool handle_return_entered();
    void wrap_tags(const QString& tag1, const QString& tag2);

signals:
    void url_clicked(QString url);
};

}

#endif
