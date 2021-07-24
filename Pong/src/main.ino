/*
  Pong for Arduboy
*/
#include <Arduboy2.h>
#include <fonts/Font3x5.h>

Arduboy2 arduboy;

Font3x5 font3x5 = Font3x5();

int defaultPlayerWidth = 4;
int defaultPlayerHeight = 16;

enum class GameState : uint8_t {
  Menu,
  GamePlay,
  MultiPlayerMenu,
  GameOver
};

GameState gameState = GameState::Menu;

enum class GameMode : uint8_t {
  SinglePlayer,
  MultiPlayerHost,
  MultiPlayerJoin
};

GameMode gameMode = GameMode::SinglePlayer;

enum class GamePhase : uint8_t {
  Start,
  Play,
  Scored,
  Paused
};

GamePhase gamePhase = GamePhase::Start;

constexpr uint8_t startMenuOptions = 2;  // single and multi player
constexpr uint8_t startMenuMinIndex = 0;
constexpr uint8_t startMenuMaxIndex = startMenuOptions - 1;

constexpr uint8_t multiplayerMenuOptions = 2;
constexpr uint8_t multiplayerMenuMinIndex = 0;
constexpr uint8_t multiplayerMenuMaxIndex = multiplayerMenuOptions - 1;

uint8_t selectedMenuIndex = 0;

uint8_t displayFrameCount = 0;
constexpr uint8_t frameRate = 30;


class Player {
  public:
    int score;
    int x;
    int y;
    int width;
    int height;
    int stepSize = 2;
    
    Player(int x) {
      this->score = 0;
      this->x = x;
      this->width = defaultPlayerWidth;
      this->height = defaultPlayerHeight;
      this->y = (HEIGHT - this->height) / 2;
    }
    
    void up() {
      if (this->y - this->stepSize > 0) {
        this->y -= this->stepSize;
      } else {
        this->y = 0;
      }
    }
    
    void down() {
      if (this->y + this->height + this->stepSize < HEIGHT) {
        this->y += this->stepSize;
      } else {
        this->y = HEIGHT - this->height;
      }
    }

    void reset() {
      this->score = 0;
      this->y = (HEIGHT - this->height) / 2;
    }
    
    void draw() {
      arduboy.fillRect(this->x, this->y, this->width, this->height);
    }
};

Player player1(0);
Player player2(WIDTH - defaultPlayerWidth);

class Ball {
  public:
    int x;
    int y;
    int dim = 6;
    int vx = 3;
    int vy = 3;
    
    Ball() {
      this->x = (WIDTH - this->dim) / 2;
      this->y = (HEIGHT - this->dim) / 2;
    }

    void reset() {
      this->x = (WIDTH - this->dim) / 2;
      this->y = random(this->dim, HEIGHT - 2 * this->dim);
      this->vx *= -1;
    }

    void draw() {
      arduboy.fillRect(this->x, this->y, this->dim, this->dim);
    }
};

Ball ball;


void setup() {
  Serial.begin(9600);
  arduboy.begin();
  arduboy.setFrameRate(frameRate);
  arduboy.initRandomSeed();
}

void loop() {
  if (arduboy.nextFrame()) {
    arduboy.pollButtons();

    update();    

    arduboy.clear();

    draw();
    
    arduboy.display();
  }
}

void update() {
  // update game logic
  switch (gameState) {
    case GameState::Menu:
      updateMenu();
      break;
    case GameState::GamePlay:
      updateGamePlay();
      break;
    case GameState::MultiPlayerMenu:
      updateMultiPlayerMenu();
      break;
    case GameState::GameOver:
      updateGameOver();
      break;
  }
}

void updateMenu() {
  if (arduboy.justPressed(UP_BUTTON)) {
    if (selectedMenuIndex > startMenuMinIndex) {
      selectedMenuIndex--;
    }
  }
  if (arduboy.justPressed(DOWN_BUTTON)) {
    if (selectedMenuIndex < startMenuMaxIndex) {
      selectedMenuIndex++;
    }
  }
  if (arduboy.justPressed(A_BUTTON)) {
    if (selectedMenuIndex == 0) {
      gameMode = GameMode::SinglePlayer;
      gameState = GameState::GamePlay;
      gamePhase = GamePhase::Start;
    } else if (selectedMenuIndex == 1) {
      gameState = GameState::MultiPlayerMenu;
    }
    
    selectedMenuIndex = 0;
  }
}

