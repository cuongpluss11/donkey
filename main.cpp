#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>

using namespace std;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int CAR_WIDTH = 90;
const int CAR_HEIGHT = 120;
const int DONKEY_WIDTH = 90;
const int DONKEY_HEIGHT = 120;
const int COIN_WIDTH = 80;
const int COIN_HEIGHT = 80;
const int LEFT_LANE_CENTER = 160 + ((SCREEN_WIDTH - 320) / 4);
const int RIGHT_LANE_CENTER = SCREEN_WIDTH / 2 + ((SCREEN_WIDTH - 320) / 4);
const int ROAD_STRIP_HEIGHT = 40;
const int NUM_ROAD_STRIPS = (SCREEN_HEIGHT / ROAD_STRIP_HEIGHT) + 2;
const float base_speed = 1.0f;
float current_speed = base_speed;
const int SPEED_INCREASE_INTERVAL = 10;
const float SPEED_INCREMENT = 0.5f;
const float MAX_SPEED = 7.0f;
bool isJumping = false;
const int JUMP_HEIGHT = 100;
int jumpProgress = 0;
float roadOffset = 0.0f;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* carTexture = NULL;
SDL_Texture* donkeyTexture = NULL;
SDL_Texture* coinTexture = NULL;
SDL_Texture* explosionTexture = NULL;
SDL_Texture* introTexture = NULL;
TTF_Font* font = NULL;
Mix_Music* introMusic = NULL;
Mix_Chunk* pointSound = NULL;
Mix_Chunk* collisionSound = NULL;
Mix_Chunk* countdown3Sound = nullptr;
Mix_Chunk* countdown2Sound = nullptr;
Mix_Chunk* countdown1Sound = nullptr;
Mix_Chunk* countdownStartSound = nullptr;
Mix_Chunk* engineSound = nullptr;
Mix_Chunk* startSound = nullptr;
SDL_Rect car = {SCREEN_WIDTH / 2 - CAR_WIDTH / 2, SCREEN_HEIGHT - CAR_HEIGHT - 20, CAR_WIDTH, CAR_HEIGHT};
SDL_Rect donkey = {rand() % 2 == 0 ? 160 : SCREEN_WIDTH / 2, -DONKEY_HEIGHT, DONKEY_WIDTH, DONKEY_HEIGHT};
//SDL_Rect coin = {rand() % (SCREEN_WIDTH - 320 - COIN_WIDTH) + 160, -COIN_HEIGHT, COIN_WIDTH, COIN_HEIGHT};
SDL_Rect coin = {
    (rand() % 2 == 0) ? (LEFT_LANE_CENTER - COIN_WIDTH / 2) : (RIGHT_LANE_CENTER - COIN_WIDTH / 2),
    -COIN_HEIGHT,
    COIN_WIDTH,
    COIN_HEIGHT
};
int goldCoins = 0;
int lives = 0;
// Thêm struct HitBox để quản lý vùng va chạm
struct HitBox {
    int x_offset; // Offset từ vị trí góc trái trên của object
    int y_offset;
    int width;
    int height;
};

// Cập nhật các hằng số
const HitBox CAR_HITBOX = {15, 20, CAR_WIDTH - 30, CAR_HEIGHT - 40};
const HitBox DONKEY_HITBOX = {10, 15, DONKEY_WIDTH - 20, DONKEY_HEIGHT - 30};
const HitBox COIN_HITBOX = {15, 15, COIN_WIDTH - 30, COIN_HEIGHT - 30};
// Hàm kiểm tra va chạm chính xác hơn
bool checkPreciseCollision(const SDL_Rect& object1, const HitBox& hitbox1,
                          const SDL_Rect& object2, const HitBox& hitbox2) {
    // Tính toán vị trí thực tế của hitbox
    SDL_Rect rect1 = {
        object1.x + hitbox1.x_offset,
        object1.y + hitbox1.y_offset,
        hitbox1.width,
        hitbox1.height
    };

    SDL_Rect rect2 = {
        object2.x + hitbox2.x_offset,
        object2.y + hitbox2.y_offset,
        hitbox2.width,
        hitbox2.height
    };
    // Kiểm tra va chạm giữa các hitbox
    return (rect1.x < rect2.x + rect2.w &&
            rect1.x + rect1.w > rect2.x &&
            rect1.y < rect2.y + rect2.h &&
            rect1.y + rect1.h > rect2.y);
}
struct RoadStrip {
    int y;
    int height;
};

