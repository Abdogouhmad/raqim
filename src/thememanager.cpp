#include "thememanager.h"

#include <QApplication>
#include <QDir>
#include <QEasingCurve>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QSet>
#include <QStringList>
#include <QStyleFactory>
#include <QVariantAnimation>

namespace {

constexpr int kTransitionMs = 320;

// Noctalia (v4/v5) writes its currently-resolved color state here, and
// rewrites it every time you change scheme, wallpaper (Material You mode),
// or light/dark mode. Older docs/install layouts sometimes used the
// quickshell-prefixed path instead, so we check both.
QStringList candidateColorsPaths() {
    const QString configHome =
        qEnvironmentVariable("XDG_CONFIG_HOME", QDir::homePath() + "/.config");
    return {
        configHome + "/noctalia/colors.json",
        configHome + "/quickshell/noctalia/colors.json",
    };
}

QColor colorOr(const QMap<QString, QColor>& colors, const QString& key, const QColor& fallback) {
    return colors.value(key, fallback);
}

QColor lerpColor(const QColor& a, const QColor& b, qreal t) {
    auto lerpChannel = [t](int from, int to) { return from + static_cast<int>((to - from) * t); };
    return QColor(lerpChannel(a.red(), b.red()), lerpChannel(a.green(), b.green()),
                  lerpChannel(a.blue(), b.blue()), lerpChannel(a.alpha(), b.alpha()));
}

} // namespace

ThemeManager::ThemeManager(QObject* parent) : QObject(parent) {
    // Fusion honors QPalette consistently across platforms; native styles
    // often ignore several of the roles we set. Do this once — not per
    // frame — since swapping QStyle repeatedly during an animation is
    // wasteful and can flicker.
    qApp->setStyle(QStyleFactory::create("Fusion"));

    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this,
            &ThemeManager::onWatchedFileChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this,
            &ThemeManager::onWatchedDirChanged);

    m_animation = new QVariantAnimation(this);
    m_animation->setDuration(kTransitionMs);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    connect(m_animation, &QVariantAnimation::valueChanged, this,
            &ThemeManager::onTransitionValueChanged);
    connect(m_animation, &QVariantAnimation::finished, this, &ThemeManager::onTransitionFinished);
}

QString ThemeManager::findColorsFile() const {
    for (const QString& path : candidateColorsPaths()) {
        if (QFileInfo::exists(path))
            return path;
    }
    return QString();
}

QMap<QString, QColor> ThemeManager::parseColors(const QString& path) const {
    QMap<QString, QColor> result;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return result;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return result;

    QJsonObject root = doc.object();
    // Palette *definition* files nest variants under "dark"/"light"; the
    // live resolved colors.json is flat. Handle both just in case.
    if (root.contains("dark") && root.value("dark").isObject()) {
        root = root.value("dark").toObject();
    }

    static const QStringList roleKeys = {
        "mPrimary",  "mOnPrimary",  "mSecondary",      "mOnSecondary",
        "mTertiary", "mOnTertiary", "mError",          "mOnError",
        "mSurface",  "mOnSurface",  "mSurfaceVariant", "mOnSurfaceVariant",
        "mOutline",  "mShadow",     "mHover",          "mOnHover",
    };

    for (const QString& key : roleKeys) {
        const QString hex = root.value(key).toString();
        if (hex.isEmpty())
            continue;
        const QColor color(hex);
        if (color.isValid())
            result.insert(key, color);
    }

    return result;
}

