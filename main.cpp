#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>

using namespace std;
// cac hang so dinh nghia kich thuoc man hinh , doi tuong game
// cac thong so ve diem , toc do , va cham
// bien toan cuc quan li trang thai game
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int CAR_WIDTH = 90;
const int CAR_HEIGHT = 120;
const int DONKEY_WIDTH = 90;
const int DONKEY_HEIGHT = 120;
const int COIN_WIDTH = 80;
const int COIN_HEIGHT = 80;
const int GRASS_WIDTH = 30;
const int GRASS_HEIGHT = 30;
const int GRASS_FRAMES = 4;
const int GRASS_SPAWN_CHANCE = 1;
const int LEFT_LANE_CENTER = 160 + ((SCREEN_WIDTH - 320) / 4);
const int RIGHT_LANE_CENTER = SCREEN_WIDTH / 2 + ((SCREEN_WIDTH - 320) / 4);
const int ROAD_STRIP_HEIGHT = 40;
const int NUM_ROAD_STRIPS = (SCREEN_HEIGHT / ROAD_STRIP_HEIGHT) + 2;
const float base_speed = 3.0f;
const int SCORE_INCREMENT = 1;
float current_speed = base_speed;
const int SPEED_INCREASE_INTERVAL = 10;
const float SPEED_INCREMENT = 2.0f;
const float MAX_SPEED = 15.0f;
bool coinCollected = false;
bool isOnGround = true;
const string HIGHSCORE_FILE = "highscore.txt";
const int BRICK_WIDTH = 10;
const int BRICK_HEIGHT = 20;
const int NUM_BRICK_ROWS = (SCREEN_HEIGHT / BRICK_HEIGHT) + 2;
int jumpProgress = 0;
float roadOffset = 0.0f;
const int GRASS_SPAWN_INTERVAL = 120; // frames
int grassSpawnTimer = 0;
Uint32 invincibleUntil = 0;
// cac con tro SDL de quan li cua so renderer texture va am thanh
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* carTexture = NULL;
SDL_Texture* donkeyTexture = NULL;
SDL_Texture* coinTexture = NULL;
SDL_Texture* explosionTexture = NULL;
SDL_Texture* introTexture = NULL;
SDL_Texture* gameOverTexture = NULL;
// mang texture cho cac frame co
SDL_Texture* grassTextures[GRASS_FRAMES];
TTF_Font* font = NULL;
// cac bien am thanh hieu ung
Mix_Music* introMusic = NULL;
Mix_Chunk* pointSound = NULL;
Mix_Chunk* collisionSound = NULL;
Mix_Chunk* countdown3Sound = NULL;
Mix_Chunk* countdown2Sound = NULL;
Mix_Chunk* countdown1Sound = NULL;
Mix_Chunk* countdownStartSound = NULL;
Mix_Chunk* engineSound = NULL;
Mix_Chunk* startSound = NULL;
Mix_Chunk* grassSound = NULL;
Mix_Chunk* coinSound = NULL;
Mix_Chunk* collisionSmallSound = NULL;
Mix_Chunk* extralifeSound= NULL;
// cau truc du lieu game
// cac hinh chu nhat rect dinh vi vi tri cac hinh chu nhat , xu
SDL_Rect car = {SCREEN_WIDTH / 2 - CAR_WIDTH / 2, SCREEN_HEIGHT - CAR_HEIGHT - 20, CAR_WIDTH, CAR_HEIGHT};
SDL_Rect donkey = {rand() % 2 == 0 ? 160 : SCREEN_WIDTH / 2, -DONKEY_HEIGHT, DONKEY_WIDTH, DONKEY_HEIGHT};
SDL_Rect coin = {
    (rand() % 2 == 0) ? (LEFT_LANE_CENTER - COIN_WIDTH / 2) : (RIGHT_LANE_CENTER - COIN_WIDTH / 2),
    -COIN_HEIGHT,
    COIN_WIDTH,
    COIN_HEIGHT
};
// cau truc quan li cac hang gach ben duong
struct BrickRow {
    int y;
    bool hasLine;
};

