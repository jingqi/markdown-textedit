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

#include <QEvent>
#include <QKeyEvent>
#include <QDebug>

#include "textedit_search_widget.h"
#include "ui_textedit_search_widget.h"

namespace mdtextedit
{

TextEditSearchWidget::TextEditSearchWidget(QPlainTextEdit *parent)
    : QWidget(parent), _ui(new Ui::TextEditSearchWidget)
{
    _ui->setupUi(this);
    _textedit = parent;
    hide();

    QObject::connect(_ui->closeButton, SIGNAL(clicked()),
                     this, SLOT(deactivate()));
    QObject::connect(_ui->searchLineEdit, SIGNAL(textChanged(const QString &)),
                     this, SLOT(searchLineEditTextChanged(const QString &)));
    QObject::connect(_ui->searchDownButton, SIGNAL(clicked()),
                     this, SLOT(doSearchDown()));
    QObject::connect(_ui->searchUpButton, SIGNAL(clicked()),
                     this, SLOT(doSearchUp()));
    QObject::connect(_ui->replaceToggleButton, SIGNAL(toggled(bool)),
                     this, SLOT(setReplaceMode(bool)));
    QObject::connect(_ui->replaceButton, SIGNAL(clicked()),
                     this, SLOT(doReplace()));
    QObject::connect(_ui->replaceAllButton, SIGNAL(clicked()),
                     this, SLOT(doReplaceAll()));

    installEventFilter(this);
    _ui->searchLineEdit->installEventFilter(this);
    _ui->replaceLineEdit->installEventFilter(this);

#ifdef Q_OS_MAC
    // set the spacing to 8 for OS X
    layout()->setSpacing(8);
    _ui->buttonFrame->layout()->setSpacing(9);

    // set the margin to 0 for the top buttons for OS X
    QString buttonStyle = "QPushButton {margin: 0}";
    _ui->closeButton->setStyleSheet(buttonStyle);
    _ui->searchDownButton->setStyleSheet(buttonStyle);
    _ui->searchUpButton->setStyleSheet(buttonStyle);
    _ui->replaceToggleButton->setStyleSheet(buttonStyle);
    _ui->matchCaseSensitiveButton->setStyleSheet(buttonStyle);
#endif
}

TextEditSearchWidget::~TextEditSearchWidget()
{
    delete _ui;
}

void TextEditSearchWidget::activate()
{
    setReplaceMode(false);
    show();

    // preset the selected text as search text if there is any and there is no
    // other search text
    QString selectedText = _textedit->textCursor().selectedText();
    if (!selectedText.isEmpty() && _ui->searchLineEdit->text().isEmpty())
    {
        _ui->searchLineEdit->setText(selectedText);
    }

    _ui->searchLineEdit->setFocus();
    _ui->searchLineEdit->selectAll();
    doSearchDown();
}

void TextEditSearchWidget::activateReplace()
{
    // replacing is prohibited if the text edit is readonly
    if (_textedit->isReadOnly())
        return;

    _ui->searchLineEdit->setText(_textedit->textCursor().selectedText());
    _ui->searchLineEdit->selectAll();
    activate();
    setReplaceMode(true);
}

void TextEditSearchWidget::deactivate()
{
    hide();
    _textedit->setFocus();
}

void TextEditSearchWidget::setReplaceMode(bool enabled)
{
    _ui->replaceToggleButton->setChecked(enabled);
    _ui->replaceLabel->setVisible(enabled);
    _ui->replaceLineEdit->setVisible(enabled);
    _ui->modeLabel->setVisible(enabled);
    _ui->buttonFrame->setVisible(enabled);
    _ui->matchCaseSensitiveButton->setVisible(enabled);
}

bool TextEditSearchWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->key() == Qt::Key_Escape)
        {
            deactivate();
            return true;
        }
        else if ((keyEvent->modifiers().testFlag(Qt::ShiftModifier) &&
                  (keyEvent->key() == Qt::Key_Return)) ||
                 (keyEvent->key() == Qt::Key_Up))
        {
            doSearchUp();
            return true;
        }
        else if ((keyEvent->key() == Qt::Key_Return) ||
                 (keyEvent->key() == Qt::Key_Down))
        {
            doSearchDown();
            return true;
        }
        else if (keyEvent->key() == Qt::Key_F3)
        {
            doSearch(!keyEvent->modifiers().testFlag(Qt::ShiftModifier));
            return true;
        }

