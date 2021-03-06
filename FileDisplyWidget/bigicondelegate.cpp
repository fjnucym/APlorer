#include "bigicondelegate.h"
#include <QTextEdit>
#include <QFileInfo>
#include <QLineEdit>
#include <QFileIconProvider>
#include <QPainter>
#include <windows.h>
#include <QtWinExtras/QtWinExtras>
#include <QIcon>
#include <QPixmap>
#include <windows.h>
#include <QFileInfo>
#include <QtWinExtras/QtWin>
#include <QMessageBox>
#include <shellapi.h>
#include <initguid.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <QFontMetrics>


QSize BigIconDelegate::rectSize = QSize(150, 150);

QIcon BigIconDelegate::getIcon(QString path, IconSize size)
{
    Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);
    QFileInfo info(path);
    SHFILEINFOW psfi;
    if (info.suffix() == "")
        SHGetFileInfo((CONST TCHAR*)info.suffix().utf16(), -1, &psfi, sizeof(psfi), SHGFI_SYSICONINDEX);
    else {
        QString suffix = path;
        suffix.replace("/", "\\");
        SHGetFileInfo((CONST TCHAR*)suffix.utf16(), -1, &psfi, sizeof(psfi), SHGFI_SYSICONINDEX);
    }
    IImageList* imageList = nullptr;
    int type;
    switch (size) {
    case ExBigIcon:
        type = 4;
        break;
    case BigIcon:
        type = 4;
        break;
    case MidIcon:
        type = SHIL_EXTRALARGE;
        break;
    }
    SHGetImageList(type, IID_IImageList, (void ** )&imageList);
    HICON hIcon;
    imageList->GetIcon(psfi.iIcon, ILD_TRANSPARENT, &hIcon);
    QPixmap pix = qt_pixmapFromWinHICON(hIcon);
    return QIcon(pix);
}


QString BigIconDelegate::stringFomat(QString text, BigIconDelegate::IconSize size) {
    QFontMetrics metrics(text);
    QString temp;
    int rectWidth;
    switch (size) {
    case BigIconDelegate::BigIcon:
        rectWidth = 140;
        break;
    case BigIconDelegate::ExBigIcon:
        rectWidth = 265;
        break;
    case BigIconDelegate::MidIcon:
        rectWidth = 80;
        break;
    }
    int hopeWidth = metrics.width(text);
    if (hopeWidth > rectWidth) {
        int start = 0, length = 1;
        int lines = hopeWidth/rectWidth;
        for (int i = 0; i < lines && i < 3; ++i) {
            while (metrics.width(text.mid(start, length)) < rectWidth) {
                length++;
            }
            temp += text.mid(start, length-1) + "\n";
            start += length - 1;
            length = 1;
        }
        if (lines > 4) {
            temp += metrics.elidedText(text.mid(start, text.length()-start), Qt::ElideRight, rectWidth);
        } else
            temp += text.mid(start, text.length()-start);
        text = temp;
    }
    return text;
}


BigIconDelegate::BigIconDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    size = BigIcon;
}

BigIconDelegate::BigIconDelegate(IconSize size, QObject *parent) :
    QStyledItemDelegate(parent), size(size)
{

}


QWidget *BigIconDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QLineEdit *editor = new QLineEdit(parent);
    QString fileName = index.data(Qt::UserRole+1).value<QString>(); // ???ModelIndex?????????Model???????????????????????????
    QFileInfo info(fileName); // ???????????????????????????FileInfo
    editor->setText(info.fileName());
    connect(editor, &QLineEdit::editingFinished, [=](){
        // ????????????????????????????????????
        editor->hide(); // ???????????????
    });
    return editor;
}

void BigIconDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect; // ???????????????
    QString absolutePath = index.data(Qt::UserRole+1).value<QString>(); // ???????????????????????????????????????
    if (absolutePath.isEmpty())
        return;
    QFileInfo info(absolutePath); // ????????????????????????????????????
    // (200, 150)
    int adjustWidth, adjustHeight1, adjustHeight2;
    QFontMetrics metrics(info.fileName());
    QSize iconSize;
    int lines;
    switch (size) {
    case BigIcon:
        iconSize = QSize(128, 128);
        adjustHeight1 = 10;
        lines = metrics.width(info.fileName())/140 + 1;
        break;
    case ExBigIcon:
        iconSize = QSize(256, 256);
        adjustHeight1 = 13;
        lines = metrics.width(info.fileName())/256 + 1;
        break;
    case MidIcon:
        iconSize = QSize(48, 48);
        lines = metrics.width(info.fileName())/80 + 1;
        adjustHeight1 = 3;
        break;
    }

    QPixmap pixmap = getIcon(absolutePath, size).pixmap(iconSize);
    adjustWidth = (rectSize.width()-iconSize.width()) / 2;
    adjustHeight2 = rectSize.height() - iconSize.height() - adjustHeight1;
    QRect targetRect = rect.adjusted(+adjustWidth, 0, -adjustWidth, 0);
    targetRect.setHeight(iconSize.height());
    targetRect = targetRect.adjusted(0, +adjustHeight1, 0, +adjustHeight1);
    painter->drawPixmap(targetRect, pixmap);

    painter->drawText(rect.adjusted(+0, +iconSize.height(), 0, 0),
                      Qt::AlignHCenter| Qt::AlignTop, stringFomat(info.fileName(), size)); // ????????????????????????
}


void BigIconDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QStandardItemModel *smodel = static_cast<QStandardItemModel *>(model);
    QLineEdit *editor1 = static_cast<QLineEdit *>(editor); // ??????????????????
    QString fileName = index.data(Qt::UserRole+1).value<QString>(); // ???????????????????????????????????????
    QFile file(fileName);
    QFileInfo x(file);
    if (file.rename(x.absoluteDir().path() + "/" + editor1->text())) {
        // ?????????????????????
        x.setFile(file);
        QStandardItem *item = new QStandardItem; // ???????????????Item??????????????????
        item->setData(x.fileName(), Qt::DisplayRole);
        item->setData(x.absoluteFilePath(), Qt::UserRole+1); // ?????????
        smodel->setItem(index.row(), 0, item); // ??????item
    }
}


QSize BigIconDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString fileName = index.data().toString();
    switch (size) {
    case BigIcon: {
        QFontMetrics metrics(stringFomat(fileName, size));
        int lines = metrics.width(fileName)/140 + 1;
        if (lines > 4)
            lines = 4;
        rectSize = QSize(150, lines*metrics.height()+128+10+5);
        break;
    }
    case ExBigIcon: {
        QFontMetrics metrics(fileName);
        int lines = metrics.width(fileName)/265 + 1;
        if (lines > 4)
            lines = 4;
        rectSize = QSize(275, lines*metrics.height()+256+13+5);
        break;
    }
    case MidIcon: {
        QFontMetrics metrics(stringFomat(fileName, size));
        int lines = metrics.width(fileName)/80 + 1;
        if (lines > 4)
            lines = 4;
        rectSize = QSize(90, lines*metrics.height()+64+5+5);
        break;
    }
    }
    return rectSize;
}

void BigIconDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
    int adjustHeight1;
    QSize iconSize;
    switch (size) {
    case BigIcon:
        iconSize = QSize(128, 128);
        adjustHeight1 = 10;
        break;
    case ExBigIcon:
        iconSize = QSize(256, 256);
        adjustHeight1 = 13;
        break;
    case MidIcon:
        iconSize = QSize(48, 48);
        adjustHeight1 = 3;
        break;
    }
    QRect rect = option.rect;
    QFontMetrics metrics(lineEdit->text());
    rect.setWidth(metrics.width(lineEdit->text()));
    rect.setHeight(metrics.height());
    rect = rect.adjusted(0, +iconSize.height()-adjustHeight1, 0, +iconSize.height()-adjustHeight1);
    lineEdit->setAlignment(Qt::AlignHCenter);
    editor->setGeometry(option.rect.adjusted(0, +iconSize.height()-adjustHeight1, 0, 0)); // ???????????????????????????
}