void updateMultiPlayerMenu() {
  if (arduboy.justPressed(UP_BUTTON)) {
    if (selectedMenuIndex > multiplayerMenuMinIndex) {
      selectedMenuIndex--;
    }
  }
  if (arduboy.justPressed(DOWN_BUTTON)) {
    if (selectedMenuIndex < multiplayerMenuMaxIndex) {
      selectedMenuIndex++;
    }
  }
  if (arduboy.justPressed(A_BUTTON)) {
    switch (selectedMenuIndex) {
      case 0: 
        gameMode = GameMode::MultiPlayerHost;
        gameState = GameState::GamePlay;
        gamePhase = GamePhase::Start;
        break;
      case 1:
        gameMode = GameMode::MultiPlayerJoin;
        gameState = GameState::GamePlay;
        gamePhase = GamePhase::Start;
        break;
    }
    selectedMenuIndex = 0;
  }
  if (arduboy.justPressed(B_BUTTON)) {
    selectedMenuIndex = 0;
    gameState = GameState::Menu;
  }
}

void updateGamePlay() {
  // process game
  readSerialCommands();
  if (arduboy.justPressed(B_BUTTON)) {
    gamePhase = GamePhase::Paused;
    if (gameMode == GameMode::MultiPlayerHost || gameMode == GameMode::MultiPlayerJoin) {
      Serial.println(F("Pause"));
    }
    return;
  }
  switch (gamePhase) {
    case GamePhase::Start:
      if (arduboy.justPressed(A_BUTTON) && gameMode != GameMode::MultiPlayerJoin) {
        gamePhase = GamePhase::Play;
        if (gameMode == GameMode::MultiPlayerHost) {
          Serial.println(F("Play"));
        }
      }
      break;
    case GamePhase::Play:
      switch (gameMode) {
        case GameMode::SinglePlayer:
          movePlayerWithButtons(player1);
          movePlayerBot(player2);
          moveBall();
          break;
        case GameMode::MultiPlayerHost:
          movePlayerWithButtons(player1);
          moveBall();
          // send commands with player and ball coordinates
          Serial.print(F("Player1 "));
          Serial.println(player1.y);
          Serial.print(F("Ball "));
          Serial.print(ball.x);
          Serial.print(F(" "));
          Serial.print(ball.y);
          Serial.print(F(" "));
          Serial.print(ball.vx);
          Serial.print(F(" "));
          Serial.println(ball.vy);
          break;
        case GameMode::MultiPlayerJoin:
          movePlayerWithButtons(player2);
          Serial.print(F("Player2 "));
          Serial.println(player2.y);
          break;
      }
      break;
    case GamePhase::Scored:
      if (++displayFrameCount >= 2 * frameRate) {
        displayFrameCount = 0;
        gamePhase = GamePhase::Play;
        if (gameMode == GameMode::MultiPlayerHost) {
          Serial.println(F("Continue"));
        }
      }
      break;
    case GamePhase::Paused:
      updatePaused();
      break;
  }
}

void updatePaused() {
  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
    gamePhase = GamePhase::Play;
    if (gameMode == GameMode::MultiPlayerHost || gameMode == GameMode::MultiPlayerJoin) {
      Serial.println(F("Unpause"));
    }
  }
}

void updateGameOver() {
  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
    resetGame();
    gameState = GameState::Menu;
  }
}

void draw() {
  // draw on screen
  switch (gameState) {
    case GameState::Menu:
      drawMenu();
      break;
    case GameState::GamePlay:
      drawGamePlay();
      break;
    case GameState::MultiPlayerMenu:
      drawMultiPlayerMenu();
      break;
    case GameState::GameOver:
      drawGameOver();
      break;
  }
}

void drawMenu() {
  constexpr uint8_t titleCenterX = (WIDTH - 4 * 3 * 6) / 2;
  constexpr uint8_t selectorPositionX = titleCenterX;
  constexpr uint8_t menuPositionX = selectorPositionX + 8;
  constexpr uint8_t menuPositionY = 40;
  constexpr uint8_t menuPadding = 10;
  

  arduboy.setTextSize(3);
  arduboy.setCursor(titleCenterX, 4);
  arduboy.print(F("Pong"));
  arduboy.setTextSize(1);

  font3x5.setTextColor(BLACK);

  font3x5.setCursor(menuPositionX, menuPositionY);
  arduboy.fillRect(menuPositionX - 2, menuPositionY - 1, 4 * 13 + 3, 5 + 2 * 2);
  font3x5.print(F("SINGLE PLAYER"));

  font3x5.setCursor(menuPositionX, menuPositionY + menuPadding);
  arduboy.fillRect(menuPositionX - 2, menuPositionY + menuPadding - 1, 4 * 12 + 3, 5 + 2 * 2);
  font3x5.print(F("MULTI PLAYER"));

  font3x5.setTextColor(WHITE);

  arduboy.fillCircle(selectorPositionX, menuPositionY + (menuPadding * selectedMenuIndex) + 3, 2);
}

