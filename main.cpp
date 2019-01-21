#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <stdexcept>
#include <memory>
#include <string>
#include <set>
#include <tuple>
#include <iostream>
#include <cstdint>
#include <vector>
#include <array>
#include <list> 
#include <time.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

auto errthrow = [](const std::string &e) {
	std::string errstr = e + " : " + SDL_GetError();
	SDL_Quit();
	throw std::runtime_error(errstr);
};

std::shared_ptr<SDL_Window> init_window(const int width = 320, const int height = 200) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) errthrow("SDL_InitVideo");

	SDL_Window *win = SDL_CreateWindow("Asteroids",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height, SDL_WINDOW_SHOWN);
	if (win == nullptr) errthrow("SDL_CreateWindow");
	std::shared_ptr<SDL_Window> window(win, [](SDL_Window * ptr) {
		SDL_DestroyWindow(ptr);
	});
	return window;
}

std::shared_ptr<SDL_Renderer> init_renderer(std::shared_ptr<SDL_Window> window) {
	SDL_Renderer *ren = SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == nullptr) errthrow("SDL_CreateRenderer");
	std::shared_ptr<SDL_Renderer> renderer(ren, [](SDL_Renderer * ptr) {
		SDL_DestroyRenderer(ptr);
	});
	return renderer;
}

std::shared_ptr<SDL_Texture> load_texture(const std::shared_ptr<SDL_Renderer> renderer, const std::string fname) {
	SDL_Surface *bmp = IMG_Load(fname.c_str());
	if (bmp == nullptr) errthrow("IMG_Load");
	std::shared_ptr<SDL_Surface> bitmap(bmp, [](SDL_Surface * ptr) {
		SDL_FreeSurface(ptr);
	});

	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer.get(), bitmap.get());
	if (tex == nullptr) errthrow("SDL_CreateTextureFromSurface");
	std::shared_ptr<SDL_Texture> texture(tex, [](SDL_Texture * ptr) {
		SDL_DestroyTexture(ptr);
	});
	return texture;
}

std::shared_ptr<SDL_Texture> load_score_text(const std::shared_ptr<SDL_Renderer> renderer, const char* textureText, const int size, const SDL_Color color) {
	if (TTF_Init() < 0) errthrow("TTF_Init");
	TTF_Font* Sans = TTF_OpenFont("space.ttf", size);
	SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, textureText, color);
	if (surfaceMessage == nullptr) errthrow("Font");
	std::shared_ptr<SDL_Surface> bitmap(surfaceMessage, [](SDL_Surface * ptr) {
		SDL_FreeSurface(ptr);
	});
	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer.get(), bitmap.get());
	if (tex == nullptr) errthrow("SDL_CreateTextureFromSurfaceFont");
	std::shared_ptr<SDL_Texture> texture(tex, [](SDL_Texture * ptr) {
		SDL_DestroyTexture(ptr);
	});
	return texture;
}

std::shared_ptr<Mix_Music> load_music(const char* fileName) {
	if (SDL_Init(SDL_INIT_AUDIO) != 0) errthrow("SDL_InitAudio");

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) errthrow("SDL_Audio");

	Mix_Music *bgm = Mix_LoadMUS(fileName);
	if (bgm == nullptr) errthrow("Music");
	std::shared_ptr<Mix_Music> music(bgm, [](Mix_Music * ptr) {
		Mix_FreeMusic(ptr);
	});
	return music;
}

std::shared_ptr<Mix_Chunk> load_sound(const char* fileName) {
	if (SDL_Init(SDL_INIT_AUDIO) != 0) errthrow("SDL_InitAudio");

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) errthrow("SDL_Audio");

	Mix_Chunk *snd = Mix_LoadWAV(fileName);
	if (snd == nullptr) errthrow("Sound");
	std::shared_ptr<Mix_Chunk> sound(snd, [](Mix_Chunk * ptr) {
		Mix_FreeChunk(ptr);
	});
	return sound;
}

using pos_t = std::array<double, 2>;

pos_t operator +(const pos_t &a, const pos_t &b) {
	return { a[0] + b[0], a[1] + b[1] };
}
pos_t operator -(const pos_t &a, const pos_t &b) {
	return { a[0] - b[0], a[1] - b[1] };
}
pos_t operator *(const pos_t &a, const pos_t &b) {
	return { a[0] * b[0], a[1] * b[1] };
}
pos_t operator *(const pos_t &a, const double &b) {
	return { a[0] * b, a[1] * b };
}

struct Circle
{
	int x, y;
	int r;
};

class player {
public:
	pos_t position;
	pos_t velocity = { 0,0 };
	float angle = 0;
	int size;
	float cd;
	SDL_Rect dstrect;
	Circle mCollider;

