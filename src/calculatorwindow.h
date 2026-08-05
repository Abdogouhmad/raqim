#pragma once

#include <QString>
#include <QWidget>

class QKeyEvent;
class QLineEdit;
class QPushButton;

class CalculatorWindow : public QWidget {
    Q_OBJECT

  public:
    enum class Operation {
        None,
        Add,
        Subtract,
        Multiply,
        Divide,
    };

    explicit CalculatorWindow(QWidget* parent = nullptr);

  protected:
    void keyPressEvent(QKeyEvent* event) override;

  private slots:
    void onDigitPressed(int digit);
    void onDecimalPressed();
    void onOperatorPressed(Operation op);
    void onEqualsPressed();
    void onClearPressed();
    void onSignTogglePressed();
    void onPercentPressed();
    void onBackspacePressed();

  private:
    void buildUi();
    void connectDigitButton(QPushButton* button, int digit);
    void connectOperatorButton(QPushButton* button, Operation op);
    double currentValue() const;
    double applyPendingOperation(double lhs, double rhs, Operation op) const;
    void updateDisplay();

    QLineEdit* m_display = nullptr;
    QString m_entryText = "0";
    double m_accumulator = 0.0;
    Operation m_pendingOp = Operation::None;
    bool m_awaitingNewValue = true;
};
