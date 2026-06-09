#include "MainWindow.h"

#include <QApplication>
#include <QClipboard>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QUrl>

#include <algorithm>

namespace {
QString texText(const QString& latex)
{
    return latex.toHtmlEscaped();
}

QString equationBlock(const QString& body)
{
    return "<div class='equation tex-display'>" + texText(body) + "</div>";
}

QString pageHtml(const QString& bodyHtml)
{
    return "<!doctype html><html><head><meta charset='utf-8'>"
           "<link rel='stylesheet' href='qrc:///katex/katex.min.css'>"
           "<script src='qrc:///katex/katex.min.js'></script>"
           "<style>"
           "html,body{margin:0;background:#fff;color:#1f2933;}"
           "body{font-family:'Microsoft YaHei UI','Segoe UI',sans-serif;font-size:15px;line-height:1.72;padding:22px 26px 34px;}"
           "h2{font-size:21px;margin:0 0 12px;color:#17212b;font-weight:650;}"
           "h2:not(:first-child){margin-top:26px;}"
           "h3{font-size:16px;margin:18px 0 8px;color:#263442;font-weight:650;}"
           "p{margin:8px 0;}"
           "code,pre{font-family:Consolas,'Cascadia Mono',monospace;}"
           "code{background:#f4f7fa;border:1px solid #dbe3ec;border-radius:4px;padding:2px 5px;}"
           "pre{white-space:pre-wrap;background:#f6f8fa;border:1px solid #d0d7de;border-radius:6px;padding:10px 12px;}"
           ".summary{border:1px solid #d7e0ea;border-radius:8px;margin:10px 0 18px;overflow:hidden;}"
           ".summary-row{display:grid;grid-template-columns:96px minmax(0,1fr);gap:12px;border-top:1px solid #e6edf3;padding:12px 14px;align-items:start;}"
           ".summary-row:first-child{border-top:0;}"
           ".summary-label{font-weight:650;color:#485766;}"
           ".equation{margin:10px 0 14px;padding:12px 14px;border-left:4px solid #527aa3;background:#f8fafc;overflow-x:auto;font-size:20px;line-height:1.65;}"
           ".tex-inline{white-space:nowrap;}"
           ".katex{font-size:1.08em;}"
           ".equation .katex-display{margin:0;text-align:left;}"
           "table{border-collapse:collapse;width:100%;margin:12px 0 16px;font-size:14px;}"
           "th,td{border:1px solid #d8e1ea;padding:8px 10px;text-align:left;vertical-align:top;}"
           "th{background:#f2f5f8;color:#3a4652;}"
           ".error{color:#9f2f2f;background:#fff6f6;border:1px solid #f0caca;border-radius:6px;padding:10px 12px;}"
           "</style>"
           "<script>"
           "function renderAllMath(){"
           "const opts={throwOnError:false,strict:false};"
           "document.querySelectorAll('.tex-inline').forEach(function(el){katex.render(el.textContent,el,Object.assign({},opts,{displayMode:false}));});"
           "document.querySelectorAll('.tex-display').forEach(function(el){katex.render(el.textContent,el,Object.assign({},opts,{displayMode:true}));});"
           "}"
           "document.addEventListener('DOMContentLoaded',renderAllMath);"
           "</script></head><body>"
        + bodyHtml + "</body></html>";
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();
    connectSignals();
    resetExample();
    showIntro();
}

void MainWindow::buildUi()
{
    setWindowTitle("ODE Calculator - 教学型常系数非齐次方程求解器");
    resize(980, 640);
    setMinimumSize(860, 560);

    auto* central = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(10, 10, 10, 10);

    auto* splitter = new QSplitter(Qt::Horizontal, central);
    rootLayout->addWidget(splitter);

    auto* left = new QWidget(splitter);
    left->setMinimumWidth(310);
    left->setMaximumWidth(420);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 8, 0);
    leftLayout->setSpacing(8);

    auto* formulaGroup = new QGroupBox("公式输入", left);
    auto* formulaLayout = new QVBoxLayout(formulaGroup);
    formulaLayout->setSpacing(8);

