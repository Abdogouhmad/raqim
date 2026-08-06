#pragma once

#include <QString>
#include <QWidget>

class QLineEdit;
class QPushButton;

class CalculatorWindow : public QWidget {
    Q_OBJECT

  public:
    explicit CalculatorWindow(QWidget* parent = nullptr);

  protected:
    void keyPressEvent(QKeyEvent* event) override;

  private:
    enum class Operation { Add, Subtract, Multiply, Divide };

    void buildUi();
    void connectDigitButton(QPushButton* button, int digit);
    void connectOperatorButton(QPushButton* button, Operation op);

    void onDigitPressed(int digit);
    void onDecimalPressed();
    void onOperatorPressed(Operation op);
    void onEqualsPressed();
    void onClearPressed();
    void onSignTogglePressed();
    void onPercentPressed();
    void onBackspacePressed();

    static QChar operatorChar(Operation op);
    void updateDisplay();

    QLineEdit* m_display = nullptr;

    // The whole expression as typed so far, e.g. "12+7*3". Stored with
    // plain ASCII operators (+  -  *  /); prettified to +  −  ×  ÷ only
    // when shown on screen. Starts at "0" the way a fresh calculator does.
    QString m_expression = "0";

    bool m_justEvaluated = false; // true right after '=' — next digit starts fresh
    bool m_hasError = false;      // true after a divide-by-zero / bad expression
};
