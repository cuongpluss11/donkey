#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int CAR_WIDTH = 90; // Điều chỉnh kích thước xe
const int CAR_HEIGHT = 120;
const int DONKEY_WIDTH = 90; // Điều chỉnh kích thước con lừa
const int DONKEY_HEIGHT = 120;
const int COIN_WIDTH = 80;
const int COIN_HEIGHT = 80;

float car_speed = 3.0f; // Giảm tốc độ ban đầu
float donkey_speed = 3.0f;
const float ACCELERATION = 0.0005f; // Giảm gia tốc
const int SPEED_INCREASE_THRESHOLD = 10;
const float SPEED_INCREMENT = 0.2f; // Giảm tốc độ tăng thêm

bool isJumping = false;
const int JUMP_HEIGHT = 100;
int jumpProgress = 0;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* carTexture = NULL;
SDL_Texture* donkeyTexture = NULL;
SDL_Texture* coinTexture = NULL;
SDL_Texture* explosionTexture = NULL;
SDL_Texture* introTexture = NULL;
TTF_Font* font = NULL;

SDL_Rect car = {rand() % 2 == 0 ? 160 : SCREEN_WIDTH / 2, -CAR_HEIGHT, CAR_WIDTH, CAR_HEIGHT};
SDL_Rect donkey = {SCREEN_WIDTH / 2 - DONKEY_WIDTH / 2, SCREEN_HEIGHT - DONKEY_HEIGHT - 20, DONKEY_WIDTH, DONKEY_HEIGHT};
SDL_Rect coin = {rand() % (SCREEN_WIDTH - 320 - COIN_WIDTH) + 160, -COIN_HEIGHT, COIN_WIDTH, COIN_HEIGHT};

int goldCoins = 0;
int lives = 0;

SDL_Texture* loadTexture(const char* path) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, path);
    if (!texture) {
        cout << "LOi ANH " << path << "! : " << IMG_GetError() << endl;
    }
    return texture;
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        return false;
    }
    if (TTF_Init() == -1) {
        return false;
    }
    window = SDL_CreateWindow("Donkey Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        return false;
    }
    return true;
}

void renderBackground() {
    SDL_SetRenderDrawColor(renderer, 0, 128, 128, 255);
    SDL_Rect leftPanel = {0, 0, 160, SCREEN_HEIGHT};
    SDL_Rect rightPanel = {SCREEN_WIDTH - 160, 0, 160, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &leftPanel);
    SDL_RenderFillRect(renderer, &rightPanel);

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect road = {160, 0, SCREEN_WIDTH - 320, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &road);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < SCREEN_HEIGHT; i += 20) {
        SDL_RenderDrawLine(renderer, 160, i, 160 + 10, i);
        SDL_RenderDrawLine(renderer, SCREEN_WIDTH - 160 - 10, i, SCREEN_WIDTH - 160, i);
    }

    for (int i = 0; i < SCREEN_HEIGHT; i += 40) {
        SDL_RenderDrawLine(renderer, SCREEN_WIDTH / 2 - 5, i, SCREEN_WIDTH / 2 - 5, i + 20);
    }
}

void renderScore(int score, int goldCoins, int lives) {
    if (!font) {
        font = TTF_OpenFont("arial.ttf", 24);
        if (!font) {
            return;
        }
    }
    string scoreText = "Score: " + to_string(score);
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {20, 20, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    string coinText = "Gold: " + to_string(goldCoins);
    textSurface = TTF_RenderText_Solid(font, coinText.c_str(), textColor);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {SCREEN_WIDTH - 150, 20, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    string livesText = "Lives: " + to_string(lives);
    textSurface = TTF_RenderText_Solid(font, livesText.c_str(), textColor);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, 20, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

bool loadMedia() {
    carTexture = loadTexture("oto.png");
    if (!carTexture) return false;

    donkeyTexture = loadTexture("conlua.png");
    if (!donkeyTexture) return false;

    coinTexture = loadTexture("tien.png");
    if (!coinTexture) return false;

    explosionTexture = loadTexture("no.png");
    if (!explosionTexture) return false;

    introTexture = loadTexture("intro2.jpg");
    if (!introTexture) return false;

    return true;
}

void close() {
    SDL_DestroyTexture(carTexture);
    SDL_DestroyTexture(donkeyTexture);
    SDL_DestroyTexture(coinTexture);
    SDL_DestroyTexture(explosionTexture);
    SDL_DestroyTexture(introTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

bool checkCollision(SDL_Rect a, SDL_Rect b) {
    return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y);
}

void showExplosion(int x, int y) {
    SDL_Rect explosionRect = {x - 32, y - 32, 64, 64};
    SDL_RenderCopy(renderer, explosionTexture, NULL, &explosionRect);
    SDL_RenderPresent(renderer);
    SDL_Delay(500);
}

void showIntro() {
    SDL_RenderCopy(renderer, introTexture, NULL, NULL);

    if (!font) {
        font = TTF_OpenFont("arial.ttf", 24);
        if (!font) {
            return;
        }
    }
    string introText = "Press any key to start";
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, introText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, SCREEN_HEIGHT / 2 + 100, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    SDL_RenderPresent(renderer);

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                exit(0);
            } else if (e.type == SDL_KEYDOWN) {
                quit = true;
            }
        }
    }
}

bool showGameOver(int score, int goldCoins) {
    for (int i = 0; i < 10; ++i) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    for (int i = 0; i < 10; ++i) {
        SDL_SetWindowPosition(window, rand() % 10 - 5, rand() % 10 - 5);
        SDL_Delay(10);
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (!font) {
        font = TTF_OpenFont("arial.ttf", 24);
        if (!font) {
            return false;
        }
    }
    string gameOverText = "Game Over! Score: " + to_string(score) + " Gold: " + to_string(goldCoins);
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, gameOverText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, SCREEN_HEIGHT / 2 - textSurface->h, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    string replayText = "Press R to replay, Q to quit";
    textSurface = TTF_RenderText_Solid(font, replayText.c_str(), textColor);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, SCREEN_HEIGHT / 2, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    SDL_RenderPresent(renderer);

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                return false;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_r) {
                    return true;
                } else if (e.key.keysym.sym == SDLK_q) {
                    return false;
                }
            }
        }
    }
    return false;
}

