#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_ExecuteMinMax_clicked();
    void on_ExecuteMatrixMult_clicked();
    void on_ExecuteSort_clicked();
    void on_ExecutePi_clicked();
    void on_ClearFields_clicked();
    void on_ExecuteTP_clicked();

private:
    Ui::MainWindow *ui;

    void executeWithRedirect(std::function<void()> func);
};
#endif
