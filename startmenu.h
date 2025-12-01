#ifndef STARTMENU_H
#define STARTMENU_H

#include <QWidget>

class QPushButton;
class QLabel;

class StartMenu : public QWidget
{
    Q_OBJECT

public:
    explicit StartMenu(QWidget *parent = nullptr);

private slots:
    void onStartClicked();
    void onQuitClicked();

private:
    QPushButton *m_startButton;
    QPushButton *m_quitButton;
    QLabel *m_titleLabel;
};

#endif // STARTMENU_H
