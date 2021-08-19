#include "dtaildelegate2.h"
#include <QPainter>
#include <QDebug>
#include <QStandardItem>

DtailDelegate2::DtailDelegate2(QObject *parent) : QStyledItemDelegate(parent)
{

}

QWidget *DtailDelegate2::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return nullptr;
}

void DtailDelegate2::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect;
    QString text = index.data().toString();
    QFontMetrics metrics(painter->font());
    if(metrics.width(text) > rect.width())
    {
        text = metrics.elidedText(text, Qt::ElideRight, rect.width());
    }
    painter->drawText(option.rect, text, Qt::AlignVCenter|Qt::AlignLeft);
}

QSize DtailDelegate2::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(150, option.rect.height());
}