    auto* formulaInputLayout = new QHBoxLayout();
    formulaInputLayout->setSpacing(6);
    formulaEdit_ = new QLineEdit(formulaGroup);
    formulaEdit_->setPlaceholderText("y''+y=x*cos(2x)");
    formulaSolveButton_ = new QPushButton("求解", formulaGroup);
    formulaInputLayout->addWidget(formulaEdit_, 1);
    formulaInputLayout->addWidget(formulaSolveButton_);
    formulaLayout->addLayout(formulaInputLayout);

    formulaPreview_ = new QWebEngineView(formulaGroup);
    formulaPreview_->setMinimumHeight(96);
    formulaPreview_->setMaximumHeight(120);
    formulaLayout->addWidget(formulaPreview_);
    formulaPreviewTimer_ = new QTimer(this);
    formulaPreviewTimer_->setSingleShot(true);
    formulaPreviewTimer_->setInterval(180);

    formulaPreviewStatus_ = new QLabel(formulaGroup);
    formulaPreviewStatus_->setWordWrap(true);
    formulaPreviewStatus_->setProperty("previewStatus", true);
    formulaPreviewStatus_->setMinimumHeight(20);
    formulaLayout->addWidget(formulaPreviewStatus_);

    auto* formulaButtonGrid = new QGridLayout();
    formulaButtonGrid->setHorizontalSpacing(5);
    formulaButtonGrid->setVerticalSpacing(5);

    enum class FormulaAction {
        Insert,
        Backspace,
        Delete,
        Clear,
        MoveLeft,
        MoveRight,
    };

    struct FormulaButton {
        const char* label;
        const char* token;
        int cursorOffset;
        FormulaAction action;
    };

    const FormulaButton buttons[] = {
        {"y", "y", 0, FormulaAction::Insert},
        {"y'", "y'", 0, FormulaAction::Insert},
        {"y''", "y''", 0, FormulaAction::Insert},
        {"x", "x", 0, FormulaAction::Insert},
        {"x^2", "x^2", 0, FormulaAction::Insert},
        {"x^3", "x^3", 0, FormulaAction::Insert},

        {"e^()", "e^()", -1, FormulaAction::Insert},
        {"exp()", "exp()", -1, FormulaAction::Insert},
        {"sin()", "sin()", -1, FormulaAction::Insert},
        {"cos()", "cos()", -1, FormulaAction::Insert},
        {"(", "(", 0, FormulaAction::Insert},
        {")", ")", 0, FormulaAction::Insert},

        {"=", "=", 0, FormulaAction::Insert},
        {"+", "+", 0, FormulaAction::Insert},
        {"-", "-", 0, FormulaAction::Insert},
        {"*", "*", 0, FormulaAction::Insert},
        {"/", "/", 0, FormulaAction::Insert},
        {"^", "^", 0, FormulaAction::Insert},

        {"0", "0", 0, FormulaAction::Insert},
        {"1", "1", 0, FormulaAction::Insert},
        {"2", "2", 0, FormulaAction::Insert},
        {"3", "3", 0, FormulaAction::Insert},
        {"4", "4", 0, FormulaAction::Insert},
        {"5", "5", 0, FormulaAction::Insert},

        {"6", "6", 0, FormulaAction::Insert},
        {"7", "7", 0, FormulaAction::Insert},
        {"8", "8", 0, FormulaAction::Insert},
        {"9", "9", 0, FormulaAction::Insert},
        {"1/2", "1/2", 0, FormulaAction::Insert},
        {"2x", "2x", 0, FormulaAction::Insert},

        {".", ".", 0, FormulaAction::Insert},
        {"←", "", 0, FormulaAction::MoveLeft},
        {"→", "", 0, FormulaAction::MoveRight},
        {"退格", "", 0, FormulaAction::Backspace},
        {"Del", "", 0, FormulaAction::Delete},
        {"清空", "", 0, FormulaAction::Clear},
    };

