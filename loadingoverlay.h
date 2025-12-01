#ifndef LOADINGOVERLAY_H
#define LOADINGOVERLAY_H

#include <QWidget>
#include <QLabel>
#include <QMovie>

class LoadingOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit LoadingOverlay(QWidget *parent = nullptr);
    ~LoadingOverlay() override;

    // Set the GIF resource path, e.g. ":/texture/loading.gif"
    void setGif(const QString &resourcePath);

public slots:
    void showOverlay();  // start GIF + show overlay
    void hideOverlay();  // stop GIF + hide overlay

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *m_label;
    QMovie *m_movie;
};

#endif // LOADINGOVERLAY_H