vector<RoadStrip> roadStrips;
// Thay đổi trong phần khởi tạo coin (ở đầu file)
//SDL_Rect coin = {
//    (rand() % 2 == 0) ? (LEFT_LANE_CENTER - COIN_WIDTH / 2) : (RIGHT_LANE_CENTER - COIN_WIDTH / 2),
//    -COIN_HEIGHT,
//    COIN_WIDTH,
//    COIN_HEIGHT
//};
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
     if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
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
     for (int i = 0; i < NUM_ROAD_STRIPS; ++i) {
        RoadStrip strip;
        strip.y = i * ROAD_STRIP_HEIGHT;
        strip.height = ROAD_STRIP_HEIGHT;
        roadStrips.push_back(strip);
    }
    return true;
}
void renderBackground() {

    SDL_SetRenderDrawColor(renderer, 194, 178, 128, 255);
    SDL_Rect leftPanel = {0, 0, 160, SCREEN_HEIGHT};
    SDL_Rect rightPanel = {SCREEN_WIDTH - 160, 0, 160, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &leftPanel);
    SDL_RenderFillRect(renderer, &rightPanel);

    SDL_SetRenderDrawColor(renderer, 178, 34, 34, 255);
    SDL_Rect brickLeft = {160, 0, 10, SCREEN_HEIGHT};
    SDL_Rect brickRight = {SCREEN_WIDTH - 170, 0, 10, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &brickLeft);
    SDL_RenderFillRect(renderer, &brickRight);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < SCREEN_HEIGHT; i += 20) {
        SDL_RenderDrawLine(renderer, 160, i, 160 + 10, i);
        SDL_RenderDrawLine(renderer, SCREEN_WIDTH - 160 - 10, i, SCREEN_WIDTH - 160, i);
    }

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect road = {170, 0, SCREEN_WIDTH - 340, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &road);

    // Vẽ các vạch kẻ đường cuộn
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (const auto& strip : roadStrips) {
        if (strip.y + strip.height >= 0 && strip.y < SCREEN_HEIGHT) {
            SDL_Rect line = {SCREEN_WIDTH / 2 - 5, strip.y, 10, strip.height / 2};
            SDL_RenderFillRect(renderer, &line);
        }
    }
}

void updateRoadStrips(float speed) {
    roadOffset += speed;

    // Cập nhật vị trí các đoạn đường kẻ
    for (auto& strip : roadStrips) {
        strip.y += (int)speed;

        // Nếu đoạn đường kẻ đi qua màn hình, đặt lại phía trên
        if (strip.y > SCREEN_HEIGHT) {
            strip.y = -ROAD_STRIP_HEIGHT;
        }
    }

    if (roadOffset >= ROAD_STRIP_HEIGHT) {
        roadOffset -= ROAD_STRIP_HEIGHT;
    }
}
void countdown() {
    Mix_HaltMusic();
    if (!font) {
        font = TTF_OpenFont("Arial.ttf", 64); // Đảm bảo font được tải với kích thước 64
        if (!font) {
            cout << "Failed to load font! SDL_ttf Error: " << TTF_GetError() << endl;
            return;
        }
    }

    for (int i = 3; i > 0; --i) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        string countText = to_string(i);
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, countText.c_str(), textColor);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, SCREEN_HEIGHT / 2 - textSurface->h / 2, textSurface->w, textSurface->h};
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
            switch (i) {
            case 3:
                Mix_PlayChannel(-1, countdown3Sound, 0);
                break;
            case 2:
                Mix_PlayChannel(-1, countdown2Sound, 0);
                break;
            case 1:
                Mix_PlayChannel(-1, countdown1Sound, 0);
                break;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(1000); // Chờ 1 giây
    }

    // Hiển thị "Start!"
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    string startText = "Start!!!";
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, startText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, SCREEN_HEIGHT / 2 - textSurface->h / 2, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    Mix_PlayChannel(-1, startSound, 0);
    Mix_PlayChannel(-1, engineSound, -1);
    SDL_RenderPresent(renderer);
    SDL_Delay(500); // Chờ 0.5 giây
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
        introMusic = Mix_LoadMUS("amthanhintro.mp3");
    if (!introMusic) return false;
     pointSound = Mix_LoadWAV("amthanhkhitinh1diem.wav");
    if (!pointSound) return false;
        collisionSound = Mix_LoadWAV("vacham.wav");
    if (!collisionSound) return false;
     countdown3Sound = Mix_LoadWAV("3.wav");
    if (!countdown3Sound) return false;
     countdown2Sound = Mix_LoadWAV("2.wav");
    if (!countdown2Sound) return false;
     countdown1Sound = Mix_LoadWAV("1.wav");
    if (!countdown3Sound) return false;
     engineSound = Mix_LoadWAV("dongco.wav");
    if (!engineSound) return false;
    Mix_VolumeChunk(engineSound, MIX_MAX_VOLUME /8);
    startSound = Mix_LoadWAV("start.wav");
    if (!startSound) return false;
//    // Thay thế grassTexture bằng mảng texture
//for (int i = 0; i < GRASS_FRAMES; i++) {
//    string path = "grass" + to_string(i+1) + ".png"; // Giả sử có 4 file grass_1.png đến grass_4.png
//    grassTextures[i] = loadTexture(path.c_str());
//    if (!grassTextures[i]) return false;
//}
//grassTexture = loadTexture("grass4.png");  // Thêm file grass.png vào thư mục assets
//if (!grassTexture) return false;
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
    Mix_FreeMusic(introMusic);
    Mix_FreeChunk(pointSound);
    Mix_FreeChunk(collisionSound);
    Mix_FreeChunk(countdown3Sound);
    Mix_FreeChunk(countdown2Sound);
    Mix_FreeChunk(countdown1Sound);
    Mix_FreeChunk(engineSound);
    Mix_FreeChunk(startSound);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    Mix_Quit();
    SDL_Quit();
}
//
//bool checkCollision(SDL_Rect a, SDL_Rect b) {
//    return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y);
//}

