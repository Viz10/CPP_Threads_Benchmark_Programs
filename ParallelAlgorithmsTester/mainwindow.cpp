#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "Tests.h"
#include <sstream>
#include <string>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/// Helper function to capture cout and update the UI
void MainWindow::executeWithRedirect(std::function<void()> func) {
    ui->plainTextEdit->clear();

    std::stringstream ss;
    std::streambuf* oldCout = std::cout.rdbuf(ss.rdbuf());

    func();

    std::cout.rdbuf(oldCout);
    ui->plainTextEdit->setPlainText(QString::fromStdString(ss.str()));
}

void MainWindow::on_ExecuteMinMax_clicked() {
    executeWithRedirect([this]() {
        size_t threads = ui->threadsMinMax->text().toULong();
        size_t size = ui->sizeArrayMinMax->text().toULong();
        Tests().findMinMaxParallel(threads, size);
    });
}

void MainWindow::on_ExecuteMatrixMult_clicked() {
    executeWithRedirect([this]() {
        size_t threads = ui->threadsMatrix->text().toULong();
        size_t rA = ui->rowsA->text().toULong();
        size_t cA = ui->columnsA->text().toULong();
        size_t rB = ui->rowsB->text().toULong();
        size_t cB = ui->columnsB->text().toULong();
        Tests().matrixMultiplicationParallel(threads, rA, cA, rB, cB);
    });
}

void MainWindow::on_ExecuteSort_clicked() {
    executeWithRedirect([this]() {
        size_t size = ui->sortArraySize->text().toULong();
        // Defaulting depth = 4
        Tests().sortArrayParalel(size, 4);
    });
}

void MainWindow::on_ExecutePi_clicked() {
    executeWithRedirect([this]() {
        size_t iter = ui->MonteCarloIterations->text().toULong();
        size_t threads = ui->MonteCarloThreads->text().toULong();
        Tests().MonteCarloCountPiEstimation(iter, threads);
    });
}

void MainWindow::on_ExecuteTP_clicked() {
    executeWithRedirect([this]() {
        size_t iter = ui->TPIterations->text().toULong();
        size_t threads = ui->TPMaxThreads->text().toULong();
        size_t num_tasks = ui->TPNrTasksAdded->text().toULong();
        Tests().run_benchmark(threads,num_tasks,iter);
    });
}

void MainWindow::on_ClearFields_clicked() {
   ui->plainTextEdit->clear();

   ui->threadsMinMax->clear();
   ui->sizeArrayMinMax->clear();

   ui->threadsMatrix->clear();
   ui->rowsA->clear();
   ui->rowsB->clear();
   ui->columnsA->clear();
   ui->columnsB->clear();

   ui->sortArraySize->clear();

   ui->MonteCarloIterations->clear();
   ui->MonteCarloThreads->clear();

   ui->TPIterations->clear();
   ui->TPMaxThreads->clear();
   ui->TPNrTasksAdded->clear();
}
