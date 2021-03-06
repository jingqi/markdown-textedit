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

#ifndef ___HEADFILE_A9E88903_74DE_486B_A737_50FCCC829B1D_
#define ___HEADFILE_A9E88903_74DE_486B_A737_50FCCC829B1D_

#include <QMainWindow>


namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Ui::MainWindow *_ui = NULL;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
};

#endif