	void Update(std::shared_ptr<SDL_Renderer> &r, std::shared_ptr<SDL_Texture> &t) {
		if (size == 0) return;
		cd -= 1 / 30.0;
		double xN = sin(2 * M_PI * (angle / 360)) * velocity[1];
		double yN = -cos(2 * M_PI * (angle / 360)) * velocity[1];
		position[0] += xN;
		position[1] += yN;

		if (position[0] < -size / 2)
			position[0] = SCREEN_WIDTH + size / 2;
		if (position[1] < -size / 2)
			position[1] = SCREEN_HEIGHT + size / 2;
		if (position[0] > SCREEN_WIDTH + size / 2)
			position[0] = -size / 2;
		if (position[1] > SCREEN_HEIGHT + size / 2)
			position[1] = -size / 2;

		mCollider.x = position[0];
		mCollider.y = position[1];

		dstrect = { (int)position[0] - size / 2, (int)position[1] - size / 2, size, size };
		SDL_RenderCopyEx(r.get(), t.get(), NULL, &dstrect, angle, NULL, SDL_FLIP_NONE);
	}
};

class bullet {
public:
	pos_t position;
	pos_t velocity = { 0,0 };
	float angle = 0;
	int size;
	float lifetime;
	SDL_Rect dstrect;
	bool shooted = false;
	Circle mCollider;

	void Update(std::shared_ptr<SDL_Renderer> &r, std::shared_ptr<SDL_Texture> &t) {
		if (size == 0) return;
		lifetime -= 1 / 30.0;
		if (lifetime <= 0) {
			size = 0;
		}
		double xN = sin(2 * M_PI * (angle / 360)) * velocity[1];
		double yN = -cos(2 * M_PI * (angle / 360)) * velocity[1];
		position[0] += xN;
		position[1] += yN;

		if (position[0] < -size / 2)
			position[0] = SCREEN_WIDTH + size / 2;
		if (position[1] < -size / 2)
			position[1] = SCREEN_HEIGHT + size / 2;
		if (position[0] > SCREEN_WIDTH + size / 2)
			position[0] = -size / 2;
		if (position[1] > SCREEN_HEIGHT + size / 2)
			position[1] = -size / 2;

		mCollider.x = position[0];
		mCollider.y = position[1];

		dstrect = { (int)position[0] - size / 2, (int)position[1] - size / 2, size, size };
		SDL_RenderCopyEx(r.get(), t.get(), NULL, &dstrect, angle, NULL, SDL_FLIP_NONE);
	}
};

class asteroid {
public:
	pos_t position;
	pos_t velocity = { 0,0 };
	float angle = 0;
	int size;
	SDL_Rect dstrect;
	bool big = true;
	Circle mCollider;

	void Update(std::shared_ptr<SDL_Renderer> &r, std::shared_ptr<SDL_Texture> &t) {
		if (size == 0) return;
		double xN = sin(2 * M_PI * (angle / 360)) * velocity[1];
		double yN = -cos(2 * M_PI * (angle / 360)) * velocity[1];
		position[0] += xN;
		position[1] += yN;

		if (position[0] < -size / 2)
			position[0] = SCREEN_WIDTH + size / 2;
		if (position[1] < -size / 2)
			position[1] = SCREEN_HEIGHT + size / 2;
		if (position[0] > SCREEN_WIDTH + size / 2)
			position[0] = -size / 2;
		if (position[1] > SCREEN_HEIGHT + size / 2)
			position[1] = -size / 2;

		mCollider.x = position[0];
		mCollider.y = position[1];

		dstrect = { (int)position[0] - size / 2, (int)position[1] - size / 2, size, size };
		SDL_RenderCopyEx(r.get(), t.get(), NULL, &dstrect, angle, NULL, SDL_FLIP_NONE);
	}
};

class explosion {
public:
	SDL_Rect position;
	SDL_Rect dstrect;
	int frameWidth = 0;
	int frameHeight = 0;
	int frameTime = 0;

	void Update(std::shared_ptr<SDL_Renderer> &r, std::shared_ptr<SDL_Texture> &t) {
		frameTime++;
		if (frameTime == 5) {
			frameTime = 0;
			dstrect.x += frameWidth;
			if (dstrect.x >= frameWidth) {
				dstrect.x = 0;
				dstrect.y += frameWidth;
			}
		}
		SDL_RenderCopy(r.get(), t.get(), &dstrect, &position);
	}
};

double distanceSquared(int x1, int y1, int x2, int y2)
{
	int deltaX = x2 - x1;
	int deltaY = y2 - y1;
	return deltaX * deltaX + deltaY * deltaY;
}

bool checkCollision(Circle& a, Circle& b)
{
	int totalRadiusSquared = a.r + b.r;
	totalRadiusSquared = totalRadiusSquared * totalRadiusSquared;

	if (distanceSquared(a.x, a.y, b.x, b.y) < (totalRadiusSquared))
	{
		return true;
	}

	return false;
}