vector<BrickRow> leftBrickRows;
vector<BrickRow> rightBrickRows;
// cau truc co voi cac amination frame quan li co
struct Grass {
    SDL_Rect rect;
    int frame;
    int frameCounter;
    bool active;
    bool scored;
    bool isDangerous;
};

vector<Grass> grasses;
int goldCoins = 0;
int lives = 0;
// cau truc vung cam tranh sinh cac vat chong len nhau
struct ForbiddenZone {
    SDL_Rect rect;
    int framesRemaining;
};

vector<ForbiddenZone> forbiddenZones;
bool isSafeToSpawn(const SDL_Rect& newObj) {
    for (const auto& zone : forbiddenZones) {
        if (SDL_HasIntersection(&newObj, &zone.rect)) {
            return false;
        }
    }
    return true;
}
// cau truc hitbox cho cac doi tuong ( nho hon kich thuoc that )
struct HitBox {
    int x_offset;
    int y_offset;
    int width;
    int height;
};

const HitBox CAR_HITBOX = {15, 20, CAR_WIDTH - 30, CAR_HEIGHT - 40};
const HitBox DONKEY_HITBOX = {10, 15, DONKEY_WIDTH - 20, DONKEY_HEIGHT - 30};
const HitBox COIN_HITBOX = {15, 15, COIN_WIDTH - 30, COIN_HEIGHT - 30};
const HitBox GRASS_HITBOX = {5, 5, GRASS_WIDTH - 10, GRASS_HEIGHT - 10};
bool checkPreciseCollision(const SDL_Rect& object1, const HitBox& hitbox1,
                          const SDL_Rect& object2, const HitBox& hitbox2) {
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

    return (rect1.x < rect2.x + rect2.w &&
            rect1.x + rect1.w > rect2.x &&
            rect1.y < rect2.y + rect2.h &&
            rect1.y + rect1.h > rect2.y);
}
// cau truc vao cac vach ke duong o giua
struct RoadStrip {
    int y;
    int height;
};

vector<RoadStrip> roadStrips;
// texture de tai anh
SDL_Texture* loadTexture(const char* path) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, path);
    if (!texture) {
        cout << "LOi ANH " << path << "! : " << IMG_GetError() << endl;
    }
    return texture;
}
// ham doc ghi diem cao
int readHighScore() {
    ifstream file(HIGHSCORE_FILE);
    int highScore = 0;
    if (file.is_open()) {
        file >> highScore;
        file.close();
    }
    return highScore;
}

