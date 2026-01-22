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

// Helper function to capture cout and update the UI
void MainWindow::executeWithRedirect(std::function<void()> func) {
    ui->plainTextEdit->clear(); // Refresh for every new problem

    std::stringstream ss;
    std::streambuf* oldCout = std::cout.rdbuf(ss.rdbuf());

    func(); // Execute the benchmark

    std::cout.rdbuf(oldCout); // Restore original cout
    ui->plainTextEdit->setPlainText(QString::fromStdString(ss.str()));
}

// 1. Min/Max Execution
void MainWindow::on_ExecuteMinMax_clicked() {
    executeWithRedirect([this]() {
        size_t threads = ui->threadsMinMax->text().toULong();
        size_t size = ui->sizeArrayMinMax->text().toULong();
        Tests().findMinMaxParallel(threads, size);
    });
}

// 2. Matrix Multiplication Execution
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

// 3. Array Sort Execution
void MainWindow::on_ExecuteSort_clicked() {
    executeWithRedirect([this]() {
        size_t size = ui->sortArraySize->text().toULong();
        // Defaulting depth to 4 if not specified, or add a field for it
        Tests().sortArrayParalel(size, 4);
    });
}

// 4. Monte Carlo Pi Execution
void MainWindow::on_ExecutePi_clicked() {
    executeWithRedirect([this]() {
        size_t iter = ui->MonteCarloIterations->text().toULong();
        size_t threads = ui->MonteCarloThreads->text().toULong();
        Tests().MonteCarloCountPiEstimation(iter, threads);
    });
}
// 5. TP Benchmarking
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
