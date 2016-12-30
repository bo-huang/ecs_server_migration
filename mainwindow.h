#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Class/migration.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    void StartToEnd();
    void EndToStart();
private slots:
    void on_startPushButton_clicked();

    void on_chooseButton_clicked();

private:
    Ui::MainWindow *ui;
    bool start;
    Migration *migration;
};

#endif // MAINWINDOW_H