void writeHighScore(int score) {
    ofstream file(HIGHSCORE_FILE);
    if (file.is_open()) {
        file << score;
        file.close();
    }
}
//ham khoi tao cac doi tuong trong sdl
bool init() {
    if (SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
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
    // tao cua so
    window = SDL_CreateWindow("Donkey Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        return false;
    }
    // tao vach ke duong
    for (int i = 0; i < NUM_ROAD_STRIPS; ++i) {
        RoadStrip strip;
        strip.y = i * ROAD_STRIP_HEIGHT;
        strip.height = ROAD_STRIP_HEIGHT;
        roadStrips.push_back(strip);
    }
    // tao tuong gach 2 ben
    for (int i = 0; i < NUM_BRICK_ROWS; ++i) {
        BrickRow row;
        row.y = i * BRICK_HEIGHT;
        row.hasLine = (i % 2 == 0);
        leftBrickRows.push_back(row);
    }
    for (int i = 0; i < NUM_BRICK_ROWS; ++i) {
        BrickRow row;
        row.y = i * BRICK_HEIGHT;
        row.hasLine = (i % 2 == 0);
        rightBrickRows.push_back(row);
    }
    return true;
}
// ham de tuong gach di chuyen
void updateBrickRows(float speed) {
    for (auto& row : leftBrickRows) {
        row.y += (int)speed;
        if (row.y > SCREEN_HEIGHT) {
            row.y = -BRICK_HEIGHT;
            row.hasLine = !row.hasLine;
        }
    }
    for (auto& row : rightBrickRows) {
        row.y += (int)speed;
        if (row.y > SCREEN_HEIGHT) {
            row.y = -BRICK_HEIGHT;
            row.hasLine = !row.hasLine;
        }
    }
}
void updateRoadStrips(float speed) {
    roadOffset += speed;

    for (auto& strip : roadStrips) {
        strip.y += (int)speed;

        if (strip.y > SCREEN_HEIGHT) {
            strip.y = -ROAD_STRIP_HEIGHT;
        }
    }

    if (roadOffset >= ROAD_STRIP_HEIGHT) {
        roadOffset -= ROAD_STRIP_HEIGHT;
    }
}
// ham hien thi giao dien cho man hinh choi chinh
void renderBackground() {
    SDL_SetRenderDrawColor(renderer, 194, 178, 128, 255);
    SDL_Rect leftPanel = {0, 0, 160, SCREEN_HEIGHT};
    SDL_Rect rightPanel = {SCREEN_WIDTH - 160, 0, 160, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &leftPanel);
    SDL_RenderFillRect(renderer, &rightPanel);
    for (const auto& row : leftBrickRows) {
        if (row.y + BRICK_HEIGHT >= 0 && row.y < SCREEN_HEIGHT) {
            SDL_SetRenderDrawColor(renderer, 178, 34, 34, 255);
            SDL_Rect brickRect = {160, row.y, BRICK_WIDTH, BRICK_HEIGHT};
            SDL_RenderFillRect(renderer, &brickRect);
            if (row.hasLine) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawLine(renderer,
                                  160, row.y + BRICK_HEIGHT/2,
                                  160 + BRICK_WIDTH, row.y + BRICK_HEIGHT/2);
            }
        }
    }
    for (const auto& row : rightBrickRows) {
        if (row.y + BRICK_HEIGHT >= 0 && row.y < SCREEN_HEIGHT) {
            SDL_SetRenderDrawColor(renderer, 178, 34, 34, 255);
            SDL_Rect brickRect = {SCREEN_WIDTH - 170, row.y, BRICK_WIDTH, BRICK_HEIGHT};
            SDL_RenderFillRect(renderer, &brickRect);
            if (row.hasLine) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawLine(renderer,
                                  SCREEN_WIDTH - 170, row.y + BRICK_HEIGHT/2,
                                  SCREEN_WIDTH - 170 + BRICK_WIDTH, row.y + BRICK_HEIGHT/2);
            }
        }
    }
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect road = {170, 0, SCREEN_WIDTH - 340, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &road);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (const auto& strip : roadStrips) {
        if (strip.y + strip.height >= 0 && strip.y < SCREEN_HEIGHT) {
            SDL_Rect line = {SCREEN_WIDTH / 2 - 5, strip.y, 10, strip.height / 2};
            SDL_RenderFillRect(renderer, &line);
        }
    }
}

