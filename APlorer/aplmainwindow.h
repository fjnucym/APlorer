#ifndef APLMAINWINDOW_H
#define APLMAINWINDOW_H

#include <QMainWindow>
#include <QMenu>

QT_BEGIN_NAMESPACE
namespace Ui { class aplMainWindow; }
QT_END_NAMESPACE

class QFileSystemModel;



class aplMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    aplMainWindow(QWidget *parent = nullptr);
    ~aplMainWindow();
    void init();
    QFileSystemModel* fileModel();
    void copy(aplMainWindow* oldApl);

private slots:
    void on_actionforward_triggered();

private:
    Ui::aplMainWindow *ui;

    QFileSystemModel *m_model;

    void initToolBar(); // 初始化工具栏
    void initBrowser(); // 初始化浏览窗口
    void setToolButtonActions(); // 设置按钮对应Action
    void setMenus(); // 设置菜单
protected:
    bool event(QEvent *evetn) override;


};
#endif // APLMAINWINDOW_H
