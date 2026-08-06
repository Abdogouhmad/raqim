#include "calculatorwindow.h"

#include <QFont>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <cmath>

namespace {
constexpr int kButtonMinSize = 56;

bool isOperatorChar(QChar c) { return c == '+' || c == '-' || c == '*' || c == '/'; }

// Finds where the "current" (rightmost) number segment begins in an
// expression string — i.e. the index right after the last real binary
// operator. A '-' that immediately follows another operator is a unary
// sign for that segment (from the +/- button), not a binary operator
// itself, so it's skipped rather than treated as a split point.
// Example: "12+-5" -> real operator is '+' at index 2, so this returns 3,
// giving a segment of "-5" (the unary minus stays part of the segment).
int currentSegmentStart(const QString& expression) {
    for (int i = expression.length() - 1; i > 0; --i) {
        const QChar c = expression.at(i);
        if (!isOperatorChar(c))
            continue;
        if (c == '-' && isOperatorChar(expression.at(i - 1)))
            continue; // unary, keep scanning left
        return i + 1;
    }
    return 0;
}

// A small recursive-descent parser for the four basic operators with
// standard precedence (* and / bind tighter than + and -), a leading unary
// minus per factor, and postfix '%'. No parentheses — the UI never offers
// them.
//
//   expression := term (('+' | '-') term)*
//   term       := factor (('*' | '/') factor)*
//   factor     := ['-'] number '%'*
//
// '%' follows classic calculator behaviour:
//   - "a + b%" and "a - b%" -> b% is a percentage of a ("1000+10%" = 1100)
//   - "a * b%" and "a / b%" -> b% is b/100
//   - a leading "b%"        -> b/100
class ExpressionParser {
  public:
    explicit ExpressionParser(const QString& text) : m_text(text) {}

    double parse(bool* ok) {
        *ok = true;
        const double result = parseExpression(ok);
        if (*ok && !atEnd())
            *ok = false; // leftover characters -> malformed input
        return result;
    }

  private:
    double parseExpression(bool* ok) {
        double result = parseTerm(ok);
        if (*ok && m_lastTermWasPercent)
            result /= 100.0; // leading "50%" -> 0.5, there is no base yet
        while (*ok && !atEnd() && (peek() == '+' || peek() == '-')) {
            const QChar op = peek();
            advance();
            double rhs = parseTerm(ok);
            const bool rhsWasPercent = m_lastTermWasPercent;
            if (!*ok)
                return 0.0;
            if (rhsWasPercent)
                rhs = result * rhs / 100.0; // "a + b%" -> b% of a
            result = (op == '+') ? result + rhs : result - rhs;
        }
        return result;
    }

    double parseTerm(bool* ok) {
        double result = parseFactor(ok);
        bool lhsIsPercent = m_factorIsPercent;
        bool termIsPercent = lhsIsPercent;
        while (*ok && !atEnd() && (peek() == '*' || peek() == '/')) {
            const QChar op = peek();
            advance();
            double rhs = parseFactor(ok);
            const bool rhsIsPercent = m_factorIsPercent;
            if (!*ok)
                return 0.0;
            if (rhsIsPercent)
                rhs /= 100.0; // within * and /, % always means /100
            if (lhsIsPercent)
                result /= 100.0; // "100% * 2" -> 1 * 2
            if (op == '/') {
                if (rhs == 0.0) {
                    *ok = false;
                    return 0.0;
                }
                result /= rhs;
            } else {
                result *= rhs;
            }
            lhsIsPercent = false;
            termIsPercent = false;
        }
        m_lastTermWasPercent = termIsPercent;
        return result;
    }

    double parseFactor(bool* ok) {
        bool negative = false;
        if (!atEnd() && peek() == '-') {
            negative = true;
            advance();
        }
        double value = parseNumber(ok);
        m_factorIsPercent = false;
        if (*ok) {
            while (!atEnd() && peek() == '%') {
                advance();
                if (m_factorIsPercent)
                    value /= 100.0; // "10%%" -> 0.1 / 100
                m_factorIsPercent = true;
            }
        }
        return negative ? -value : value;
    }

    double parseNumber(bool* ok) {
        const int start = m_pos;
        while (!atEnd() && (peek().isDigit() || peek() == '.'))
            advance();
        if (m_pos == start) {
            *ok = false;
            return 0.0;
        }
        bool convOk = false;
        const double value = m_text.mid(start, m_pos - start).toDouble(&convOk);
        if (!convOk)
            *ok = false;
        return value;
    }

    bool atEnd() const { return m_pos >= m_text.length(); }
    QChar peek() const { return m_text.at(m_pos); }
    void advance() { ++m_pos; }

    QString m_text;
    int m_pos = 0;
    bool m_lastTermWasPercent = false; // did the most recent term end in '%'?
    bool m_factorIsPercent = false;    // does the most recent factor carry '%'?
};

double evaluateExpression(const QString& expression, bool* ok) {
    ExpressionParser parser(expression);
    return parser.parse(ok);
}

} // namespace

CalculatorWindow::CalculatorWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Calculator");
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

QChar CalculatorWindow::operatorChar(Operation op) {
    switch (op) {
    case Operation::Add:
        return '+';
    case Operation::Subtract:
        return '-';
    case Operation::Multiply:
        return '*';
    case Operation::Divide:
        return '/';
    }
    return '+';
}

