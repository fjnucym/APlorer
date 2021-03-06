#include "mytableview.h"
#include <QDebug>
#include <QMimeDatabase>
#include <QClipboard>
#include <QDateTime>
#include <QLineEdit>
#include <QHeaderView>
#include <QSettings>
#include <aplmainwindow.h>
#include <QFileIconProvider>
#include <QDesktopServices>
#include "CShell.h"

#include "contentdelegate.h"
#include "detaildelegate.h"
#include "detaildelegate2.h"
#include "listdelegate.h"
#include "bigicondelegate.h"

MyTableView::MyTableView(QWidget *parent) : QTableView(parent)
{
    // 隐藏垂直标头
    QHeaderView* vHeaderView =  this->verticalHeader();
    vHeaderView->setHidden(true);
    // 平行标头可移动
    QHeaderView* hHeaderView = this->horizontalHeader();
    hHeaderView->setMinimumSectionSize(50); // 最小的表头宽度
    hHeaderView->setSectionsMovable(true);


}


MyTableView::MyTableView(QString absolutePath, DisplayMode mode, QWidget *parent):
    QTableView(parent)
{
    historyIndex  = -1;
    isLeft = false;

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setMinimumSize(QSize(782, 490));
    this->setShowGrid(false);
    this->setGridStyle(Qt::NoPen);
    this->setSortingEnabled(true);

    QHeaderView *vHeadView = this->verticalHeader();
    vHeadView->setHidden(true);
    // 文件信息显示的model
    model = new QStandardItemModel(this);
    mySelectionModel = new QItemSelectionModel(model);
    // 创建代理
    detailDelegate = new DetailDelegate(this);
    detailDelegate2 = new DetailDelegate2(this);
    listDelegate = new ListDelegate(this);
    exbigIconDelegate = new BigIconDelegate(BigIconDelegate::ExBigIcon, this);
    bigIconDelegate = new BigIconDelegate(BigIconDelegate::BigIcon, this);
    midIconDelegate = new BigIconDelegate(BigIconDelegate::MidIcon, this);
    contentDelegate = new ContentDelegate(this);

    this->setModel(model);
    this->setSelectionModel(mySelectionModel);

    forward =  new QAction(QIcon(":/icon/default theme/forward-.png"), "前进");
    backward = new QAction(QIcon(":/icon/default theme/back-arrow-.png"), "后退");

    // 历史记录菜单
    historyMenu = new QMenu;

    // 布局的菜单
    layoutMenu = new QMenu;
    QList <QAction *> acts_layout = {
        new QAction(QIcon(":/icon/default theme/ratio--_2.png"), "超大图标"),
        new QAction(QIcon(":/icon/default theme/grid-.png"), "大图标"),
        new QAction(QIcon(":/icon/default theme/grid-_1.png"), "中图标"),
        new QAction(QIcon(":/icon/default theme/layout-.png"), "详情"),
        new QAction(QIcon(":/icon/default theme/layout-_4.png"), "内容"),
        new QAction(QIcon(":/icon/default theme/layout-_1.png"), "列表"),
    };
    layoutMenu->addActions(acts_layout);
    // 排序菜单
    this->setSortingEnabled(true);


    // 初始化页面
    setCurrentPage(absolutePath, mode, true);
    // 所有连接信号
    foreach (auto action, acts_layout) {
        connect(action, &QAction::triggered, this, &MyTableView::slt_layoutChanged);
    }
    connect(forward, &QAction::triggered, this, &MyTableView::slt_forward);
    connect(backward, &QAction::triggered, this, &MyTableView::slt_backward);
}

