#include "MainWindow.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QUrl>

#include <cmath>

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
    resize(1180, 760);

    auto* central = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(14, 14, 14, 14);

    auto* splitter = new QSplitter(Qt::Horizontal, central);
    rootLayout->addWidget(splitter);

    auto* left = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 10, 0);
    leftLayout->setSpacing(12);

    auto* formulaGroup = new QGroupBox("公式输入", left);
    auto* formulaLayout = new QVBoxLayout(formulaGroup);
    formulaLayout->setSpacing(8);

    auto* formulaInputLayout = new QHBoxLayout();
    formulaEdit_ = new QLineEdit(formulaGroup);
    formulaEdit_->setPlaceholderText("例如：y''+y=x*cos(2x) 或 y''-2y'+y=x*e^x");
    formulaSolveButton_ = new QPushButton("解析并求解", formulaGroup);
    formulaInputLayout->addWidget(formulaEdit_, 1);
    formulaInputLayout->addWidget(formulaSolveButton_);
    formulaLayout->addLayout(formulaInputLayout);

    formulaPreview_ = new QWebEngineView(formulaGroup);
    formulaPreview_->setMinimumHeight(124);
    formulaPreview_->setMaximumHeight(150);
    formulaLayout->addWidget(formulaPreview_);
    formulaPreviewTimer_ = new QTimer(this);
    formulaPreviewTimer_->setSingleShot(true);
    formulaPreviewTimer_->setInterval(180);

    formulaPreviewStatus_ = new QLabel(formulaGroup);
    formulaPreviewStatus_->setWordWrap(true);
    formulaPreviewStatus_->setProperty("previewStatus", true);
    formulaLayout->addWidget(formulaPreviewStatus_);

    auto* formulaButtonGrid = new QGridLayout();
    formulaButtonGrid->setHorizontalSpacing(6);
    formulaButtonGrid->setVerticalSpacing(6);

    struct FormulaButton {
        const char* label;
        const char* token;
        int cursorOffset;
    };

    const FormulaButton buttons[] = {
        {"y", "y", 0}, {"y'", "y'", 0}, {"y''", "y''", 0}, {"x", "x", 0},
        {"x^2", "x^2", 0}, {"x^3", "x^3", 0},
        {"e^x", "e^(x)", 0}, {"sin", "sin(x)", 0}, {"cos", "cos(x)", 0},
        {"(", "(", 0}, {")", ")", 0}, {"=", "=", 0},
        {"+", "+", 0}, {"-", "-", 0}, {"*", "*", 0}, {"/", "/", 0},
        {"1/2", "1/2", 0}, {"清空", "", 0},
    };

    const int buttonCount = static_cast<int>(sizeof(buttons) / sizeof(buttons[0]));
    for (int i = 0; i < buttonCount; ++i) {
        auto* button = new QPushButton(QString::fromUtf8(buttons[i].label), formulaGroup);
        button->setProperty("formulaKey", true);
        button->setMinimumHeight(30);
        const QString token = QString::fromUtf8(buttons[i].token);
        const int cursorOffset = buttons[i].cursorOffset;
        if (QString::fromUtf8(buttons[i].label) == "清空") {
            connect(button, &QPushButton::clicked, formulaEdit_, &QLineEdit::clear);
        } else {
            connect(button, &QPushButton::clicked, this, [this, token, cursorOffset]() {
                insertFormulaToken(token, cursorOffset);
            });
        }
        formulaButtonGrid->addWidget(button, i / 6, i % 6);
    }
    formulaLayout->addLayout(formulaButtonGrid);
    leftLayout->addWidget(formulaGroup);

    auto* equationGroup = new QGroupBox("方程", left);
    auto* equationLayout = new QFormLayout(equationGroup);
    orderCombo_ = new QComboBox(equationGroup);
    orderCombo_->addItem("一阶", 1);
    orderCombo_->addItem("二阶", 2);
    equationLayout->addRow("阶数", orderCombo_);

    a1Label_ = new QLabel("a1", equationGroup);
    a1Edit_ = new QLineEdit(equationGroup);
    a1Edit_->setPlaceholderText("y'' + a1 y' + a0 y");
    equationLayout->addRow(a1Label_, a1Edit_);

    a0Edit_ = new QLineEdit(equationGroup);
    equationLayout->addRow("a0", a0Edit_);

    equationPreview_ = new QLabel(equationGroup);
    equationPreview_->setWordWrap(true);
    equationPreview_->setFrameShape(QFrame::StyledPanel);
    equationPreview_->setMinimumHeight(54);
    equationLayout->addRow("标准型", equationPreview_);
    leftLayout->addWidget(equationGroup);

    auto* forcingGroup = new QGroupBox("非齐次项", left);
    auto* forcingLayout = new QVBoxLayout(forcingGroup);
    kindCombo_ = new QComboBox(forcingGroup);
    kindCombo_->addItem("P(x)e^(rx)", static_cast<int>(NonHomogeneousKind::PolynomialExp));
    kindCombo_->addItem("e^(ax)(R(x)cos bx + I(x)sin bx)", static_cast<int>(NonHomogeneousKind::ExpTrig));
    forcingLayout->addWidget(kindCombo_);

    forcingStack_ = new QStackedWidget(forcingGroup);

    auto* expPage = new QWidget(forcingStack_);
    auto* expLayout = new QFormLayout(expPage);
    expREdit_ = new QLineEdit(expPage);
    polynomialEdit_ = new QLineEdit(expPage);
    polynomialEdit_->setPlaceholderText("最高次到常数项，如 1, 0, -2；支持 1/3");
    expLayout->addRow("r", expREdit_);
    expLayout->addRow("P(x) 系数", polynomialEdit_);
    forcingStack_->addWidget(expPage);

    auto* trigPage = new QWidget(forcingStack_);
    auto* trigLayout = new QFormLayout(trigPage);
    trigAEdit_ = new QLineEdit(trigPage);
    trigBEdit_ = new QLineEdit(trigPage);
    cosinePolynomialEdit_ = new QLineEdit(trigPage);
    sinePolynomialEdit_ = new QLineEdit(trigPage);
    cosinePolynomialEdit_->setPlaceholderText("R(x)，最高次到常数项；支持分数");
    sinePolynomialEdit_->setPlaceholderText("I(x)，最高次到常数项；支持分数");
    trigLayout->addRow("a", trigAEdit_);
    trigLayout->addRow("b", trigBEdit_);
    trigLayout->addRow("R(x) 系数", cosinePolynomialEdit_);
    trigLayout->addRow("I(x) 系数", sinePolynomialEdit_);
    forcingStack_->addWidget(trigPage);

    forcingLayout->addWidget(forcingStack_);
    leftLayout->addWidget(forcingGroup);

    auto* buttonLayout = new QHBoxLayout();
    solveButton_ = new QPushButton("求解", left);
    resetButton_ = new QPushButton("示例", left);
    copyButton_ = new QPushButton("复制结果", left);
    buttonLayout->addWidget(solveButton_);
    buttonLayout->addWidget(resetButton_);
    buttonLayout->addWidget(copyButton_);
    leftLayout->addLayout(buttonLayout);
    leftLayout->addStretch();

    output_ = new QWebEngineView(splitter);
    output_->setMinimumWidth(620);

    splitter->addWidget(left);
    splitter->addWidget(output_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({390, 790});

    setCentralWidget(central);

    setStyleSheet(
        "QWidget { font-family: 'Microsoft YaHei UI', 'Segoe UI'; font-size: 10.5pt; }"
        "QGroupBox { font-weight: 600; border: 1px solid #cfd6df; border-radius: 6px; margin-top: 10px; padding: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        "QLineEdit, QComboBox { padding: 6px; border: 1px solid #c4ccd6; border-radius: 4px; }"
        "QPushButton { padding: 8px 12px; border: 1px solid #9ba8b7; border-radius: 4px; background: #f7f9fb; }"
        "QPushButton[formulaKey='true'] { padding: 5px 8px; min-width: 42px; background: #fbfcfd; }"
        "QPushButton:hover { background: #eef3f8; }"
        "QPushButton:pressed { background: #e1e8f0; }"
        "QWebEngineView { border: 1px solid #cfd6df; border-radius: 6px; background: white; }"
        "QLabel[previewStatus='true'] { color: #667586; font-size: 9pt; min-height: 18px; }"
        "QLabel[frameShape='6'] { background: #fafafa; padding: 8px; }");
}

void MainWindow::connectSignals()
{
    connect(orderCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::updateEquationState);
    connect(kindCombo_, qOverload<int>(&QComboBox::currentIndexChanged), forcingStack_, &QStackedWidget::setCurrentIndex);
    connect(kindCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::updateEquationState);
    connect(a1Edit_, &QLineEdit::textChanged, this, &MainWindow::updateEquationState);
    connect(a0Edit_, &QLineEdit::textChanged, this, &MainWindow::updateEquationState);
    connect(formulaSolveButton_, &QPushButton::clicked, this, &MainWindow::solveFormulaInput);
    connect(formulaEdit_, &QLineEdit::returnPressed, this, &MainWindow::solveFormulaInput);
    connect(formulaEdit_, &QLineEdit::textChanged, this, &MainWindow::scheduleFormulaPreviewUpdate);
    connect(formulaPreviewTimer_, &QTimer::timeout, this, &MainWindow::updateFormulaPreview);
    connect(solveButton_, &QPushButton::clicked, this, &MainWindow::solveCurrentInput);
    connect(resetButton_, &QPushButton::clicked, this, &MainWindow::resetExample);
    connect(copyButton_, &QPushButton::clicked, this, &MainWindow::copyResult);
}

void MainWindow::resetExample()
{
    orderCombo_->setCurrentIndex(1);
    a1Edit_->setText("0");
    a0Edit_->setText("1");

    kindCombo_->setCurrentIndex(1);
    trigAEdit_->setText("0");
    trigBEdit_->setText("2");
    cosinePolynomialEdit_->setText("1, 0");
    sinePolynomialEdit_->setText("0");

    expREdit_->setText("0");
    polynomialEdit_->setText("1, 0, -2");
    formulaEdit_->setText("y''+y=x*cos(2x)");
    updateEquationState();
    updateFormulaPreview();
}

void MainWindow::updateEquationState()
{
    const int order = orderCombo_->currentData().toInt();
    const bool secondOrder = order == 2;
    a1Label_->setVisible(secondOrder);
    a1Edit_->setVisible(secondOrder);
    equationPreview_->setText(equationPreview());
}

void MainWindow::solveCurrentInput()
{
    bool ok = false;
    SolverInput input = makeInput(ok);
    if (!ok) {
        return;
    }

    lastResult_ = solver_.solve(input);
    if (!lastResult_.ok) {
        fallbackCopyText_ = lastResult_.error;
        showOutputHtml("<h2>输入需要调整</h2><p class='error'>"
            + lastResult_.error.toHtmlEscaped() + "</p>");
        return;
    }
    fallbackCopyText_.clear();
    showOutputHtml(lastResult_.html);
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

bool MainWindow::readRational(QLineEdit* edit, const QString& name, Rational& value)
{
    QString error;
    if (!Rational::parse(edit->text(), value, &error)) {
        QMessageBox::warning(this, "输入错误", name + " 需要是整数、小数或分数。\n" + error);
        edit->setFocus();
        edit->selectAll();
        return false;
    }
    return true;
}

bool MainWindow::readCoefficients(QLineEdit* edit, const QString& name, std::vector<Rational>& ascending)
{
    QString text = edit->text().trimmed();
    text.replace("，", ",");
    text.replace("；", ",");
    text.replace(";", ",");
    text.replace("\n", ",");

    const QStringList tokens = text.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
    if (tokens.isEmpty()) {
        QMessageBox::warning(this, "输入错误", name + " 至少需要一个系数。");
        edit->setFocus();
        return false;
    }

    std::vector<Rational> descending;
    descending.reserve(tokens.size());
    for (const QString& token : tokens) {
        Rational value;
        QString error;
        if (!Rational::parse(token, value, &error)) {
            QMessageBox::warning(this, "输入错误", name + " 中的 \"" + token + "\" 不是有效有理数。\n" + error);
            edit->setFocus();
            return false;
        }
        descending.push_back(value);
    }

    ascending.assign(descending.rbegin(), descending.rend());
    while (ascending.size() > 1 && ascending.back().isZero()) {
        ascending.pop_back();
    }
    return true;
}

SolverInput MainWindow::makeInput(bool& ok)
{
    ok = false;
    SolverInput input;
    input.order = orderCombo_->currentData().toInt();
    if (input.order == 2) {
        if (!readRational(a1Edit_, "a1", input.a1)) {
            return input;
        }
    }
    if (!readRational(a0Edit_, "a0", input.a0)) {
        return input;
    }

    input.kind = static_cast<NonHomogeneousKind>(kindCombo_->currentData().toInt());
    if (input.kind == NonHomogeneousKind::PolynomialExp) {
        if (!readRational(expREdit_, "r", input.expR)) {
            return input;
        }
        if (!readCoefficients(polynomialEdit_, "P(x) 系数", input.polynomialAscending)) {
            return input;
        }
    } else {
        if (!readRational(trigAEdit_, "a", input.trigA)) {
            return input;
        }
        if (!readRational(trigBEdit_, "b", input.trigB)) {
            return input;
        }
        if (!readCoefficients(cosinePolynomialEdit_, "R(x) 系数", input.cosinePolynomialAscending)) {
            return input;
        }
        if (!readCoefficients(sinePolynomialEdit_, "I(x) 系数", input.sinePolynomialAscending)) {
            return input;
        }
    }

    ok = true;
    return input;
}

QString MainWindow::equationPreview() const
{
    if (orderCombo_->currentData().toInt() == 1) {
        return "y' + (" + a0Edit_->text().trimmed() + ")y = f(x)";
    }
    return "y'' + (" + a1Edit_->text().trimmed() + ")y' + ("
        + a0Edit_->text().trimmed() + ")y = f(x)";
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
