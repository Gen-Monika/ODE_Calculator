#pragma once

#include "OdeSolver.h"

#include <QMainWindow>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTextBrowser;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void buildUi();
    void connectSignals();
    void resetExample();
    void updateEquationState();
    void solveCurrentInput();
    void copyResult();

    bool readRational(QLineEdit* edit, const QString& name, Rational& value);
    bool readCoefficients(QLineEdit* edit, const QString& name, std::vector<Rational>& ascending);
    SolverInput makeInput(bool& ok);
    QString equationPreview() const;
    void showIntro();

    OdeSolver solver_;
    SolverResult lastResult_;

    QComboBox* orderCombo_ = nullptr;
    QLabel* a1Label_ = nullptr;
    QLineEdit* a1Edit_ = nullptr;
    QLineEdit* a0Edit_ = nullptr;
    QLabel* equationPreview_ = nullptr;

    QComboBox* kindCombo_ = nullptr;
    QStackedWidget* forcingStack_ = nullptr;

    QLineEdit* expREdit_ = nullptr;
    QLineEdit* polynomialEdit_ = nullptr;

    QLineEdit* trigAEdit_ = nullptr;
    QLineEdit* trigBEdit_ = nullptr;
    QLineEdit* cosinePolynomialEdit_ = nullptr;
    QLineEdit* sinePolynomialEdit_ = nullptr;

    QPushButton* solveButton_ = nullptr;
    QPushButton* resetButton_ = nullptr;
    QPushButton* copyButton_ = nullptr;
    QTextBrowser* output_ = nullptr;
};