void MyTableView::mousePressEvent(QMouseEvent *event)
{
    if (openedEdior != nullptr) {
        closePersistentEditor(*openedEdior);
    }
    if (event->button() == Qt::LeftButton)
    {
        // 左键
        static QModelIndex lastIndex = this->indexAt(event->pos());
        static QTime lastTime = QTime::currentTime();

        // 选中项目预览
        mySelectionModel->select(this->indexAt(event->pos()), QItemSelectionModel::ClearAndSelect);
        QModelIndexList indexList = this->selectedIndexes();
        if (indexList.count() == 1) {
            QModelIndex singleIndex = indexList.at(0);
            QMimeDatabase db;
            QString url = singleIndex.data(Qt::UserRole+1).toString();
            QMimeType type = db.mimeTypeForUrl(url);
            if (type.name().contains("image") | type.name().contains("text")) {
                emit refreshPreview(singleIndex);
            }
        }

        // 上一次不是左键
        if (!isLeft) {
            isLeft = true;
            lastIndex = this->indexAt(event->pos());
            lastTime = QTime::currentTime();
        } else {
            QTime currentTime = QTime::currentTime();
            if (lastTime.msecsTo(currentTime) < 1000) {
                // 两次点击事件不超过1s，打开文件
                QModelIndex index = this->indexAt(event->pos());
                if (index == lastIndex) {
                    // 两次点击同一个名称
                    openFile(index);
                }
            } else {
                // 否则修改文件名
                QModelIndex index = this->indexAt(event->pos());
                if (index == lastIndex && ! index.data(Qt::UserRole+1).toString().isEmpty()) {
                    // 两次点击同一个名称
                    if (currentMode == MyTableView::DETAIL && index.column() == 0)
                        openPersistentEditor(index);
                    else
                        openPersistentEditor(index);
                    openedEdior = new QModelIndex(index);
                }
                lastIndex = index;
            }
            lastTime = QTime::currentTime();
        }
    } else if (event->button() == Qt::RightButton) {
        isLeft = false;
        QPoint pos = event->globalPos();
        QModelIndex index = this->indexAt(event->pos());
        mySelectionModel->select(index, QItemSelectionModel::ClearAndSelect);
        if (index.column() == -1 && index.row() == -1) {
            std::vector<std::wstring> paths;
            QString path1 =  dir.path();
            paths.push_back(path1.toStdWString());

            OsShell::openShellContextMenuForObjects(paths, pos.x(), pos.y(), reinterpret_cast<void*>(winId()));
        } else {
            std::vector<std::wstring> paths;
            QString path1 = index.data(Qt::UserRole+1).toString();
            paths.push_back(path1.toStdWString());

            OsShell::openShellContextMenuForObjects(paths, pos.x(), pos.y(), reinterpret_cast<void*>(winId()));
        }
    } else {
        isLeft = false;
    }
    // QAbstractItemView::mousePressEvent(event);
}



QString MyTableView::fileType(QFileInfo info)
{
    // 获得文件类型
    QFileIconProvider provider;
    QString type;
    if (provider.type(info) == "File Folder")
        // 文件夹直接替换内容
        type = "文件夹";
    else {
        // 其他替换掉为文件字样
        QString temp = provider.type(info);
        type = temp.replace("File", "文件");
    }
    return type;
}

QString MyTableView::sizeFormat(QFileInfo info)
{
    // 获取文件大小合适的显示方式
    if (info.size() == 0)
        return "";
    else if (info.size() < 1024)
        return QString::number(info.size()) + "B";
    else if (info.size()/1024 < 1024)
        return QString::number(info.size()/1024) + "KB";
    else if (info.size()/(1024*1024) < 1024)
        return QString::number(info.size()/(1024*1024)) + "MB";
    else
        return QString::number(info.size()/(1024*1024)) + "GB";
}

void MyTableView::setCurrentPage(QString path, DisplayMode displayMode, bool isNew)
{
    isLeft = false;
    // 改变当前路径
    if (path != dir.path() && isNew) {
        history.insert(++historyIndex, path);
        dir.setPath(path);
        QAction *newPath;
        if (!dir.dirName().isEmpty())
        {
            newPath = new QAction(dir.dirName());
            emit currentPageChanged(dir.path());
        }
        else{
            newPath = new QAction(path);
            emit currentPageChanged(path);
        }
        newPath->setData(dir.path());
        historyMenu->addAction(newPath);
        connect(newPath, &QAction::triggered, this, &MyTableView::slt_setHistoryPath);

    } else if (path != dir.path() && !isNew) {
        dir.setPath(path);
        emit currentPageChanged1(dir.path());
    }
    if (currentMode != displayMode || isNew) {
        currentMode = displayMode;
        QHeaderView* hHeaderView =  this->horizontalHeader();
        switch (currentMode) {
        case DETAIL:
            // 名称显示部分使用代理
            this->setItemDelegateForColumn(0, detailDelegate);
            setDetailModel();
            // 其他部分也使用代理，做到无法选择的效果
            for (int i = 1; i < 4; ++i)
                this->setItemDelegateForColumn(i, detailDelegate2);
            hHeaderView->setHidden(false);
            break;
        case LIST:
            for (int i = 0; i < 4; ++i)
                this->setItemDelegateForColumn(i, listDelegate);
            this->setItemDelegate(listDelegate);
            setListModel();
            // 隐藏水平表头
            hHeaderView->setHidden(true);
            break;
        case BIGICON:
            for (int i = 0; i < 4; ++i)
                this->setItemDelegateForColumn(i, bigIconDelegate);
            this->setItemDelegate(bigIconDelegate);
            hHeaderView->setHidden(true);
            break;
        case EXBIGICION:
            for (int i = 0; i < 4; ++i)
                this->setItemDelegateForColumn(i, exbigIconDelegate);
            this->setItemDelegate(exbigIconDelegate);
            hHeaderView->setHidden(true);
            break;
        case MIDICON:
            for (int i = 0; i < 4; ++i)
                this->setItemDelegateForColumn(i, midIconDelegate);
            this->setItemDelegate(midIconDelegate);
            hHeaderView->setHidden(true);
            break;
        case CONTENT:
            this->setItemDelegateForColumn(0, contentDelegate);
            hHeaderView->setHidden(true);
            break;
        }
    }
    switch (currentMode) {
    case DETAIL:
        setDetailModel();
        break;
    case LIST:
        setListModel();
        break;
    case BIGICON:
    case EXBIGICION:
    case MIDICON:
        setBigIconModel();
        break;
    case CONTENT:
        setContentModel();
        break;
    }
    status = QString::number(dir.entryInfoList(QDir::NoDotAndDotDot|QDir::AllEntries).count()) + "项";
    emit currentPageChanged3(status);
    // 设置tableView的合适宽度,这个大小依赖于代理中的SizeHint
    this->resizeColumnsToContents();
    this->resizeRowsToContents();
    this->selectionModel();
}