int main(int argc, char **argv) {
	srand(time(NULL));
	auto window = init_window(SCREEN_WIDTH, SCREEN_HEIGHT);
	auto renderer = init_renderer(window);

	auto background_texture = load_texture(renderer, "bg.png");
	auto ship_texture = load_texture(renderer, "ship.png");
	auto asteroid_texture = load_texture(renderer, "asteroid.png");
	auto bullet_texture = load_texture(renderer, "bullet.png");
	auto explosion_texture = load_texture(renderer, "explosion.png");

	auto music = load_music("bgmusic.wav");
	auto shootSound = load_sound("shoot.wav");
	auto explosionSound = load_sound("explosion.wav");

	int highScore = 0;
	SDL_RWops* file = SDL_RWFromFile("score.sav", "r+b");

	if (file == NULL) {
		file = SDL_RWFromFile("score.sav", "w+b");
		if (file != NULL) {
			SDL_RWwrite(file, &highScore, sizeof(int), 1);
			SDL_RWclose(file);
		}
	}
	else {
		SDL_RWread(file, &highScore, sizeof(int), 1);
		SDL_RWclose(file);
	}

	bool restart = false;
	do {
		restart = false;
		SDL_Color score_color = { 0, 0, 255 };
		SDL_Color lose_color = { 255, 0, 0 };
		auto lose_text = load_score_text(renderer, "You lose!", 50, lose_color);

		std::vector<bullet> bullets;
		std::vector<asteroid> asteroids;
		std::vector<explosion> explosions;

		player player;
		player.size = 32;
		player.position[0] = SCREEN_WIDTH / 2;
		player.position[1] = SCREEN_HEIGHT / 2;
		player.mCollider.r = player.size / 2;
		player.mCollider.x = player.position[0];
		player.mCollider.y = player.position[1];

		char str[128];
		double dt = 1 / 60.0;
		float time = 0;
		int score = 0;
		float spawnTimer = 5;

		for (bool game_active = true; game_active; ) {
			if (!Mix_PlayingMusic()) {
				Mix_PlayMusic(music.get(), -1);
			}
			sprintf_s(str, "Score: %d", score);
			auto score_text = load_score_text(renderer, str, 30, score_color);
			time += dt;

			if (time >= spawnTimer) {
				spawnTimer += 5;
				asteroid asteroid;
				asteroid.size = 48;
				asteroid.angle = rand() % 360;
				asteroid.position = pos_t{ (double)(rand() % 832 - 24) , -24 };
				asteroid.velocity[1] = 3;
				asteroid.mCollider.r = asteroid.size / 2;
				asteroid.mCollider.x = asteroid.position[0];
				asteroid.mCollider.y = asteroid.position[1];
				asteroids.push_back(asteroid);
			}

			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT) { game_active = false; }
			}

			const Uint8 *kstate = SDL_GetKeyboardState(NULL);


			if (kstate[SDL_SCANCODE_R]) {
				restart = true;
				game_active = false;
			}

			if (player.size > 0) {
				if (kstate[SDL_SCANCODE_UP] && player.velocity[1] < 3) player.velocity[1] += 0.05;
				if (kstate[SDL_SCANCODE_DOWN] && player.velocity[1] > -3) player.velocity[1] -= 0.05;
				if (kstate[SDL_SCANCODE_RIGHT]) player.angle += 2;
				if (kstate[SDL_SCANCODE_LEFT]) player.angle -= 2;
				if (kstate[SDL_SCANCODE_SPACE] && player.cd <= 0) {
					Mix_PlayChannel(-1, shootSound.get(), 0);
					player.cd = 1;
					bullet bullet;
					bullet.lifetime = 3;
					bullet.size = 8;
					bullet.angle = player.angle;
					bullet.position = player.position;
					bullet.velocity[1] = 5;
					bullet.mCollider.r = bullet.size / 2;
					bullet.mCollider.x = bullet.position[0];
					bullet.mCollider.y = bullet.position[1];
					bullets.push_back(bullet);
				}
			}

			SDL_RenderClear(renderer.get());
			SDL_RenderCopy(renderer.get(), background_texture.get(), NULL, NULL);

			for (auto& bullet : bullets)
			{
				if (bullet.size == 0) continue;
				bullet.Update(renderer, bullet_texture);
				if (checkCollision(bullet.mCollider, player.mCollider)) {
					if (bullet.shooted) {
						Mix_PlayChannel(-1, explosionSound.get(), 0);
						int textureWidth;
						int textureHeight;
						SDL_QueryTexture(explosion_texture.get(), NULL, NULL, &textureWidth, &textureHeight);
						explosion explosion;
						explosion.frameWidth = textureWidth / 9;
						explosion.frameHeight = textureHeight / 9;
						explosion.dstrect = { 0, 0, explosion.frameWidth, explosion.frameHeight };
						explosion.position = { (int)player.position[0] - 12, (int)player.position[1] - 12, 48, 48 };
						explosions.push_back(explosion);
						player.size = 0;
						bullet.size = 0;
					}
				}
				else
					bullet.shooted = true;

				for (auto& ast : asteroids) {
					if (ast.size == 0) continue;
					if (checkCollision(bullet.mCollider, ast.mCollider)) {
						Mix_PlayChannel(-1, explosionSound.get(), 0);
						int textureWidth;
						int textureHeight;
						SDL_QueryTexture(explosion_texture.get(), NULL, NULL, &textureWidth, &textureHeight);
						explosion explosion;
						explosion.frameWidth = textureWidth / 9;
						explosion.frameHeight = textureHeight / 9;
						explosion.dstrect = { 0, 0, explosion.frameWidth, explosion.frameHeight };
						explosion.position = { (int)ast.position[0] - 12, (int)ast.position[1] - 12, 48, 48 };
						explosions.push_back(explosion);
						score += 10;
						ast.size = 0;
						bullet.size = 0;
						if (ast.big) {
							asteroid asteroid1;
							asteroid1.size = 24;
							asteroid1.angle = rand() % 360;
							asteroid1.position = ast.position;
							asteroid1.velocity[1] = 3;
							asteroid1.big = false;
							asteroid1.mCollider.r = asteroid1.size / 2;
							asteroid1.mCollider.x = asteroid1.position[0];
							asteroid1.mCollider.y = asteroid1.position[1];
							asteroids.push_back(asteroid1);
							asteroid asteroid2;
							asteroid2.size = 24;
							asteroid2.angle = rand() % 360;
							asteroid2.position = ast.position;
							asteroid2.velocity[1] = 3;
							asteroid2.big = false;
							asteroid2.mCollider.r = asteroid2.size / 2;
							asteroid2.mCollider.x = asteroid2.position[0];
							asteroid2.mCollider.y = asteroid2.position[1];
							asteroids.push_back(asteroid2);
						}
					}
				}
			}

			player.Update(renderer, ship_texture);

			for (auto& asteroid : asteroids)
			{
				if (asteroid.size == 0) continue;
				asteroid.Update(renderer, asteroid_texture);
				if (checkCollision(asteroid.mCollider, player.mCollider)) {
					if (player.size > 0) {
						Mix_PlayChannel(-1, explosionSound.get(), 0);
						int textureWidth;
						int textureHeight;
						SDL_QueryTexture(explosion_texture.get(), NULL, NULL, &textureWidth, &textureHeight);
						explosion explosion;
						explosion.frameWidth = textureWidth / 9;
						explosion.frameHeight = textureHeight / 9;
						explosion.dstrect = { 0, 0, explosion.frameWidth, explosion.frameHeight };
						explosion.position = { (int)player.position[0] - 12, (int)player.position[1] - 12, 48, 48 };
						explosions.push_back(explosion);
					}
					player.size = 0;
				}
			}

			for (auto& explosion : explosions)
			{
				explosion.Update(renderer, explosion_texture);
			}

			int texW = 0;
			int texH = 0;
			SDL_QueryTexture(score_text.get(), NULL, NULL, &texW, &texH);
			SDL_Rect Message_rect = { 400 - texW / 2, 25 - texH / 2, texW, texH };
			SDL_RenderCopy(renderer.get(), score_text.get(), NULL, &Message_rect);

			if (player.size == 0) {
				texW = 0;
				texH = 0;
				SDL_QueryTexture(lose_text.get(), NULL, NULL, &texW, &texH);
				Message_rect = { 400 - texW / 2, 200 - texH / 2, texW, texH };
				SDL_RenderCopy(renderer.get(), lose_text.get(), NULL, &Message_rect);

				sprintf_s(str, "Highscore: %d", highScore);

				if (score > highScore) {
					highScore = score;
					sprintf_s(str, "New highscore: %d", highScore);
					SDL_RWops* file = SDL_RWFromFile("score.sav", "w+b");
					SDL_RWwrite(file, &highScore, sizeof(int), 1);
					SDL_RWclose(file);
				}

				auto score_text = load_score_text(renderer, str, 60, score_color);
				texW = 0;
				texH = 0;
				SDL_QueryTexture(score_text.get(), NULL, NULL, &texW, &texH);
				Message_rect = { 400 - texW / 2, 400 - texH / 2, texW, texH };
				SDL_RenderCopy(renderer.get(), score_text.get(), NULL, &Message_rect);
			}
			SDL_RenderPresent(renderer.get());
			SDL_Delay((int)(dt / 1000));
		}
	} while (restart);
	return 0;
}
