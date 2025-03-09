#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

// Kích thước màn hình
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// Kích thước xe và donkey
const int CAR_WIDTH = 50;
const int CAR_HEIGHT = 80;
const int DONKEY_WIDTH = 50;
const int DONKEY_HEIGHT = 80;

// Tốc độ di chuyển
float car_speed = 5.0f;
float donkey_speed = 5.0f;
const float ACCELERATION = 0.01f; // Gia tốc tăng dần

// Ngưỡng điểm và tốc độ tăng
const int SPEED_INCREASE_THRESHOLD = 10; // Cứ 10 điểm thì tăng tốc độ
const float SPEED_INCREMENT = 1.0f;     // Tốc độ tăng thêm mỗi lần

// Tốc độ cuộn cây cỏ
float grass_scroll_speed = 2.0f;
float grass_offset = 0.0f;

// Biến toàn cục
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* carTexture = NULL;
SDL_Texture* donkeyTexture = NULL;
SDL_Texture* explosionTexture = NULL; // Texture cho hiệu ứng nổ
SDL_Texture* grassTexture = NULL;     // Texture cho cỏ cây
SDL_Texture* introTexture = NULL;     // Texture cho màn hình intro
TTF_Font* font = NULL;

// Tọa độ của xe và con lừa
SDL_Rect car = {SCREEN_WIDTH / 2 - CAR_WIDTH / 2, SCREEN_HEIGHT - CAR_HEIGHT - 20, CAR_WIDTH, CAR_HEIGHT};
SDL_Rect donkey = {rand() % (SCREEN_WIDTH - 320 - DONKEY_WIDTH) + 160, -DONKEY_HEIGHT, DONKEY_WIDTH, DONKEY_HEIGHT};

// Hàm tải ảnh sử dụng SDL_image
SDL_Texture* loadTexture(const char* path) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, path);
    if (!texture) {
        cout << "LOi ANH " << path << "! : " << IMG_GetError() << endl;
    }
    return texture;
}

// Khởi tạo SDL, SDL_image và SDL_ttf
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

// Vẽ background và viền giới hạn
void renderBackground() {
    SDL_SetRenderDrawColor(renderer, 0, 128, 128, 255); // Màu xanh cyan
    SDL_Rect leftPanel = {0, 0, 160, SCREEN_HEIGHT};
    SDL_Rect rightPanel = {SCREEN_WIDTH - 160, 0, 160, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &leftPanel);
    SDL_RenderFillRect(renderer, &rightPanel);

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); // Màu xám đường đua
    SDL_Rect road = {160, 0, SCREEN_WIDTH - 320, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &road);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Màu trắng cho vạch kẻ
    for (int i = 0; i < SCREEN_HEIGHT; i += 40) {
        SDL_Rect laneMark = {SCREEN_WIDTH / 2 - 5, i, 10, 20};
        SDL_RenderFillRect(renderer, &laneMark);
    }

    // Vẽ cỏ cây ven đường với hiệu ứng cuộn
    SDL_Rect leftGrass = {0, (int)grass_offset, 160, SCREEN_HEIGHT};
    SDL_Rect rightGrass = {SCREEN_WIDTH - 160, (int)grass_offset, 160, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, grassTexture, NULL, &leftGrass);
    SDL_RenderCopy(renderer, grassTexture, NULL, &rightGrass);

    // Vẽ thêm một bản sao của cỏ cây để tạo hiệu ứng cuộn liên tục
    SDL_Rect leftGrass2 = {0, (int)grass_offset - SCREEN_HEIGHT, 160, SCREEN_HEIGHT};
    SDL_Rect rightGrass2 = {SCREEN_WIDTH - 160, (int)grass_offset - SCREEN_HEIGHT, 160, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, grassTexture, NULL, &leftGrass2);
    SDL_RenderCopy(renderer, grassTexture, NULL, &rightGrass2);
}

// Vẽ điểm số trên màn hình
void renderScore(int score) {
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
}

bool loadMedia() {
    carTexture = loadTexture("car.png");
    if (!carTexture) return false;

    donkeyTexture = loadTexture("conlua.bmp"); // Đảm bảo bạn đang dùng đúng hình ảnh con lừa
    if (!donkeyTexture) return false;

    explosionTexture = loadTexture("no.png");
    if (!explosionTexture) return false;

    grassTexture = loadTexture("grass.png");
    if (!grassTexture) return false;

    introTexture = loadTexture("intro2.jpg");
    if (!introTexture) return false;

    return true;
}