// ham tao hieu ung countdown truoc khi vao tro choi
void countdown() {
    Mix_HaltMusic();
TTF_Font* countdownFont = TTF_OpenFont("comic.ttf", 80);
    if (!countdownFont) {
        TTF_GetError();
        return;
    }
    for (int i = 3; i > 0; --i) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        string countText = to_string(i);
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface* textSurface = TTF_RenderText_Solid(countdownFont, countText.c_str(), textColor);
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
        SDL_Delay(1000);
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    string startText = "Start!!!";
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(countdownFont, startText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, SCREEN_HEIGHT / 2 - textSurface->h / 2, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    Mix_PlayChannel(-1, startSound, 0);
    Mix_PlayChannel(-1, engineSound, -1);
    SDL_RenderPresent(renderer);
    SDL_Delay(500);
}
void renderScore(int score, int goldCoins, int lives, int highScore) {
    TTF_Font* normalFont = TTF_OpenFont("arial.ttf", 24);
    TTF_Font* smallFont = TTF_OpenFont("arial.ttf", 18);
    if (!normalFont) normalFont = TTF_OpenFont("arial.ttf", 24);
    if (!smallFont) smallFont = normalFont;

    SDL_Color textColor = {255, 255, 255, 255};

    string scoreText = "Score: " + to_string(score);
    SDL_Surface* textSurface = TTF_RenderText_Solid(normalFont, scoreText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {20, 20, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    string highScoreText = "High Score: " + to_string(highScore);
    textSurface = TTF_RenderText_Solid(smallFont, highScoreText.c_str(), textColor);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {20, 50, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    string coinText = "Gold: " + to_string(goldCoins);
    textSurface = TTF_RenderText_Solid(normalFont, coinText.c_str(), textColor);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {SCREEN_WIDTH - 150, 20, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    string livesText = "Lives: " + to_string(lives);
    textSurface = TTF_RenderText_Solid(normalFont, livesText.c_str(), textColor);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, 20, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    if (smallFont != normalFont) {
        TTF_CloseFont(smallFont);
    }
    if (normalFont != font) {
        TTF_CloseFont(normalFont);
    }
}
// ham hieu ung khi dat diem cao moi
void showNewRecordEffect(SDL_Renderer* renderer, TTF_Font* font, bool& isFirstNewRecord) {
    if (!isFirstNewRecord) {
        return;
    }
    isFirstNewRecord = false;

    Uint32 startTime = SDL_GetTicks();
    while (SDL_GetTicks() - startTime < 2000) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &overlay);

        string recordText = "NEW HIGH SCORE!";
        SDL_Color color = {255, 215, 0, 255}; // Màu vàng
        SDL_Surface* surface = TTF_RenderText_Solid(font, recordText.c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {
            SCREEN_WIDTH/2 - surface->w/2,
            SCREEN_HEIGHT/2 - surface->h/2,
            surface->w,
            surface->h
        };
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        SDL_RenderPresent(renderer);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);


        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return;
            }
        }
    }
}
// ham tai media anh va am thanh
bool loadMedia() {
    carTexture = loadTexture("oto.png");
    if (!carTexture) return false;
    donkeyTexture = loadTexture("conlua.png");
    if (!donkeyTexture) return false;
    coinTexture = loadTexture("tien.png");
    if (!coinTexture) return false;
    explosionTexture = loadTexture("no.png");
    if (!explosionTexture) return false;
    introTexture = loadTexture("bg1.jpg");
    if (!introTexture) return false;
    gameOverTexture= loadTexture("gameover.jpg");
    if(!gameOverTexture) return false;
    for (int i = 0; i < GRASS_FRAMES; i++) {
        string path = "grass" + to_string(i+1) + ".png";
        grassTextures[i] = loadTexture(path.c_str());
        if (!grassTextures[i]) return false;
    }
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
    grassSound = Mix_LoadWAV("grass.wav");
    if (!grassSound) return false;
    coinSound = Mix_LoadWAV("coinSound.wav");
    if(!coinSound) return false;
    collisionSmallSound=Mix_LoadWAV("vachamtrumang.wav");
    if(!collisionSmallSound) return false;
    extralifeSound=Mix_LoadWAV("mangthem.wav");
    if(!extralifeSound) return false;
     return true;
}
// ham giai phong bo nho cac doi tuong
void close() {
    SDL_DestroyTexture(carTexture);
    SDL_DestroyTexture(donkeyTexture);
    SDL_DestroyTexture(coinTexture);
    SDL_DestroyTexture(explosionTexture);
    SDL_DestroyTexture(introTexture);
    SDL_DestroyTexture(gameOverTexture);
    for (int i = 0; i < GRASS_FRAMES; i++) {
        SDL_DestroyTexture(grassTextures[i]);
    }
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
    Mix_FreeChunk(grassSound);
    Mix_FreeChunk(coinSound);
    Mix_FreeChunk(collisionSmallSound);
    Mix_FreeChunk(extralifeSound);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    Mix_Quit();
}
// ham hien thi hieu ung no
void showExplosion(int x, int y) {
    SDL_Rect explosionRect = {x - 32, y - 32, 64, 64};
    SDL_RenderCopy(renderer, explosionTexture, NULL, &explosionRect);
    SDL_RenderPresent(renderer);
    SDL_Delay(500);
}
// ham tao chu cho phan intro cong hinh sin
void renderCurvedTitle(int yOffset) {
    string title = "DONKEY GAME";
    SDL_Color textColor = {255, 0, 0, 255};
    TTF_Font* font = TTF_OpenFont("Impacted.ttf", 72);
    if (!font) {
       cout<< TTF_GetError();
        return;
    }
    int totalWidth = 0;
    vector<SDL_Surface*> charSurfaces;
    vector<SDL_Texture*> charTextures;
    for (char c : title) {
        string charStr(1, c);
        SDL_Surface* surface = TTF_RenderText_Solid(font, charStr.c_str(), textColor);
        if (!surface) {
            cout << TTF_GetError() << endl;
            continue;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_GetError() ;
            SDL_FreeSurface(surface);
            continue;
        }

        charSurfaces.push_back(surface);
        charTextures.push_back(texture);
        totalWidth += surface->w;
    }

    // can giua
    int startX = (SCREEN_WIDTH - totalWidth) / 2;
    int baseY = SCREEN_HEIGHT / 3 + yOffset;

    // ve tung doi tuong cong hinh sin
    for (size_t i = 0; i < charTextures.size(); i++) {
        SDL_Rect charRect = {
            startX,
            baseY + static_cast<int>(20 * sin(i * 0.5)), // Cong theo hàm sin
            charSurfaces[i]->w,
            charSurfaces[i]->h
        };
        SDL_RenderCopy(renderer, charTextures[i], NULL, &charRect);
        startX += charSurfaces[i]->w;
    }
    for (size_t i = 0; i < charSurfaces.size(); i++) {
        SDL_FreeSurface(charSurfaces[i]);
        SDL_DestroyTexture(charTextures[i]);
    }
    TTF_CloseFont(font);
}
// ham intro
void showIntro() {
    Mix_PlayMusic(introMusic, -1);
    bool quit = false;
    SDL_Event e;
    Uint32 startTime = SDL_GetTicks();
    bool visible = true;
    int yOffset = 0;
    bool movingDown = true;
    const int BLINK_INTERVAL = 600;
    const int MOVE_RANGE = 8;
    while (!quit) {
        SDL_RenderCopy(renderer, introTexture, NULL, NULL);

        if (visible) {
            renderCurvedTitle(yOffset);

        }


        string introText = "Press any to start";
        SDL_Color textColor = {255, 255, 255, 255};
        TTF_Font* font = TTF_OpenFont("arial.ttf", 28);
        if (font) {
            SDL_Surface* surface = TTF_RenderText_Solid(font, introText.c_str(), textColor);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {
                SCREEN_WIDTH/2 - surface->w/2,
                SCREEN_HEIGHT*2/3,
                surface->w,
                surface->h
            };
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
            TTF_CloseFont(font);
        }

        SDL_RenderPresent(renderer);

        // hieu ung
        if (SDL_GetTicks() - startTime > BLINK_INTERVAL) {
            visible = !visible;
            startTime = SDL_GetTicks();
        }
        yOffset += movingDown ? 1 : -1;
        if (abs(yOffset) >= MOVE_RANGE) movingDown = !movingDown;

        // su kien
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym != SDLK_ESCAPE)) {
                quit = true;
            }
        }

        SDL_Delay(16);
    }

    Mix_HaltMusic();
}
// ham hien thi  ket thuc game
bool showGameOver(int score, int goldCoins) {
    SDL_RenderCopy(renderer, gameOverTexture, NULL, NULL);
    SDL_Texture* screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                                 SDL_TEXTUREACCESS_TARGET,
                                                 SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_SetRenderTarget(renderer, screenTexture);
    SDL_RenderCopy(renderer, NULL, NULL, NULL);
    SDL_SetRenderTarget(renderer, NULL);

    for (int i = 0; i < 8; ++i) {
        SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, (i % 2 == 0) ? 128 : 0);
        SDL_RenderFillRect(renderer, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(30);
    }

    for (int i = 0; i < 15; ++i) {
        int offsetX = rand() % 12 - 6;
        int offsetY = rand() % 12 - 6;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Rect destRect = {offsetX, offsetY, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, screenTexture, NULL, &destRect);
        SDL_RenderPresent(renderer);
        SDL_Delay(20);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    if (gameOverTexture) {
        SDL_Rect imgRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, gameOverTexture, NULL, &imgRect);
    }

    if (!font) {
        font = TTF_OpenFont("arialbd.ttf", 24);
        if (!font) {
            SDL_DestroyTexture(screenTexture);
            if (gameOverTexture) SDL_DestroyTexture(gameOverTexture);
            return false;
        }
    }

    SDL_Color textColor = {255, 255, 255, 255};
    std::string scoreText = "SCORE: " + std::to_string(score) + "   GOLD: " + std::to_string(goldCoins);
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, scoreText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    SDL_Rect scoreRect = {
        SCREEN_WIDTH / 2 - textSurface->w / 2,
        SCREEN_HEIGHT / 2 - textSurface->h - 20,
        textSurface->w,
        textSurface->h
    };
    SDL_RenderCopy(renderer, textTexture, NULL, &scoreRect);

    std::string replayText = "PRESS [R] TO REPLAY   [Q] TO QUIT";
    SDL_Surface* replaySurface = TTF_RenderText_Blended(font, replayText.c_str(), textColor);
    SDL_Texture* replayTexture = SDL_CreateTextureFromSurface(renderer, replaySurface);

    SDL_Rect replayRect = {
        SCREEN_WIDTH / 2 - replaySurface->w / 2,
        SCREEN_HEIGHT / 2 + 40,
        replaySurface->w,
        replaySurface->h
    };
    SDL_RenderCopy(renderer, replayTexture, NULL, &replayRect);

    SDL_RenderPresent(renderer);

    SDL_FreeSurface(textSurface);
    SDL_FreeSurface(replaySurface);
    SDL_DestroyTexture(textTexture);
    SDL_DestroyTexture(replayTexture);
    SDL_DestroyTexture(screenTexture);

    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_r) return true;
                if (e.key.keysym.sym == SDLK_q) return false;
            }
        }
        SDL_Delay(16);// cai nay chinh 1 hoac 16 thi se dung duoc r va q
    }
}