void ThemeManager::applyColors(const QMap<QString, QColor>& colors) {
    if (colors.isEmpty())
        return;

    const QColor surface = colorOr(colors, "mSurface", QColor("#1e1e2e"));
    const QColor onSurface = colorOr(colors, "mOnSurface", QColor("#cdd6f4"));
    const QColor surfaceVariant = colorOr(colors, "mSurfaceVariant", surface.lighter(115));
    const QColor onSurfaceVariant = colorOr(colors, "mOnSurfaceVariant", onSurface.darker(115));
    const QColor primary = colorOr(colors, "mPrimary", QColor("#89b4fa"));
    const QColor onPrimary = colorOr(colors, "mOnPrimary", surface);
    const QColor error = colorOr(colors, "mError", QColor("#f38ba8"));
    const QColor onError = colorOr(colors, "mOnError", surface);
    const QColor hover = colorOr(colors, "mHover", surfaceVariant.lighter(110));
    const QColor outline = colorOr(colors, "mOutline", onSurfaceVariant);

    QPalette palette;
    palette.setColor(QPalette::Window, surface);
    palette.setColor(QPalette::WindowText, onSurface);
    palette.setColor(QPalette::Base, surfaceVariant);
    palette.setColor(QPalette::AlternateBase, surface);
    palette.setColor(QPalette::Text, onSurface);
    palette.setColor(QPalette::Button, surfaceVariant);
    palette.setColor(QPalette::ButtonText, onSurface);
    palette.setColor(QPalette::ToolTipBase, surface);
    palette.setColor(QPalette::ToolTipText, onSurface);
    palette.setColor(QPalette::Highlight, primary);
    palette.setColor(QPalette::HighlightedText, onPrimary);
    palette.setColor(QPalette::PlaceholderText, onSurfaceVariant);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, outline);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, outline);
    qApp->setPalette(palette);

    const QString qss =
        QString(R"(
        QWidget { background-color: %1; color: %2; }
        QPushButton {
            background-color: %3;
            color: %2;
            border: none;
            border-radius: 12px;
            padding: 10px;
        }
        QPushButton:hover { background-color: %8; }
        QPushButton:pressed { background-color: %4; }
        QPushButton#operatorButton, QPushButton#equalsButton {
            background-color: %4;
            color: %5;
            font-weight: 600;
        }
        QPushButton#operatorButton:hover, QPushButton#equalsButton:hover {
            background-color: %4;
            border: 2px solid %5;
        }
        QPushButton#clearButton {
            background-color: %6;
            color: %7;
        }
        QLineEdit {
            background-color: transparent;
            border: none;
            color: %2;
        }
    )")
            .arg(surface.name(), onSurface.name(), surfaceVariant.name(), primary.name(),
                 onPrimary.name(), error.name(), onError.name(), hover.name());
    qApp->setStyleSheet(qss);
}

void ThemeManager::ensureWatchingFile() {
    if (m_colorsFilePath.isEmpty())
        return;
    if (!m_watcher.files().contains(m_colorsFilePath) && QFileInfo::exists(m_colorsFilePath)) {
        m_watcher.addPath(m_colorsFilePath);
    }
}

void ThemeManager::startTransitionTo(const QMap<QString, QColor>& target) {
    if (target.isEmpty())
        return;

    if (!m_hasAppliedOnce) {
        // Nothing to animate from yet (app just started, window likely
        // isn't shown) — snap straight to it.
        m_currentColors = target;
        applyColors(target);
        m_hasAppliedOnce = true;
        emit themeChanged();
        return;
    }

    m_animation->stop(); // in case a previous transition was still running
    m_transitionFrom = m_currentColors;
    m_transitionTo = target;
    m_animation->start();
}

void ThemeManager::onTransitionValueChanged(const QVariant& value) {
    const qreal t = value.toReal();
    QMap<QString, QColor> blended;
    const QStringList keys =
        QSet<QString>(m_transitionFrom.keyBegin(), m_transitionFrom.keyEnd())
            .unite(QSet<QString>(m_transitionTo.keyBegin(), m_transitionTo.keyEnd()))
            .values();
    for (const QString& key : keys) {
        const QColor from = m_transitionFrom.value(key, m_transitionTo.value(key));
        const QColor to = m_transitionTo.value(key, from);
        blended.insert(key, lerpColor(from, to, t));
    }
    m_currentColors = blended;
    applyColors(blended);
}

void ThemeManager::onTransitionFinished() {
    m_currentColors = m_transitionTo;
    applyColors(m_currentColors); // guarantees an exact final state, not just t≈1
    emit themeChanged();
}

void ThemeManager::reload() {
    if (m_colorsFilePath.isEmpty())
        return;
    const auto colors = parseColors(m_colorsFilePath);
    startTransitionTo(colors);
    ensureWatchingFile();
}

void ThemeManager::applyToApplication() {
    m_colorsFilePath = findColorsFile();
    if (m_colorsFilePath.isEmpty())
        return; // Noctalia not present: keep default Qt theme

    startTransitionTo(parseColors(m_colorsFilePath));

    m_watcher.addPath(m_colorsFilePath);
    // Watch the containing directory too: some writers replace the file
    // atomically (write temp + rename), which drops a direct file watch.
    m_watcher.addPath(QFileInfo(m_colorsFilePath).absolutePath());
}

void ThemeManager::onWatchedFileChanged(const QString& path) {
    Q_UNUSED(path);
    reload();
}

void ThemeManager::onWatchedDirChanged(const QString& path) {
    Q_UNUSED(path);
    reload();
}
