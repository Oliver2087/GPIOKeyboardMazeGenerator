#include "loadingoverlay.h"
#include <QResizeEvent>

LoadingOverlay::LoadingOverlay(QWidget *parent)
    : QWidget(parent),
    m_label(new QLabel(this)),
    m_movie(nullptr)
{
    // Semi-transparent dark background
    setStyleSheet("background-color: rgba(0, 0, 0, 200);");
    setAttribute(Qt::WA_StyledBackground, true);

    // Fill parent if available
    if (parent) {
        setGeometry(parent->rect());
    }

    // Center the label inside
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setGeometry(rect());

    hide(); // hidden by default
}

LoadingOverlay::~LoadingOverlay()
{

}

void LoadingOverlay::setGif(const QString &resourcePath)
{
    if (m_movie) {
        m_movie->stop();
        m_movie->deleteLater();
        m_movie = nullptr;
    }

    m_movie = new QMovie(resourcePath, QByteArray(), this);
    m_movie->setCacheMode(QMovie::CacheAll);

    // Force infinite looping: when the movie finishes, restart it
    connect(m_movie, &QMovie::finished, this, [this]() {
        if (this->isVisible() && m_movie) {
            m_movie->start();
        }
    });

    m_label->setMovie(m_movie);
}

void LoadingOverlay::showOverlay()
{
    if (m_movie) {
        m_movie->start();
    }

    if (parentWidget()) {
        setGeometry(parentWidget()->rect());
    }
    m_label->setGeometry(rect());

    if (m_movie) {
        m_movie->setScaledSize(size());
        m_movie->start();
    }

    raise();
    show();
}

void LoadingOverlay::hideOverlay()
{
    if (m_movie) {
        m_movie->stop();
    }
    hide();
}

void LoadingOverlay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_label->setGeometry(rect());

    if (m_movie) {
        m_movie->setScaledSize(size());   // <-- SCALE GIF TO OVERLAY SIZE
    }
}