// ham sinh ra co
void spawnGrass() {
    if (rand() % 1000 >= GRASS_SPAWN_CHANCE) return;
    SDL_Rect newGrass;
    newGrass.w = GRASS_WIDTH;
    newGrass.h = GRASS_HEIGHT;
    newGrass.y = -GRASS_HEIGHT;

    int attempts = 0;
    const int MAX_ATTEMPTS = 5;

    do {
        newGrass.x = (rand() % 2 == 0) ? (LEFT_LANE_CENTER - GRASS_WIDTH / 2)
                                      : (RIGHT_LANE_CENTER - GRASS_WIDTH / 2);
        attempts++;

        if (attempts >= MAX_ATTEMPTS) {
            return;
        }
    } while (!isSafeToSpawn(newGrass));

    Grass grass;
    grass.rect = newGrass;
    grass.frame = 0;
    grass.frameCounter = 0;
    grass.active = true;
    grass.scored = false;
    grass.isDangerous = true;
    grasses.push_back(grass);

    // them vung cam
    ForbiddenZone zone;
    zone.rect = {grass.rect.x - 10, grass.rect.y - 10,
                GRASS_WIDTH + 20, GRASS_HEIGHT + 30};
    zone.framesRemaining = 60;
    forbiddenZones.push_back(zone);
}
// ham sinh ra lua
void spawnDonkey() {
    SDL_Rect newDonkey;
    newDonkey.w = DONKEY_WIDTH;
    newDonkey.h = DONKEY_HEIGHT;
    newDonkey.y = -DONKEY_HEIGHT;

    int attempts = 0;
    const int MAX_ATTEMPTS = 10;

    do {
        newDonkey.x = (rand() % 2 == 0) ? (LEFT_LANE_CENTER - DONKEY_WIDTH / 2)
                                       : (RIGHT_LANE_CENTER - DONKEY_WIDTH / 2);
        attempts++;

        if (attempts >= MAX_ATTEMPTS) {
            return;
        }
    } while (!isSafeToSpawn(newDonkey));
    donkey = newDonkey;
    ForbiddenZone zone;
    zone.rect = {donkey.x - 20, donkey.y - 20,
                DONKEY_WIDTH + 40, DONKEY_HEIGHT + 100};
    zone.framesRemaining = 120;
    forbiddenZones.push_back(zone);
}
// ham sinh xu
void spawnCoin() {
    SDL_Rect newCoin;
    newCoin.w = COIN_WIDTH;
    newCoin.h = COIN_HEIGHT;
    newCoin.y = -COIN_HEIGHT;
    int attempts = 0;
    const int MAX_ATTEMPTS = 10;

    do {
        newCoin.x = (rand() % 2 == 0) ? (LEFT_LANE_CENTER - COIN_WIDTH / 2)
                                     : (RIGHT_LANE_CENTER - COIN_WIDTH / 2);
        attempts++;

        if (attempts >= MAX_ATTEMPTS) {
            return;
        }
    } while (!isSafeToSpawn(newCoin));

    coin = newCoin;
    coinCollected = false;  // trang thai chua thu thap

    ForbiddenZone zone;
    zone.rect = {coin.x - 15, coin.y - 15,
                COIN_WIDTH + 30, COIN_HEIGHT + 50};
    zone.framesRemaining = 90;
    forbiddenZones.push_back(zone);
}
void updateForbiddenZones() {
    for (size_t i = 0; i < forbiddenZones.size(); ) {
        forbiddenZones[i].framesRemaining--;
        if (forbiddenZones[i].framesRemaining <= 0) {
            forbiddenZones.erase(forbiddenZones.begin() + i);
        } else {
            i++;
        }
    }
}
void updateGrasses() {
    for (size_t i = 0; i < grasses.size(); ) {
        grasses[i].rect.y += (int)current_speed;
        grasses[i].frameCounter++;
        if (grasses[i].frameCounter >= 15) {
            grasses[i].frame = (grasses[i].frame + 1) % GRASS_FRAMES;
            grasses[i].frameCounter = 0;
        }
        if (grasses[i].rect.y > SCREEN_HEIGHT) {
            grasses.erase(grasses.begin() + i);
        } else {
            i++;
        }
    }
}
// ham tao hieu ung co lan lap lai 4 trang thai cua co khi lan
void renderGrasses() {
    for (const auto& grass : grasses) {
        if (grass.active) {
            SDL_RenderCopy(renderer, grassTextures[grass.frame], NULL, &grass.rect);
        }
    }
}
// ham lap game chinh
void gameLoop() {
    bool quit = false;
    SDL_Event e;
    // khoi tao cac trang thai cho game
    int score = 0;
    int highScore = readHighScore();
    int goldCoins = 0;
    const int MAX_LIVES = 5;
    int lives = 3;
    int coinsForExtraLife = 0;
    current_speed = base_speed;
    bool donkeyPassed = false;
    bool donkeyHalfPassed = false;
    bool coinActive = true;
    bool coinCollected = false;
    bool isFirstNewRecord = true;
    Uint32 coinSpawnTime = 0;
    grasses.clear();
    forbiddenZones.clear();
    // sinh ra cac doi tuong ban dau
    spawnDonkey();
    spawnCoin();
    car.x = SCREEN_WIDTH / 2 - CAR_WIDTH / 2;
    car.y = SCREEN_HEIGHT - CAR_HEIGHT - 20;
    // bat dau am thanh
    Mix_VolumeChunk(coinSound, MIX_MAX_VOLUME / 3);
    Mix_PlayChannel(-1, engineSound, -1);

    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                // cac thao tac tren ban phim
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                        car.x = LEFT_LANE_CENTER - CAR_WIDTH / 2;
                        break;
                    case SDLK_RIGHT:
                        car.x = RIGHT_LANE_CENTER - CAR_WIDTH / 2;
                        break;
                           case SDLK_r:
                        score = 0;
                        goldCoins = 0;
                        coinsForExtraLife = 0;
                        lives = 3;
                        current_speed = base_speed;
                        grasses.clear();
                        forbiddenZones.clear();
                        spawnDonkey();
                        spawnCoin();
                        car.x = SCREEN_WIDTH / 2 - CAR_WIDTH / 2;
                        car.y = SCREEN_HEIGHT - CAR_HEIGHT - 20;
                        donkeyPassed = false;
                        coinActive = true;
                        coinCollected = false;
                        isFirstNewRecord = true;
                        Mix_HaltChannel(-1);
                        Mix_PlayChannel(-1, engineSound, -1);
                        break;
                    case SDLK_q:
                        quit = true;
                        break;
                }
            }
        }
        // cap nhat cac trang thai game

        updateForbiddenZones();
        updateRoadStrips(current_speed);
        updateBrickRows(current_speed);
        spawnGrass();
        updateGrasses();

        donkey.y += (int)current_speed;
        if (donkey.y > SCREEN_HEIGHT) {
            spawnDonkey();
            donkeyPassed = false;
            donkeyHalfPassed = false;
        } else {
            int carBottom = car.y + CAR_HEIGHT;
            int donkeyTop = donkey.y;
            int donkeyMiddle = donkey.y + DONKEY_HEIGHT / 2;

            if (!donkeyHalfPassed && carBottom <= donkeyMiddle) {
                donkeyHalfPassed = true;
            }
            if (!donkeyPassed && donkeyHalfPassed && carBottom <= donkeyTop) {
                score += 1;
                donkeyPassed = true;
                Mix_PlayChannel(-1, pointSound, 0);
                if (score % SPEED_INCREASE_INTERVAL == 0 && current_speed < MAX_SPEED) {
                    current_speed += SPEED_INCREMENT;
                }
            }
        }