//        if ((obj == _ui->replaceLineEdit) && (keyEvent->key() == Qt::Key_Tab)
//                && _ui->replaceToggleButton->isChecked()) {
//            _ui->replaceLineEdit->setFocus();
//        }

        return false;
    }

    return QWidget::eventFilter(obj, event);
}

void TextEditSearchWidget::searchLineEditTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    doSearchDown();
}

void TextEditSearchWidget::doSearchUp()
{
    doSearch(false);
}

void TextEditSearchWidget::doSearchDown()
{
    doSearch(true);
}

bool TextEditSearchWidget::doReplace(bool forAll)
{
    if (_textedit->isReadOnly())
        return false;

    QTextCursor c = _textedit->textCursor();

    if (!forAll && c.selectedText().isEmpty())
        return false;

    int searchMode = _ui->modeComboBox->currentIndex();
    if (searchMode == RegularExpressionMode)
    {
        QString text = c.selectedText();
        text.replace(QRegExp(_ui->searchLineEdit->text()), _ui->replaceLineEdit->text());
        c.insertText(text);
    }
    else
    {
        c.insertText(_ui->replaceLineEdit->text());
    }

    if (!forAll)
    {
        int position = c.position();

        if (!doSearch(true))
        {
            // restore the last cursor position if text wasn't found any more
            c.setPosition(position);
            _textedit->setTextCursor(c);
        }
    }

    return true;
}

void TextEditSearchWidget::doReplaceAll()
{
    if (_textedit->isReadOnly())
        return;

    // start at the top
    _textedit->moveCursor(QTextCursor::Start);

    // replace until everything to the bottom is replaced
    while (doSearch(true, false) && doReplace(true))
    {}
}

/**
 * @brief Searches for text in the text edit
 * @returns true if found
 */
bool TextEditSearchWidget::doSearch(bool searchDown, bool allowRestartAtTop)
{
    QString text = _ui->searchLineEdit->text();

    if (text == "")
    {
        _ui->searchLineEdit->setStyleSheet("* { background: none; }");
        return false;
    }

    int searchMode = _ui->modeComboBox->currentIndex();

    QFlags<QTextDocument::FindFlag> options = searchDown ?
                                              QTextDocument::FindFlag(0)
                                              : QTextDocument::FindBackward;
    if (searchMode == WholeWordsMode)
    {
        options |= QTextDocument::FindWholeWords;
    }

    if (_ui->matchCaseSensitiveButton->isChecked())
    {
        options |= QTextDocument::FindCaseSensitively;
    }

    bool found;
    if (searchMode == RegularExpressionMode)
        found = _textedit->find(QRegExp(text), options);
    else
        found = _textedit->find(text, options);

    // start at the top (or bottom) if not found
    if (!found && allowRestartAtTop)
    {
        _textedit->moveCursor(searchDown ? QTextCursor::Start : QTextCursor::End);
        found = _textedit->find(text, options);
    }

    QRect rect = _textedit->cursorRect();
    QMargins margins = _textedit->layout()->contentsMargins();
    int searchWidgetHotArea = _textedit->height() - this->height();
    int marginBottom = (rect.y() > searchWidgetHotArea) ? (this->height() + 10)
                                                        : 0;

    // move the search box a bit up if we would block the search result
    if (margins.bottom() != marginBottom)
    {
        margins.setBottom(marginBottom);
        _textedit->layout()->setContentsMargins(margins);
    }

    // add a background color according if we found the text or not
    QString colorCode = found ? "#D5FAE2" : "#FAE9EB";
    _ui->searchLineEdit->setStyleSheet("* { background: " + colorCode + "; }");

    return found;
}

}
