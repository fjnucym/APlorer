﻿#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ntfsdata.h"
#include <volume.h>
#include <QCompleter>
#include <QLineEdit>

#include <QFileSystemModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    NTFSData ntfsData;

    QStringList wordList;
    ntfsData.initData(wordList);
    qDebug() << "所有文件数量: " << wordList.size() << endl;
    QCompleter *completer = new QCompleter(wordList, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setFilterMode(Qt::MatchContains);
    completer->setMaxVisibleItems(15);

    ui->lineEdit->setCompleter(completer);
}

MainWindow::~MainWindow()
{
    delete ui;
}
