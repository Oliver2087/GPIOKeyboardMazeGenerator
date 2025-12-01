#include "startmenu.h"
#include "gameview.h"

#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFont>
#include <QInputDialog>

StartMenu::StartMenu(QWidget *parent)
    : QWidget(parent),
    m_startButton(new QPushButton("Start Game", this)),
    m_quitButton(new QPushButton("Quit", this)),
    m_titleLabel(new QLabel("Maze Game", this))
{
    // Title label styling
    QFont titleFont;
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // Layout: vertical stack
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(m_titleLabel);
    layout->addSpacing(20);
    layout->addWidget(m_startButton);
    layout->addWidget(m_quitButton);
    layout->addStretch();

    // Optional: fix window size a bit
    setFixedSize(400, 300);

    // Connect buttons to slots
    connect(m_startButton, &QPushButton::clicked,
            this, &StartMenu::onStartClicked);
    connect(m_quitButton, &QPushButton::clicked,
            this, &StartMenu::onQuitClicked);
}

void StartMenu::onStartClicked()
{
    // Let the player choose a character
    QStringList characters;
    characters << "Assassin" << "Robber" << "Thug";

    bool ok = false;
    QString chosen = QInputDialog::getItem(
        this,
        "Select Character",
        "Choose your character:",
        characters,
        0,          // default index
        false,      // editable? false = fixed list
        &ok
        );

    if (!ok || chosen.isEmpty()) {
        // User cancelled → stay in the menu
        return;
    }

    // Create and show the game window with the chosen character
    GameView *view = new GameView(chosen);
    view->setAttribute(Qt::WA_DeleteOnClose);
    view->show();

    // Optional: hide the menu
    this->hide();
}

void StartMenu::onQuitClicked()
{
    close();  // closes this window
    // This will exit the app if it's the last window
}
