#include "calculatorwindow.h"

#include <QFont>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <limits>

namespace {
constexpr int kButtonMinSize = 56;
}

CalculatorWindow::CalculatorWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Calculator " APP_VERSION);
    buildUi();
    updateDisplay();
}

void CalculatorWindow::buildUi() {
    m_display = new QLineEdit("0");
    m_display->setReadOnly(true);
    m_display->setAlignment(Qt::AlignRight);
    m_display->setMinimumHeight(48);
    QFont displayFont = m_display->font();
    displayFont.setPointSize(20);
    m_display->setFont(displayFont);

    auto* grid = new QGridLayout;
    grid->setSpacing(12);

    auto makeButton = [this](const QString& label) {
        auto* btn = new QPushButton(label);
        btn->setMinimumSize(kButtonMinSize, kButtonMinSize);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QFont f = btn->font();
        f.setPointSize(14);
        btn->setFont(f);
        return btn;
    };

    auto* btnClear = makeButton("C");
    btnClear->setObjectName("clearButton");
    auto* btnSign = makeButton("+/-");
    auto* btnPercent = makeButton("%");
    auto* btnDivide = makeButton("÷");
    btnDivide->setObjectName("operatorButton");

    auto* btn7 = makeButton("7");
    auto* btn8 = makeButton("8");
    auto* btn9 = makeButton("9");
    auto* btnMultiply = makeButton("×");
    btnMultiply->setObjectName("operatorButton");

    auto* btn4 = makeButton("4");
    auto* btn5 = makeButton("5");
    auto* btn6 = makeButton("6");
    auto* btnSubtract = makeButton("−");
    btnSubtract->setObjectName("operatorButton");

    auto* btn1 = makeButton("1");
    auto* btn2 = makeButton("2");
    auto* btn3 = makeButton("3");
    auto* btnAdd = makeButton("+");
    btnAdd->setObjectName("operatorButton");

    auto* btn0 = makeButton("0");
    auto* btnDecimal = makeButton(".");
    auto* btnBackspace = makeButton("⌫");
    auto* btnEquals = makeButton("=");
    btnEquals->setObjectName("equalsButton");

    grid->addWidget(btnClear, 0, 0);
    grid->addWidget(btnSign, 0, 1);
    grid->addWidget(btnPercent, 0, 2);
    grid->addWidget(btnDivide, 0, 3);

    grid->addWidget(btn7, 1, 0);
    grid->addWidget(btn8, 1, 1);
    grid->addWidget(btn9, 1, 2);
    grid->addWidget(btnMultiply, 1, 3);

    grid->addWidget(btn4, 2, 0);
    grid->addWidget(btn5, 2, 1);
    grid->addWidget(btn6, 2, 2);
    grid->addWidget(btnSubtract, 2, 3);

    grid->addWidget(btn1, 3, 0);
    grid->addWidget(btn2, 3, 1);
    grid->addWidget(btn3, 3, 2);
    grid->addWidget(btnAdd, 3, 3);

    grid->addWidget(btn0, 4, 0, 1, 2);
    grid->addWidget(btnDecimal, 4, 2);
    grid->addWidget(btnBackspace, 4, 3);

    grid->addWidget(btnEquals, 5, 0, 1, 4);

    for (int col = 0; col < 4; ++col)
        grid->setColumnStretch(col, 1);
    for (int row = 0; row < 6; ++row)
        grid->setRowStretch(row, 1);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(16);
    rootLayout->addWidget(m_display);
    rootLayout->addLayout(grid);

    connectDigitButton(btn0, 0);
    connectDigitButton(btn1, 1);
    connectDigitButton(btn2, 2);
    connectDigitButton(btn3, 3);
    connectDigitButton(btn4, 4);
    connectDigitButton(btn5, 5);
    connectDigitButton(btn6, 6);
    connectDigitButton(btn7, 7);
    connectDigitButton(btn8, 8);
    connectDigitButton(btn9, 9);

    connectOperatorButton(btnAdd, Operation::Add);
    connectOperatorButton(btnSubtract, Operation::Subtract);
    connectOperatorButton(btnMultiply, Operation::Multiply);
    connectOperatorButton(btnDivide, Operation::Divide);

    connect(btnDecimal, &QPushButton::clicked, this, &CalculatorWindow::onDecimalPressed);
    connect(btnEquals, &QPushButton::clicked, this, &CalculatorWindow::onEqualsPressed);
    connect(btnClear, &QPushButton::clicked, this, &CalculatorWindow::onClearPressed);
    connect(btnSign, &QPushButton::clicked, this, &CalculatorWindow::onSignTogglePressed);
    connect(btnPercent, &QPushButton::clicked, this, &CalculatorWindow::onPercentPressed);
    connect(btnBackspace, &QPushButton::clicked, this, &CalculatorWindow::onBackspacePressed);
}

