#pragma once

#include "ExpressionParser.h"
#include "OdeSolver.h"

#include <QMainWindow>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTimer;
class QWebEngineView;

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
    void solveFormulaInput();
    void copyResult();
    void insertFormulaToken(const QString& token, int cursorOffset = 0);
    void scheduleFormulaPreviewUpdate();
    void updateFormulaPreview();

    bool readRational(QLineEdit* edit, const QString& name, Rational& value);
    bool readCoefficients(QLineEdit* edit, const QString& name, std::vector<Rational>& ascending);
    SolverInput makeInput(bool& ok);
    QString equationPreview() const;
    void showIntro();
    void showOutputHtml(const QString& bodyHtml);
    void showFormulaPreviewHtml(const QString& bodyHtml);

    OdeSolver solver_;
    ExpressionParser parser_;
    SolverResult lastResult_;
    QString fallbackCopyText_;

    QLineEdit* formulaEdit_ = nullptr;
    QPushButton* formulaSolveButton_ = nullptr;
    QWebEngineView* formulaPreview_ = nullptr;
    QLabel* formulaPreviewStatus_ = nullptr;
    QTimer* formulaPreviewTimer_ = nullptr;

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
    QWebEngineView* output_ = nullptr;
};