// Giải phóng bộ nhớ
void close() {
    SDL_DestroyTexture(carTexture);
    SDL_DestroyTexture(donkeyTexture);
    SDL_DestroyTexture(explosionTexture);
    SDL_DestroyTexture(grassTexture);
    SDL_DestroyTexture(introTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

// Kiểm tra va chạm
bool checkCollision(SDL_Rect a, SDL_Rect b) {
    return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y);
}

// Hiển thị hiệu ứng nổ
void showExplosion(int x, int y) {
    SDL_Rect explosionRect = {x - 32, y - 32, 64, 64}; // Kích thước hiệu ứng nổ
    SDL_RenderCopy(renderer, explosionTexture, NULL, &explosionRect);
    SDL_RenderPresent(renderer);
    SDL_Delay(500); // Tạm dừng 500ms để hiển thị hiệu ứng nổ
}

// Hiển thị màn hình intro
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

// Hiển thị màn hình kết thúc
bool showGameOver(int score) {
    // Hiệu ứng lóe sáng
    for (int i = 0; i < 10; ++i) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    // Hiệu ứng rung màn hình
    for (int i = 0; i < 10; ++i) {
        SDL_SetWindowPosition(window, rand() % 10 - 5, rand() % 10 - 5);
        SDL_Delay(10);
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // Hiển thị thông báo Game Over
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (!font) {
        font = TTF_OpenFont("arial.ttf", 24);
        if (!font) {
            return false;
        }
    }
    string gameOverText = "Game Over! Score: " + to_string(score);
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

// Hàm xử lý game
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
                        car.x -= (int)car_speed;
                        if (car.x < 160) car.x = 160;
                        break;
                    case SDLK_RIGHT:
                        car.x += (int)car_speed;
                        if (car.x + CAR_WIDTH > SCREEN_WIDTH - 160) car.x = SCREEN_WIDTH - 160 - CAR_WIDTH;
                        break;
                }
            }
        }

        Uint32 currentTime = SDL_GetTicks();
        donkey_speed += ACCELERATION * ((currentTime - startTime) / 1000.0f);

        donkey.y += (int)donkey_speed;
        if (donkey.y > SCREEN_HEIGHT) {
            donkey.x = rand() % (SCREEN_WIDTH - 320 - DONKEY_WIDTH) + 160;
            donkey.y = -DONKEY_HEIGHT;
            score++;

            // Tăng tốc độ xe khi đạt ngưỡng điểm
            if (score % SPEED_INCREASE_THRESHOLD == 0 && score != 0) {
                car_speed += SPEED_INCREMENT;
            }
        }

        // Cập nhật vị trí cuộn của cỏ cây
        grass_offset += grass_scroll_speed;
        if (grass_offset > SCREEN_HEIGHT) {
            grass_offset = 0.0f;
        }

        if (checkCollision(car, donkey)) {
            showExplosion(car.x + CAR_WIDTH / 2, car.y + CAR_HEIGHT / 2);

            if (showGameOver(score)) {
                // Reset game
                car.x = SCREEN_WIDTH / 2 - CAR_WIDTH / 2;
                car.y = SCREEN_HEIGHT - CAR_HEIGHT - 20;
                donkey.x = rand() % (SCREEN_WIDTH - 320 - DONKEY_WIDTH) + 160;
                donkey.y = -DONKEY_HEIGHT;
                score = 0;
                donkey_speed = 5.0f;
                car_speed = 5.0f; // Reset tốc độ xe
                startTime = SDL_GetTicks();
            } else {
                quit = true;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        renderBackground();
        renderScore(score);
                SDL_RenderCopy(renderer, carTexture, NULL, &car);
        SDL_RenderCopy(renderer, donkeyTexture, NULL, &donkey);
        SDL_RenderPresent(renderer);

        // Điều chỉnh tốc độ khung hình
        SDL_Delay(16); // ~60 FPS
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
// qua 1 muc diem nhan=t dinh doi map cho choi kie