if (score > highScore) {
            showNewRecordEffect(renderer, font, isFirstNewRecord);
            highScore = score;
            writeHighScore(highScore);
        }
        if (coinActive) {
            coin.y += (int)current_speed;
            if (!coinCollected && checkPreciseCollision(car, CAR_HITBOX, coin, COIN_HITBOX)) {
                goldCoins = min(goldCoins + 1, 999);
                coinsForExtraLife++;
                coinCollected = true;
                coinActive = false;
                coin.y = SCREEN_HEIGHT + 100;
                Mix_PlayChannel(-1, coinSound, 0);
                if (coinsForExtraLife >= 10 && lives < MAX_LIVES) {
                    lives++;
                    Mix_PlayChannel(-1,extralifeSound,0);
                    coinsForExtraLife -= 10;
                }
            }
            if (coin.y > SCREEN_HEIGHT) {
                spawnCoin();
                coinCollected = false;
            }
        }
        if (!coinActive && currentTime >= coinSpawnTime) {
            spawnCoin();
            coinActive = true;
            coinCollected = false;
        }
      if (checkPreciseCollision(car, CAR_HITBOX, donkey, DONKEY_HITBOX)) {
            if (lives > 1) {
                lives--;
                spawnDonkey();
                Mix_PlayChannel(-1,collisionSmallSound,0);
                donkeyPassed = false;
            } else if (lives == 1) {
                lives--;
                showExplosion(car.x + CAR_WIDTH / 2, car.y + CAR_HEIGHT / 2);
                Mix_HaltChannel(-1);
                Mix_PlayChannel(-1, collisionSound, 0);
                if (showGameOver(score, goldCoins)) {
                    score = 0;
                    goldCoins = 0;
                    coinsForExtraLife = 0;
                    lives = 3;
                    current_speed = base_speed;
                    grasses.clear();
                    forbiddenZones.clear();
                    spawnDonkey();
                    spawnCoin();
                    car.x = SCREEN_WIDTH / 2 - CAR_WIDTH / 2;
                    car.y = SCREEN_HEIGHT - CAR_HEIGHT - 20;
                    donkeyPassed = false;
                    coinActive = true;
                    coinCollected = false;
                    Mix_PlayChannel(-1, engineSound, -1);
                } else {
                    quit = true;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        renderBackground();
        renderScore(score, goldCoins, lives, highScore);
        renderGrasses();
        SDL_RenderCopy(renderer, carTexture, NULL, &car);
        SDL_RenderCopy(renderer, donkeyTexture, NULL, &donkey);
        if (coinActive) {
            SDL_RenderCopy(renderer, coinTexture, NULL, &coin);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    // dung am thanh khi thoat game
    Mix_HaltChannel(-1);
}
int main(int argc, char* argv[]) {
    srand(time(NULL));// khoi tao sinh ngau nhien

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