    constexpr int columns = 6;
    const int buttonCount = static_cast<int>(sizeof(buttons) / sizeof(buttons[0]));
    for (int i = 0; i < buttonCount; ++i) {
        auto* button = new QPushButton(QString::fromUtf8(buttons[i].label), formulaGroup);
        button->setProperty("formulaKey", true);
        button->setMinimumHeight(28);
        const QString token = QString::fromUtf8(buttons[i].token);
        const int cursorOffset = buttons[i].cursorOffset;
        const FormulaAction action = buttons[i].action;
        connect(button, &QPushButton::clicked, this, [this, token, cursorOffset, action]() {
            switch (action) {
            case FormulaAction::Insert:
                insertFormulaToken(token, cursorOffset);
                break;
            case FormulaAction::Backspace:
                backspaceFormulaInput();
                break;
            case FormulaAction::Delete:
                deleteFormulaInput();
                break;
            case FormulaAction::Clear:
                formulaEdit_->clear();
                formulaEdit_->setFocus();
                break;
            case FormulaAction::MoveLeft:
                moveFormulaCursor(-1);
                break;
            case FormulaAction::MoveRight:
                moveFormulaCursor(1);
                break;
            }
        });
        formulaButtonGrid->addWidget(button, i / columns, i % columns);
    }
    formulaLayout->addLayout(formulaButtonGrid);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(6);
    resetButton_ = new QPushButton("示例", formulaGroup);
    copyButton_ = new QPushButton("复制结果", formulaGroup);
    buttonLayout->addStretch();
    buttonLayout->addWidget(resetButton_);
    buttonLayout->addWidget(copyButton_);
    formulaLayout->addLayout(buttonLayout);

    leftLayout->addWidget(formulaGroup);
    leftLayout->addStretch();

    output_ = new QWebEngineView(splitter);
    output_->setMinimumWidth(500);