void gameLoop() {
    bool quit = false;
    SDL_Event e;
    int score = 0;
    Uint32 startTime = SDL_GetTicks();

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                        donkey.x = 160;
                        break;
                    case SDLK_RIGHT:
                        donkey.x = SCREEN_WIDTH / 2;
                        break;
                    case SDLK_UP:
                        if (!isJumping) {
                            isJumping = true;
                            jumpProgress = 0;
                        }
                        break;
                }
            }
        }

        if (isJumping) {
            if (jumpProgress < JUMP_HEIGHT) {
                donkey.y -= 5;
                jumpProgress += 5;
            } else if (jumpProgress < JUMP_HEIGHT * 2) {
                donkey.y += 5;
                jumpProgress += 5;
            } else {
                isJumping = false;
                donkey.y = SCREEN_HEIGHT - DONKEY_HEIGHT - 20;
            }
        }

        Uint32 currentTime = SDL_GetTicks();
        car_speed += ACCELERATION * ((currentTime - startTime) / 1000.0f);

        car.y += (int)car_speed;
        if (car.y > SCREEN_HEIGHT) {
            car.x = rand() % 2 == 0 ? 160 : SCREEN_WIDTH / 2;
            car.y = -CAR_HEIGHT;
            score++;
        }

        coin.y += (int)car_speed;
        if (coin.y > SCREEN_HEIGHT) {
            coin.x = rand() % (SCREEN_WIDTH - 320 - COIN_WIDTH) + 160;
            coin.y = -COIN_HEIGHT;
        }

        if (checkCollision(donkey, coin)) {
            goldCoins++;
            if (goldCoins % 5 == 0) {
                lives++;
            }
            coin.x = rand() % (SCREEN_WIDTH - 320 - COIN_WIDTH) + 160;
            coin.y = -COIN_HEIGHT;
        }

        if (score % SPEED_INCREASE_THRESHOLD == 0 && score != 0) {
            car_speed += SPEED_INCREMENT;
        }

        if (checkCollision(donkey, car)) {
            if (lives > 0) {
                lives--;
                car.x = rand() % 2 == 0 ? 160 : SCREEN_WIDTH / 2;
                car.y = -CAR_HEIGHT;
            } else {
                showExplosion(donkey.x + DONKEY_WIDTH / 2, donkey.y + DONKEY_HEIGHT / 2);

                if (showGameOver(score, goldCoins)) {
                    car.x = rand() % 2 == 0 ? 160 : SCREEN_WIDTH / 2;
                    car.y = -CAR_HEIGHT;
                    donkey.x = SCREEN_WIDTH / 2 - DONKEY_WIDTH / 2;
                    donkey.y = SCREEN_HEIGHT - DONKEY_HEIGHT - 20;
                    coin.x = rand() % (SCREEN_WIDTH - 320 - COIN_WIDTH) + 160;
                    coin.y = -COIN_HEIGHT;
                    score = 0;
                    goldCoins = 0;
                    lives = 0;
                    car_speed = 3.0f;
                    startTime = SDL_GetTicks();
                } else {
                    quit = true;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        renderBackground();
        renderScore(score, goldCoins, lives);
        SDL_RenderCopy(renderer, carTexture, NULL, &car);
        SDL_RenderCopy(renderer, donkeyTexture, NULL, &donkey);
        SDL_RenderCopy(renderer, coinTexture, NULL, &coin);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    if (!init()) {
        return -1;
    }

    if (!loadMedia()) {
        close();
        return -1;
    }

    showIntro();
    gameLoop();

    close();
    return 0;
}
