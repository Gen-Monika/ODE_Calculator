#pragma once

#include "ExpressionParser.h"
#include "OdeSolver.h"

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPushButton;
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
    void solveFormulaInput();
    void copyResult();
    void insertFormulaToken(const QString& token, int cursorOffset = 0);
    void backspaceFormulaInput();
    void deleteFormulaInput();
    void moveFormulaCursor(int delta);
    void scheduleFormulaPreviewUpdate();
    void updateFormulaPreview();

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

    QPushButton* resetButton_ = nullptr;
    QPushButton* copyButton_ = nullptr;
    QWebEngineView* output_ = nullptr;
};