void CalculatorWindow::onDigitPressed(int digit) {
    if (m_hasError || m_justEvaluated) {
        m_expression = QString::number(digit);
        m_hasError = false;
        m_justEvaluated = false;
        updateDisplay();
        return;
    }

    const QString seg = m_expression.mid(currentSegmentStart(m_expression));
    if (seg.endsWith('%')) {
        m_expression.chop(seg.length()); // a %-locked number is done; a digit restarts it
    } else if (seg == "0" || seg == "-0") {
        // Replace just the trailing zero, keeping a leading '-' if present.
        m_expression.chop(1);
    }
    m_expression += QString::number(digit);
    updateDisplay();
}

void CalculatorWindow::onDecimalPressed() {
    if (m_hasError || m_justEvaluated) {
        m_expression = "0.";
        m_hasError = false;
        m_justEvaluated = false;
        updateDisplay();
        return;
    }

    const QString seg = m_expression.mid(currentSegmentStart(m_expression));
    if (seg.endsWith('%')) {
        m_expression.chop(seg.length());
        m_expression += "0.";
    } else if (seg.isEmpty() || seg == "-") {
        m_expression += "0.";
    } else if (!seg.contains('.')) {
        m_expression += '.';
    }
    updateDisplay();
}

void CalculatorWindow::onOperatorPressed(Operation op) {
    if (m_hasError)
        onClearPressed();
    m_justEvaluated = false;

    const QChar opChar = operatorChar(op);

    // Special-case starting a negative number from a totally fresh state.
    if ((m_expression == "0" || m_expression == "0%") && opChar == '-') {
        m_expression = "-";
        updateDisplay();
        return;
    }

    const int segStart = currentSegmentStart(m_expression);
    const QString seg = m_expression.mid(segStart);

    if (seg.isEmpty()) {
        // Right after an operator, nothing typed yet for the next number.
        if (opChar == '-') {
            // Toggle a pending unary minus for the number about to be typed.
            if (!m_expression.isEmpty() && m_expression.back() == '-') {
                m_expression.chop(1);
            } else {
                m_expression += '-';
            }
        } else if (!m_expression.isEmpty()) {
            // Swap the trailing operator instead of stacking a second one.
            if (m_expression.back() == '-' && m_expression.size() >= 2 &&
                isOperatorChar(m_expression.at(m_expression.size() - 2))) {
                m_expression.chop(2); // drop operator + its pending unary minus
            } else {
                m_expression.chop(1);
            }
            m_expression += opChar;
        }
    } else if (seg == "-") {
        // A lone unary minus with nothing typed after it yet.
        m_expression.chop(1);
        if (opChar != '-')
            m_expression += opChar;
    } else {
        m_expression += opChar;
    }
    updateDisplay();
}

void CalculatorWindow::onEqualsPressed() {
    if (m_hasError || m_justEvaluated)
        return;

    QString expr = m_expression;
    while (!expr.isEmpty() && isOperatorChar(expr.back()))
        expr.chop(1); // drop a trailing incomplete operator
    if (expr.isEmpty())
        return;

    bool ok = false;
    const double result = evaluateExpression(expr, &ok);

    if (!ok || std::isnan(result) || std::isinf(result)) {
        m_hasError = true;
        m_display->setText("Error");
        return;
    }

    m_expression = QString::number(result, 'g', 15);
    m_justEvaluated = true;
    updateDisplay();
}

void CalculatorWindow::onClearPressed() {
    m_expression = "0";
    m_justEvaluated = false;
    m_hasError = false;
    updateDisplay();
}

void CalculatorWindow::onSignTogglePressed() {
    if (m_hasError)
        return;

    if (m_justEvaluated) {
        if (m_expression.startsWith('-')) {
            m_expression.remove(0, 1);
        } else if (m_expression != "0") {
            m_expression.prepend('-');
        }
        updateDisplay();
        return;
    }

    const int segStart = currentSegmentStart(m_expression);
    const QString seg = m_expression.mid(segStart);
    if (seg.isEmpty()) {
        m_expression += '-'; // pre-negate the number about to be typed
    } else if (seg.startsWith('-')) {
        m_expression.remove(segStart, 1);
    } else {
        m_expression.insert(segStart, '-');
    }
    updateDisplay();
}

void CalculatorWindow::onPercentPressed() {
    if (m_hasError)
        return;

    if (m_justEvaluated) {
        m_expression = QString::number(m_expression.toDouble() / 100.0, 'g', 15);
        m_justEvaluated = false;
        updateDisplay();
        return;
    }

    // Keep the '%' visible inline in the expression ("1000+10%"); the parser
    // turns it into "10% of 1000" when '=' is pressed. Pressing '%' again on
    // the same number toggles it back off.
    const int segStart = currentSegmentStart(m_expression);
    const QString seg = m_expression.mid(segStart);
    if (seg.isEmpty() || seg == "-")
        return; // nothing typed yet to mark as a percentage

    if (seg.endsWith('%')) {
        m_expression.chop(1);
    } else {
        m_expression += '%';
    }
    updateDisplay();
}

void CalculatorWindow::onBackspacePressed() {
    if (m_hasError || m_justEvaluated) {
        onClearPressed();
        return;
    }
    if (!m_expression.isEmpty())
        m_expression.chop(1);
    if (m_expression.isEmpty())
        m_expression = "0";
    updateDisplay();
}

void CalculatorWindow::updateDisplay() {
    if (m_hasError) {
        m_display->setText("Error");
        return;
    }
    QString pretty = m_expression;
    pretty.replace('*', QChar(0x00D7)); // ×
    pretty.replace('/', QChar(0x00F7)); // ÷
    pretty.replace('-', QChar(0x2212)); // − (proper minus sign, not hyphen)
    m_display->setText(pretty.isEmpty() ? "0" : pretty);
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
    case Qt::Key_Percent:
        onPercentPressed();
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