void CalculatorWindow::connectDigitButton(QPushButton* button, int digit) {
    connect(button, &QPushButton::clicked, this, [this, digit] { onDigitPressed(digit); });
}

void CalculatorWindow::connectOperatorButton(QPushButton* button, Operation op) {
    connect(button, &QPushButton::clicked, this, [this, op] { onOperatorPressed(op); });
}

void CalculatorWindow::onDigitPressed(int digit) {
    if (m_awaitingNewValue) {
        m_entryText = QString::number(digit);
        m_awaitingNewValue = false;
    } else if (m_entryText == "0") {
        m_entryText = QString::number(digit);
    } else {
        m_entryText += QString::number(digit);
    }
    updateDisplay();
}

void CalculatorWindow::onDecimalPressed() {
    if (m_awaitingNewValue) {
        m_entryText = "0.";
        m_awaitingNewValue = false;
    } else if (!m_entryText.contains('.')) {
        m_entryText += '.';
    }
    updateDisplay();
}

void CalculatorWindow::onOperatorPressed(Operation op) {
    if (m_pendingOp != Operation::None && !m_awaitingNewValue) {
        m_accumulator = applyPendingOperation(m_accumulator, currentValue(), m_pendingOp);
    } else {
        m_accumulator = currentValue();
    }
    m_pendingOp = op;
    m_awaitingNewValue = true;
    m_entryText = QString::number(m_accumulator, 'g', 15);
    updateDisplay();
}

void CalculatorWindow::onEqualsPressed() {
    if (m_pendingOp == Operation::None)
        return;
    const double result = applyPendingOperation(m_accumulator, currentValue(), m_pendingOp);
    m_pendingOp = Operation::None;
    m_awaitingNewValue = true;
    m_entryText = QString::number(result, 'g', 15);
    updateDisplay();
}

void CalculatorWindow::onClearPressed() {
    m_accumulator = 0.0;
    m_entryText = "0";
    m_pendingOp = Operation::None;
    m_awaitingNewValue = true;
    updateDisplay();
}

void CalculatorWindow::onSignTogglePressed() {
    if (m_entryText.startsWith('-')) {
        m_entryText.remove(0, 1);
    } else if (m_entryText != "0") {
        m_entryText.prepend('-');
    }
    updateDisplay();
}

void CalculatorWindow::onPercentPressed() {
    m_entryText = QString::number(currentValue() / 100.0, 'g', 15);
    updateDisplay();
}

void CalculatorWindow::onBackspacePressed() {
    if (m_awaitingNewValue)
        return;
    m_entryText.chop(1);
    if (m_entryText.isEmpty() || m_entryText == "-") {
        m_entryText = "0";
        m_awaitingNewValue = true;
    }
    updateDisplay();
}

double CalculatorWindow::currentValue() const { return m_entryText.toDouble(); }

double CalculatorWindow::applyPendingOperation(double lhs, double rhs, Operation op) const {
    switch (op) {
    case Operation::Add:
        return lhs + rhs;
    case Operation::Subtract:
        return lhs - rhs;
    case Operation::Multiply:
        return lhs * rhs;
    case Operation::Divide:
        return rhs != 0.0 ? lhs / rhs : std::numeric_limits<double>::quiet_NaN();
    case Operation::None:
        return rhs;
    }
    return rhs;
}

void CalculatorWindow::updateDisplay() {
    if (m_entryText == "nan" || m_entryText == "-nan") {
        m_display->setText("Error");
        return;
    }
    m_display->setText(m_entryText);
}

void CalculatorWindow::keyPressEvent(QKeyEvent* event) {
    const QString text = event->text();
    if (text.size() == 1 && text[0].isDigit()) {
        onDigitPressed(text[0].digitValue());
        return;
    }
    switch (event->key()) {
    case Qt::Key_Plus:
        onOperatorPressed(Operation::Add);
        return;
    case Qt::Key_Minus:
        onOperatorPressed(Operation::Subtract);
        return;
    case Qt::Key_Asterisk:
        onOperatorPressed(Operation::Multiply);
        return;
    case Qt::Key_Slash:
        onOperatorPressed(Operation::Divide);
        return;
    case Qt::Key_Period:
    case Qt::Key_Comma:
        onDecimalPressed();
        return;
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Equal:
        onEqualsPressed();
        return;
    case Qt::Key_Backspace:
        onBackspacePressed();
        return;
    case Qt::Key_Escape:
        onClearPressed();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}