void drawMultiPlayerMenu() {
  constexpr uint8_t titleCenterX = (WIDTH - 12 * 6) / 2;
  constexpr uint8_t selectorPositionX = (WIDTH - 26) / 2;
  constexpr uint8_t menuPositionX = selectorPositionX + 8;
  constexpr uint8_t menuPositionY = 40;
  constexpr uint8_t menuPadding = 10;

  arduboy.setCursor(titleCenterX, 4);
  arduboy.print(F("MULTI PLAYER"));

  font3x5.setTextColor(BLACK);

  font3x5.setCursor(menuPositionX, menuPositionY);
  arduboy.fillRect(menuPositionX - 2, menuPositionY - 1, 4 * 4 + 3, 5 + 2 * 2);
  font3x5.print(F("HOST"));

  font3x5.setCursor(menuPositionX, menuPositionY + menuPadding);
  arduboy.fillRect(menuPositionX - 2, menuPositionY + menuPadding - 1, 4 * 4 + 3, 5 + 2 * 2);
  font3x5.print(F("JOIN"));

  font3x5.setTextColor(WHITE);

  arduboy.fillCircle(selectorPositionX, menuPositionY + (menuPadding * selectedMenuIndex) + 3, 2);
}

void drawGamePlay() {
  switch(gamePhase) {
    case GamePhase::Start:
      player1.draw();
      player2.draw();
      ball.draw();
    case GamePhase::Play:
      player1.draw();
      player2.draw();
      ball.draw();
      break;
    case GamePhase::Scored:
      drawScored();
      break;
    case GamePhase::Paused:
      drawPaused();
      break;
  }
}

void drawPaused() {
  constexpr uint8_t titleCenterX = (WIDTH - 6 * 3 * 6) / 2;
  constexpr uint8_t titleCenterY = (HEIGHT - 8 * 3) / 2;

  arduboy.setTextSize(3);
  arduboy.setCursor(titleCenterX, titleCenterY);
  arduboy.println(F("PAUSED"));
  arduboy.setTextSize(1);
}

void drawGameOver() {
  constexpr uint8_t winCenterX = (WIDTH - 8 * 2 * 6) / 2;
  constexpr uint8_t loseCenterX = (WIDTH - 9 * 2 * 6) / 2;
  constexpr uint8_t scoreCenterX = (WIDTH - 6 * 3 * 6) / 2;

  arduboy.setTextSize(2);
  arduboy.setCursorY(4);

  switch (gameMode) {
    case GameMode::SinglePlayer:
    case GameMode::MultiPlayerHost:
      if (player1.score > player2.score) {
        arduboy.setCursorX(winCenterX);
        arduboy.println(F("YOU WON!"));
      } else {
        arduboy.setCursorX(loseCenterX);
        arduboy.println(F("YOU LOST!"));
      }
      break;
    case GameMode::MultiPlayerJoin:
      if (player1.score < player2.score) {
        arduboy.setCursorX(winCenterX);
        arduboy.println(F("YOU WON!"));
      } else {
        arduboy.setCursorX(loseCenterX);
        arduboy.println(F("YOU LOST!"));
      }
      break;
  }

  // print scores
  arduboy.setTextSize(3);
  arduboy.setCursor(scoreCenterX, 32);
  arduboy.print(player1.score);
  arduboy.print(F(" - "));
  arduboy.print(player2.score);
}

void drawScored() {
  constexpr uint8_t scoreCenterX = (WIDTH - 5 * 3 * 6) / 2;
  constexpr uint8_t scoreCenterY = (HEIGHT - 3 * 8) / 2;
  arduboy.setTextSize(3);
  arduboy.setCursor(scoreCenterX, scoreCenterY);
  arduboy.print(player1.score);
  arduboy.print(F(" - "));
  arduboy.print(player2.score);
}