void MyTableView::slt_backward()
{
    if (historyIndex == 0) {
        emit backHome();
    } else {
        QString backPath = history.at(--historyIndex);
        setCurrentPage(backPath, currentMode, false);
    }
}

void MyTableView::slt_forward()
{
    if (history.length() > historyIndex+1) {
        QString forwardPath = history.at(++historyIndex);
        setCurrentPage(forwardPath, currentMode, false);
    }
}

void MyTableView::slt_layoutChanged()
{
    QAction *action = static_cast<QAction *>(sender());
    QString text = action->text();
    if (text == "超大图标")
        setCurrentPage(dir.path(), EXBIGICION, true);
    else if (text == "大图标")
        setCurrentPage(dir.path(), BIGICON, true);
    else if (text == "中图标")
        setCurrentPage(dir.path(), MIDICON, true);
    else if (text == "内容")
        setCurrentPage(dir.path(), CONTENT, true);
    else if (text == "列表")
        setCurrentPage(dir.path(), LIST, true);
    else if (text == "详情")
        setCurrentPage(dir.path(), DETAIL, true);
    setCurrentPage(dir.path(), currentMode, false);
}

void MyTableView::slt_setHistoryPath()
{
    QAction* historyPath = static_cast<QAction *>(sender());
    QString absolutePath = historyPath->data().toString();
    setCurrentPage(absolutePath, currentMode, false);
    historyIndex = history.indexOf(absolutePath);
}

void MyTableView::setDetailModel()
{
    model->clear();
    model->setColumnCount(4); // 设置列数4列如下，这个不设置标头不会显示
    model->setHeaderData(0, Qt::Horizontal, "名称");
    model->setHeaderData(1, Qt::Horizontal, "修改日期");
    model->setHeaderData(2, Qt::Horizontal, "类型");
    model->setHeaderData(3, Qt::Horizontal, "大小");

    for (auto x: dir.entryInfoList(QDir::NoDotAndDotDot|QDir::AllEntries)) {
        // 过滤选择不要上一级和本级目录
        QStandardItem* item = new QStandardItem; // 第一列需要较为复杂的item
        int row = model->rowCount(); // 每次都是新增列，所以行数为rowCount
        item->setData(x.fileName(), Qt::DisplayRole); // 排序用文件名
        item->setData(x.absoluteFilePath(), Qt::UserRole+1); // 文件绝对路径
        item->setData(x.isDir(), Qt::UserRole+2); // 是否为文件夹
        model->setItem(row, 0, item); // 第一列文件名
        model->setItem(row, 1, new QStandardItem(x.lastModified().toString("yyyy/M/d hh:m"))); // 第二列上次修改日期
        model->setItem(row, 2, new QStandardItem(fileType(x))); // 第三列文件类型
        model->setItem(row, 3, new QStandardItem(sizeFormat(x))); // 第四列大小
    }
}