    splitter->addWidget(left);
    splitter->addWidget(output_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({340, 620});

    setCentralWidget(central);

    setStyleSheet(
        "QWidget { font-family: 'Microsoft YaHei UI', 'Segoe UI'; font-size: 10.5pt; }"
        "QGroupBox { font-weight: 600; border: 1px solid #cfd6df; border-radius: 6px; margin-top: 8px; padding: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        "QLineEdit { padding: 6px; border: 1px solid #c4ccd6; border-radius: 4px; }"
        "QPushButton { padding: 7px 10px; border: 1px solid #9ba8b7; border-radius: 4px; background: #f7f9fb; }"
        "QPushButton[formulaKey='true'] { padding: 4px 6px; min-width: 36px; background: #fbfcfd; }"
        "QPushButton:hover { background: #eef3f8; }"
        "QPushButton:pressed { background: #e1e8f0; }"
        "QWebEngineView { border: 1px solid #cfd6df; border-radius: 6px; background: white; }"
        "QLabel[previewStatus='true'] { color: #667586; font-size: 9pt; min-height: 18px; }");
}

void MainWindow::connectSignals()
{
    connect(formulaSolveButton_, &QPushButton::clicked, this, &MainWindow::solveFormulaInput);
    connect(formulaEdit_, &QLineEdit::returnPressed, this, &MainWindow::solveFormulaInput);
    connect(formulaEdit_, &QLineEdit::textChanged, this, &MainWindow::scheduleFormulaPreviewUpdate);
    connect(formulaPreviewTimer_, &QTimer::timeout, this, &MainWindow::updateFormulaPreview);
    connect(resetButton_, &QPushButton::clicked, this, &MainWindow::resetExample);
    connect(copyButton_, &QPushButton::clicked, this, &MainWindow::copyResult);
}

void MainWindow::resetExample()
{
    formulaEdit_->setText("y''+y=x*cos(2x)");
    formulaEdit_->setCursorPosition(formulaEdit_->text().size());
    formulaEdit_->setFocus();
    updateFormulaPreview();
}

void MainWindow::solveFormulaInput()
{
    const EquationParseResult parsed = parser_.parseEquation(formulaEdit_->text());
    if (!parsed.ok) {
        lastResult_ = SolverResult();
        fallbackCopyText_ = parsed.error;
        showOutputHtml("<h2>公式解析失败</h2><p class='error'>"
            + parsed.error.toHtmlEscaped() + "</p>"
            + "<p>当前原型支持示例：<code>y''+y=x*cos(2x)</code>、"
            + "<code>y''-2y'+y=x*e^x</code>、<code>y''+y=cos(x)</code>。</p>");
        return;
    }

    lastResult_ = solver_.solve(parsed.input);
    if (!lastResult_.ok) {
        fallbackCopyText_ = lastResult_.error;
        showOutputHtml("<h2>求解失败</h2><p class='error'>"
            + lastResult_.error.toHtmlEscaped() + "</p>");
        return;
    }

    QString html;
    html += "<h2>公式输入</h2>";
    html += "<p><b>规范化：</b> <code>" + parsed.normalizedText.toHtmlEscaped() + "</code></p>";
    html += equationBlock(parsed.latex);
    html += "<p><b>LaTeX 源码：</b></p><pre>" + parsed.latex.toHtmlEscaped() + "</pre>";
    html += lastResult_.html;
    fallbackCopyText_.clear();
    showOutputHtml(html);
}

void MainWindow::copyResult()
{
    if (!lastResult_.ok) {
        QApplication::clipboard()->setText(fallbackCopyText_);
        return;
    }
    QApplication::clipboard()->setText(lastResult_.plainSolution);
}

void MainWindow::insertFormulaToken(const QString& token, int cursorOffset)
{
    formulaEdit_->setFocus();
    const int insertedAt = formulaEdit_->cursorPosition();
    formulaEdit_->insert(token);
    if (cursorOffset != 0) {
        formulaEdit_->setCursorPosition(insertedAt + token.size() + cursorOffset);
    }
}

void MainWindow::backspaceFormulaInput()
{
    formulaEdit_->setFocus();
    formulaEdit_->backspace();
}

void MainWindow::deleteFormulaInput()
{
    formulaEdit_->setFocus();
    formulaEdit_->del();
}

void MainWindow::moveFormulaCursor(int delta)
{
    formulaEdit_->setFocus();
    const int textLength = static_cast<int>(formulaEdit_->text().size());
    const int nextPosition = std::clamp(
        formulaEdit_->cursorPosition() + delta,
        0,
        textLength);
    formulaEdit_->setCursorPosition(nextPosition);
}

void MainWindow::scheduleFormulaPreviewUpdate()
{
    formulaPreviewTimer_->start();
}

void MainWindow::updateFormulaPreview()
{
    const QString text = formulaEdit_->text().trimmed();
    if (text.isEmpty()) {
        formulaPreviewStatus_->clear();
        showFormulaPreviewHtml(equationBlock("\\phantom{y''+y=x\\cos(2x)}"));
        return;
    }

    const EquationParseResult parsed = parser_.parseEquation(text);
    if (!parsed.ok) {
        formulaPreviewStatus_->setText(parsed.error);
        showFormulaPreviewHtml("<p class='error'>" + parsed.error.toHtmlEscaped() + "</p>");
        return;
    }

    formulaPreviewStatus_->setText(parsed.normalizedText);
    showFormulaPreviewHtml(equationBlock(parsed.latex));
}

void MainWindow::showOutputHtml(const QString& bodyHtml)
{
    output_->setHtml(pageHtml(bodyHtml), QUrl("qrc:///"));
}

void MainWindow::showFormulaPreviewHtml(const QString& bodyHtml)
{
    formulaPreview_->setHtml(pageHtml(bodyHtml), QUrl("qrc:///"));
}

void MainWindow::showIntro()
{
    fallbackCopyText_ = "面向教学的 ODE 图形化计算器";
    showOutputHtml(
        "<h2>面向教学的 ODE 图形化计算器</h2>"
        "<p>当前原型聚焦一阶、二阶常系数线性非齐次方程。</p>"
        "<p>核心流程采用论文中的待定系数矩阵化思路：识别非齐次项形态，判断特征根重数，"
        "构造下三角线性方程组，求出特解并合成通解。</p>"
        + equationBlock("y'' + y = x\\cos(2x)")
        + "<p>默认示例如上，可直接点击“求解”。</p>");
}