void movePlayerWithButtons(Player &player) {
  if (arduboy.pressed(UP_BUTTON)) {
    player.up();
  }
  if (arduboy.pressed(DOWN_BUTTON)) {
    player.down();
  }
}

void movePlayerBot(Player &player) {
  // follow midpoint of the ball
  if (((ball.y + ball.dim)/2) < ((player.y + player.height)/2)) {
    player.up();
  } else if (((ball.y + ball.dim)/2) > ((player.y + player.height)/2)) {
    player.down();
  }
}

void moveBall() {
  ball.x += ball.vx;
  ball.y += ball.vy;

  // ball hits top or bottom border
  if (ball.y < 0) {
    ball.y = 0;
    ball.vy *= -1;
  } else if (ball.y + ball.dim > HEIGHT) {
    ball.y = HEIGHT - ball.dim;
    ball.vy *= -1;
  }

  // ball hits player
  if (arduboy.collide(Rect(ball.x, ball.y, ball.dim, ball.dim), Rect(player1.x, player1.y, player1.width, player1.height))) {
    ball.x = player1.x + player1.width;
    ball.vx *= -1;
  } else if (arduboy.collide(Rect(ball.x, ball.y, ball.dim, ball.dim), Rect(player2.x, player2.y, player2.width, player2.height))) {
    ball.x = player2.x - ball.dim;
    ball.vx *= -1;
  }

  // ball hits left or right border
  if (ball.x < 0) {
    player2.score++;
    ball.reset();
    gamePhase = GamePhase::Scored;
  } else if ((ball.x + ball.dim) > WIDTH) {
    player1.score++;
    ball.reset();
    gamePhase = GamePhase::Scored;
  }

  if (gameMode == GameMode::MultiPlayerHost) {
    if (gamePhase == GamePhase::Scored) {
      Serial.print(F("Scored "));
      Serial.print(player1.score);
      Serial.print(F(" "));
      Serial.println(player2.score);
    }
  }

  // reset score if one of the players reaches 10 points
  if ((player1.score == 10) || (player2.score == 10)) {
    gameState = GameState::GameOver;
    Serial.println(F("Game Over"));
  }
}

void resetGame() {
  player1.reset();
  player2.reset();
  ball.reset();
}

void readSerialCommands() {
  while (Serial.available()) {
    String in = Serial.readStringUntil('\n');
    if (in.startsWith(F("Player"))) {
      char playerN = in.charAt(6);
      int y = in.substring(8).toInt();
      switch (playerN) {
        case '1':
          player1.y = y;
          break;
        case '2':
          player2.y = y;
          break;
      }
    } else if (in.startsWith(F("Ball"))) {
      int spaceIndexStart = 4;
      int spaceIndexEnd = in.indexOf(' ', spaceIndexStart + 1);
      ball.x = in.substring(spaceIndexStart + 1, spaceIndexEnd).toInt();
      spaceIndexStart = spaceIndexEnd;
      spaceIndexEnd = in.indexOf(' ', spaceIndexStart + 1);
      ball.y = in.substring(spaceIndexStart + 1, spaceIndexEnd).toInt();
      spaceIndexStart = spaceIndexEnd;
      spaceIndexEnd = in.indexOf(' ', spaceIndexStart + 1);
      ball.vx = in.substring(spaceIndexStart + 1, spaceIndexEnd).toInt();
      spaceIndexStart = spaceIndexEnd;
      spaceIndexEnd = in.indexOf(' ', spaceIndexStart + 1);
      ball.vy = in.substring(spaceIndexStart + 1, spaceIndexEnd).toInt();
    } else if (in.equals(F("Play"))) {
      gamePhase = GamePhase::Play;
    } else if (in.equals(F("Pause"))) {
      gamePhase = GamePhase::Paused;
    } else if (in.equals(F("Unpause"))) {
      gamePhase = GamePhase::Play;
    } else if (in.startsWith(F("Scored"))) {
      int spaceIndexStart = 6;
      int spaceIndexEnd = in.indexOf(' ', spaceIndexStart + 1);
      player1.score = in.substring(spaceIndexStart + 1, spaceIndexEnd).toInt();
      spaceIndexStart = spaceIndexEnd;
      player2.score = in.substring(spaceIndexStart + 1).toInt();
      gamePhase = GamePhase::Scored;
    } else if (in.equals(F("Game Over"))) {
      gameState = GameState::GameOver;
    }
  }
}