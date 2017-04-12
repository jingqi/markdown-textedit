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
 * mainwindow.cpp
 *
 * Example to show the QMarkdownTextEdit widget
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), _ui(new Ui::MainWindow)
{
    _ui->setupUi(this);
    _ui->textEdit->load_style_from_stylesheet(":/markdown-textedit/theme/solarized-light+.txt");

    // Load sample file
    QFile f(":/sample.md");
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    QString all = f.readAll();
    _ui->textEdit->setPlainText(all);
}

MainWindow::~MainWindow()
{
    delete _ui;
}
