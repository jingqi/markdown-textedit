/*
 * Copyright (C) 2016 Patrizio Bekerle -- http://www.bekerle.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 */

#ifndef ___HEADFILE_33BE0D68_E546_4776_A383_CBF0E518B9D8_
#define ___HEADFILE_33BE0D68_E546_4776_A383_CBF0E518B9D8_

#include <QWidget>
#include <QPlainTextEdit>


namespace Ui
{
class TextEditSearchWidget;
}

namespace organic
{

class TextEditSearchWidget : public QWidget
{
    Q_OBJECT

private:
    Ui::TextEditSearchWidget *_ui = NULL;

protected:
    QPlainTextEdit *_textedit;
    bool eventFilter(QObject *obj, QEvent *event);

public:
    enum SearchMode
    {
        PlainTextMode,
        WholeWordsMode,
        RegularExpressionMode
    };

    explicit TextEditSearchWidget(QPlainTextEdit *parent = 0);
    ~TextEditSearchWidget();
    bool doSearch(bool searchDown = true, bool allowRestartAtTop = true);

public slots:
    void activate();
    void deactivate();
    void doSearchDown();
    void doSearchUp();
    void setReplaceMode(bool enabled);
    void activateReplace();
    bool doReplace(bool forAll = false);
    void doReplaceAll();

protected slots:
    void searchLineEditTextChanged(const QString &arg1);
};

}

#endif