void showExplosion(int x, int y) {
    SDL_Rect explosionRect = {x - 32, y - 32, 64, 64};
    SDL_RenderCopy(renderer, explosionTexture, NULL, &explosionRect);
    SDL_RenderPresent(renderer);
    SDL_Delay(500);
}

void showIntro() {
    Mix_PlayMusic(introMusic, -1);
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
                    Mix_HaltMusic();
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
current_speed = base_speed;
//int lastSpeedIncreaseScore = 0;
//int lastSpeedIncreaseAt = -SPEED_INCREASE_THRESHOLD;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
    case SDLK_LEFT:
        car.x = LEFT_LANE_CENTER - CAR_WIDTH / 2; // Giữa làn trái
        break;
    case SDLK_RIGHT:
        car.x = RIGHT_LANE_CENTER - CAR_WIDTH / 2; // Giữa làn phải
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
                car.y -= 5;
                jumpProgress += 5;
            } else if (jumpProgress < JUMP_HEIGHT * 2) {
                car.y += 5;
                jumpProgress += 5;
            } else {
                isJumping = false;
                car.y = SCREEN_HEIGHT - CAR_HEIGHT - 20;
            }
        }
  updateRoadStrips(current_speed);

        donkey.y += (int)current_speed;
        if (donkey.y > SCREEN_HEIGHT) {
                donkey.x = (rand() % 2 == 0) ? (LEFT_LANE_CENTER - DONKEY_WIDTH / 2)
                             : (RIGHT_LANE_CENTER - DONKEY_WIDTH / 2);

            donkey.y = -DONKEY_HEIGHT;
            score++;
             Mix_PlayChannel(-1, pointSound, 0);
if (score > 0 && score % SPEED_INCREASE_INTERVAL == 0 &&
    donkey.y == -DONKEY_HEIGHT &&
    current_speed < MAX_SPEED) {
    current_speed += SPEED_INCREMENT;
    if (current_speed > MAX_SPEED) {
        current_speed = MAX_SPEED;
    }
}
            }
        coin.y += (int)current_speed;
        if (coin.y > SCREEN_HEIGHT) {
                coin.x = (rand() % 2 == 0) ? (LEFT_LANE_CENTER - COIN_WIDTH / 2)
                           : (RIGHT_LANE_CENTER - COIN_WIDTH / 2);
            coin.y = -COIN_HEIGHT;
        }

//        if (checkCollision(car, coin)) {
if (checkPreciseCollision(car, CAR_HITBOX, coin, COIN_HITBOX)) {
            goldCoins++;
            if (goldCoins % 5 == 0) {
                lives++;
            }
           coin.x = (rand() % 2 == 0) ? (LEFT_LANE_CENTER - COIN_WIDTH / 2)
                               : (RIGHT_LANE_CENTER - COIN_WIDTH / 2);
            coin.y = -COIN_HEIGHT;

        }

//        if (score % SPEED_INCREASE_INTERVAL == 0 && score != 0) {
//            current_speed += SPEED_INCREMENT;
//        }

//        if (checkCollision(car, donkey)) {
if (checkPreciseCollision(car, CAR_HITBOX, donkey, DONKEY_HITBOX)) {
            if (lives > 0) {
                lives--;
                donkey.x = rand() % 2 == 0 ? 160 : SCREEN_WIDTH / 2;
                donkey.y = -DONKEY_HEIGHT;
            } else {
                showExplosion(car.x + CAR_WIDTH / 2, car.y + CAR_HEIGHT / 2);
                 Mix_HaltChannel(-1);
                 Mix_PlayChannel(-1, collisionSound, 0);
                if (showGameOver(score, goldCoins)) {
                    donkey.x = rand() % 2 == 0 ? 160 : SCREEN_WIDTH / 2;
                    donkey.y = -DONKEY_HEIGHT;
                    car.x = SCREEN_WIDTH / 2 - CAR_WIDTH / 2;
                    car.y = SCREEN_HEIGHT - CAR_HEIGHT - 20;
                    coin.x = rand() % (SCREEN_WIDTH - 320 - COIN_WIDTH) + 160;
                    coin.y = -COIN_HEIGHT;
                    score = 0;
                    goldCoins = 0;
                    lives = 0;
                    current_speed = 3.0f;
                    Mix_PlayChannel(-1, engineSound, -1);
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
    countdown();
    gameLoop();

    close();
    return 0;
}
// them phan co cuon xuong o to co the nhay qua trong 20 diem dau tien
// xu li random sao cho lua va xu khong chong vao nhau
