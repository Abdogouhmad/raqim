#pragma once

#include <QColor>
#include <QFileSystemWatcher>
#include <QMap>
#include <QObject>
#include <QString>

class QVariantAnimation;

// Reads Noctalia's resolved color state (colors.json, written on every
// theme/wallpaper change) and applies it to the running QApplication as a
// QPalette + stylesheet. Watches the file so the app re-themes live when
// you switch schemes in Noctalia — no restart needed. Transitions between
// color sets are animated rather than snapped.
class ThemeManager : public QObject {
    Q_OBJECT

  public:
    explicit ThemeManager(QObject* parent = nullptr);

    // Locates colors.json, applies it if found, and starts watching it.
    // Safe to call even if Noctalia isn't running/installed: falls back
    // to the default Qt palette silently. The very first apply is instant
    // (there's nothing to transition from yet); later reloads animate.
    void applyToApplication();

  signals:
    void themeChanged();

  private slots:
    void onWatchedFileChanged(const QString& path);
    void onWatchedDirChanged(const QString& path);
    void onTransitionValueChanged(const QVariant& value);
    void onTransitionFinished();

  private:
    QString findColorsFile() const;
    QMap<QString, QColor> parseColors(const QString& path) const;
    void applyColors(const QMap<QString, QColor>& colors);
    void startTransitionTo(const QMap<QString, QColor>& target);
    void reload();
    void ensureWatchingFile();

    QFileSystemWatcher m_watcher;
    QString m_colorsFilePath;

    QVariantAnimation* m_animation = nullptr;
    QMap<QString, QColor> m_currentColors; // last fully-applied (or in-flight blended) state
    QMap<QString, QColor> m_transitionFrom;
    QMap<QString, QColor> m_transitionTo;
    bool m_hasAppliedOnce = false;
};