void MyTableView::setListModel()
{
    model->clear();
    int row = 0;
    for (auto x: dir.entryInfoList(QDir::NoDotAndDotDot|QDir::AllEntries)) {
        // 过滤选择不要上一级和本级目录
        QStandardItem* item = new QStandardItem; // 第一列需要较为复杂的item
        item->setData(x.fileName(), Qt::DisplayRole); // 文件名
        item->setData(x.absoluteFilePath(), Qt::UserRole+1); // 文件绝对路径名
        static int maxColumn;
        if (row == 0) {
            model->setItem(row, 0, item);
            maxColumn = this->contentsRect().height()/this->rowHeight(0) - 1;
            row++;
        } else {
            model->setItem(row%maxColumn, row/maxColumn, item);
            row++;
        }
    }
}

void MyTableView::setBigIconModel()
{
    model->clear();
    int count = 0;
    for (auto x: dir.entryInfoList(QDir::NoDotAndDotDot|QDir::AllEntries)) {
        // 过滤选择不要上一级和本级目录
        QStandardItem* item = new QStandardItem; // 第一列需要较为复杂的item
        item->setData(x.fileName(), Qt::DisplayRole); // 文件名
        item->setData(x.absoluteFilePath(), Qt::UserRole+1); // 文件绝对路径名
        static int maxColumn;
        if (count == 0) {
            model->setItem(count, 0, item);
            QAbstractItemDelegate *delegate = this->itemDelegate();
            BigIconDelegate *delegate1 = static_cast<BigIconDelegate *>(delegate);
            maxColumn = this->contentsRect().width()/delegate1->rectSize.width();
            count++;
        } else {
            model->setItem(count/maxColumn, count%maxColumn, item);
            count++;
        }
    }
}

void MyTableView::setContentModel()
{
    model->clear();
    int count = 0;
    for (auto x: dir.entryInfoList(QDir::NoDotAndDotDot|QDir::AllEntries)) {
        // 过滤选择不要上一级和本级目录
        QStandardItem* item = new QStandardItem; // 第一列需要较为复杂的item
        item->setData(x.fileName(), Qt::DisplayRole); // 文件名
        item->setData(x.absoluteFilePath(), Qt::UserRole+1); // 文件绝对路径名
        model->setItem(count, 0, item);
        count++;
    }
}

void MyTableView::openFile(QModelIndex index)
{
    if (index.isValid()) {
        QString absolutePath;
        QFileInfo info;
        switch (currentMode) {
        case DETAIL:
            if (index.column() != 0)
                index = model->index(index.row(), 0);
            absolutePath = index.data(Qt::UserRole+1).value<QString>();
            info.setFile(absolutePath);
            if (info.isDir())
                // 是文件夹打开文件夹
                setCurrentPage(index.data(Qt::UserRole+1).value<QString>(), currentMode, true);
            else
                QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath));
            break;
        case LIST:
            absolutePath = index.data(Qt::UserRole+1).value<QString>();
            info.setFile(absolutePath);
            if (info.isDir())
                // 是文件夹打开文件夹
                setCurrentPage(index.data(Qt::UserRole+1).value<QString>(), currentMode, true);
            else
                QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath));
            break;
        case BIGICON:
        case MIDICON:
        case EXBIGICION:
        case CONTENT:
            absolutePath = index.data(Qt::UserRole+1).value<QString>();
            info.setFile(absolutePath);
            if (info.isDir())
                // 是文件夹打开文件夹
                setCurrentPage(index.data(Qt::UserRole+1).value<QString>(), currentMode, true);
            else
                QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath));
            break;
        }

    }

}


#include "CShell.h"

myTableView::myTableView(QWidget *parent): QTableView(parent)
{

}

void myTableView::setFileInfo(const QVector<QFileInfo> &fileInfos)
{
    _fileInfos = fileInfos;
}

void myTableView::mouseReleaseEvent(QMouseEvent* e) {
    QModelIndex idx = this->indexAt(e->pos());
    QPoint pos = mapToGlobal(e->pos());
    if (e->button() == Qt::RightButton) {
        if (idx.isValid()) {
            QString filePath(_fileInfos[idx.row()].path() + "/" + _fileInfos[idx.row()].fileName());
            std::vector<std::wstring> paths;
            paths.push_back(filePath.toStdWString());
            OsShell::openShellContextMenuForObjects(paths, pos.x(), pos.y(), reinterpret_cast<void*>(winId()));
        }
    } else if (e->button() == Qt::LeftButton) {
        if (idx.isValid()){
            QString filePath(_fileInfos[idx.row()].path() + "/" + _fileInfos[idx.row()].fileName());
            std::vector<std::wstring> paths;
            paths.push_back(filePath.toStdWString());
            aplMainWindow *mainwindow = static_cast<aplMainWindow *>(this->parent());

            // mainwindow->setPreviewLabel(QString::fromWCharArray(paths[0].data()));
        }
    }
    QTableView::mouseReleaseEvent(e);
}
