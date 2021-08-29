#include <iostream>
#include <utility>
#include <set>
#include <map>
#include <initializer_list>

#include <SDL2/SDL.h>
#include <SDL2_ttf/SDL_ttf.h>
#include <SDL2_image/SDL_image.h>

#define null nullptr

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

bool init();
void close();
void loadMedia();

SDL_Window* gWindow = null;
SDL_Renderer* gRenderer = null;
std::map<std::string, SDL_Texture*> gTextures;

void evaluateDirection(const int inputs[2], const int lastInputs[2], int lastNonZeroInputs[2], int& rotation);

class Entity {
protected:
	int xPos, yPos;
	int width, height;
	int vel;

	SDL_Texture* sprite;
public:
	Entity();
	Entity(SDL_Rect& rect, int vel);
	~Entity();
	void loadSprite(SDL_Renderer*& renderer, const std::string& path);
	virtual void render(SDL_Renderer*& renderer);
};

class Player : public Entity {
private: // Constants
	const int MAX_DASH_FRAMES = 7;
	const int NORMAL_SPEED = 7;
	const int DASH_SPEED = 25, DASH_COOLDOWN = 500;
	const int SIZE = 60;
private:
	int rotationAngle;
	int lastInputs[2] = { 0, 0 };
	int lastNonZeroInputs[2] = { -1, 0};

public: bool dashMode;
	int dashFrames = 0;
	bool dashPossible;
	Uint32 timeSinceLastDash = -5000;
public:
	Player();
	Player(int x, int y);
	void move(const Uint8*& keyStates);
	void dash(Uint8 spaceState);
	void render(SDL_Renderer*& renderer) override;

	[[nodiscard]] SDL_Rect getRect() const { return (SDL_Rect){ this->xPos, this->yPos, this->width, this->width}; }
	void setPosition(const int position[2]) { this->xPos = position[0]; this->yPos = position[1]; }
	int* getLastMovement() { return this->lastNonZeroInputs; }
	int getVelocity() { return this->vel; }
};

struct WallRect;

class Walls {
public:
	enum Directions {
		UP, DOWN, LEFT, RIGHT
	};
	const int CORNER_WALL_SIZE = 200;
	const int WALL_WIDTH = 50;
private:
	std::set<WallRect> solidWalls;
public :
	explicit Walls(std::set<Directions> walls);
	~Walls() = default;
	void render(SDL_Renderer*& renderer);
	void manageCollision(Player& ply);
};

struct WallRect {
	WallRect(std::initializer_list<int> values) {
		if (values.size() != 5) {
			std::cout << "The initializer list for the WallRect didn't get the right amount of parameters" << std::endl;
			return;
		}
		auto pos = values.begin();
		r.x = *pos;
		pos++;
		r.y = *pos;
		pos++;
		r.w = *pos;
		pos++;
		r.h = *pos;
		pos++;
		id = *pos;
	}

	SDL_Rect r = {0, 0, 0, 0};
	int id;

	bool operator < (const WallRect& rect2) const {
		return this->id < rect2.id;
	}
};

void setBackPlayer(Player& ply, const WallRect& rect);

int main() {
	if (!init()) {
		close();
		return -69;
	}
	loadMedia();

	Player ply(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
	ply.loadSprite(gRenderer, "../Sprites/Ladybug.png");

	Walls wall((std::set<Walls::Directions>){Walls::Directions::UP, Walls::Directions::DOWN,
										  Walls::Directions::LEFT, Walls::Directions::RIGHT});

	bool running = true;
	SDL_Event e;

	while (running) {
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				running = false;
			}
		}
		const Uint8* currentKeyStates = SDL_GetKeyboardState(null);

		ply.move(currentKeyStates);
		wall.manageCollision(ply);

		SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(gRenderer);

		SDL_RenderCopy(gRenderer, gTextures.find("BG1")->second, null, null);

		ply.render(gRenderer);
		wall.render(gRenderer);

		SDL_RenderPresent(gRenderer);
	}
	
	close();
	return 0;
}

Entity::Entity() {
	this->xPos = 0;
	this->yPos = 0;
	this->width = 10;
	this->height = 10;
	this->vel = 1;
	this->sprite = null;
}

Entity::Entity(SDL_Rect& rect, int vel) {
	this->xPos = rect.x;
	this->yPos = rect.y;
	this->width = rect.w;
	this->height = rect.h;
	this->vel = vel;

	this->sprite = null;
}

Entity::~Entity() {
	SDL_DestroyTexture(this->sprite);
	this->sprite = null;
}

void Entity::render(SDL_Renderer*& renderer) {
	SDL_Rect rect = {xPos, yPos, width, height};

	if (this->sprite == null) {
		SDL_SetRenderDrawColor(renderer, 0xFF, 0, 0, 0xFF);
		SDL_RenderFillRect(renderer, &rect);
	} else {
		SDL_RenderCopy(renderer, this->sprite, null, &rect);
	}
}

void Player::render(SDL_Renderer*& renderer) {
	SDL_Rect dstRect = { xPos, yPos, width, height };
	/// Change this if the player gets another sprite size
	SDL_Rect srcRect = { 0, 0, 40, 40 };
	srcRect.x = dashPossible ? 0 : 41;

	if (this->sprite == null) {
		SDL_SetRenderDrawColor(renderer, 0xFF, 0, 0, 0xFF);
		SDL_RenderFillRect(renderer, &dstRect);
	} else {
		SDL_RenderCopyEx(renderer, this->sprite, &srcRect, &dstRect, this->rotationAngle, null, SDL_FLIP_NONE);
	}
}

Player::Player() {
	this->xPos = 0;
	this->yPos = 0;
	this->width = SIZE;
	this->height = SIZE;
	this->vel = NORMAL_SPEED;
	this->sprite = null;

	this->rotationAngle = 0;
	this->dashMode = false;
	this->dashPossible = true;
}

Player::Player(int x, int y) {
	this->xPos = x;
	this->yPos = y;
	this->rotationAngle = 0;
	this->dashMode = false;
	this->dashPossible = true;

	this->sprite = null;
	this->width = SIZE;
	this->height = SIZE;
	this->vel = NORMAL_SPEED;
}

void Entity::loadSprite(SDL_Renderer*& renderer, const std::string& path) {
	SDL_Surface* tempSurface = IMG_Load(path.c_str());
	if (tempSurface == null) {
		std::cout << "Could not load sprite from " << path << ": " << SDL_GetError() << std::endl;
		SDL_FreeSurface(tempSurface);
		return;
	}

	this->sprite = SDL_CreateTextureFromSurface(renderer, tempSurface);
	if (sprite == null) {
		std::cout << "Could not create tecture for" << path << ": " << SDL_GetError() << std::endl;
	}
	SDL_FreeSurface(tempSurface);
}

void Player::move(const Uint8*& keyStates) {
	this->dash(keyStates[SDL_SCANCODE_SPACE]);

	int inputs[2] = { 0, 0 };

	if (keyStates[SDL_SCANCODE_D] && !dashMode) {
		this->xPos += vel;
		inputs[1] += 1;
	}
	if (xPos + width > SCREEN_WIDTH) {
		xPos = SCREEN_WIDTH - width;
	}
	
	if (keyStates[SDL_SCANCODE_A] && !dashMode) {
		this->xPos -= vel;
		inputs[1] -= 1;
	}
	if (xPos < 0) {
		xPos = 0;
	}
	
	if (keyStates[SDL_SCANCODE_S] && !dashMode) {
		this->yPos += vel;
		inputs[0] += 1;
	}
	if (yPos + height > SCREEN_HEIGHT) {
		yPos = SCREEN_HEIGHT - height;
	}
	
	if (keyStates[SDL_SCANCODE_W] && !dashMode){
		this->yPos -= vel;
		inputs[0] -= 1;
	}
	if (yPos < 0) {
		yPos = 0;
	}

	if (!dashMode) {
		evaluateDirection(inputs, lastInputs, this->lastNonZeroInputs, this->rotationAngle);

		lastInputs[0] = inputs[0];
		lastInputs[1] = inputs[1];
	}
}

void Player::dash(Uint8 spaceState) {
	if (dashFrames > MAX_DASH_FRAMES) {
		dashMode = false;
		vel = NORMAL_SPEED;
		dashFrames = 0;
	}
	if (SDL_GetTicks() - timeSinceLastDash >= DASH_COOLDOWN) {
		this->dashPossible = true;
	}
	if (spaceState != false && dashPossible) {  // Initialiize dash values
		dashMode = true;
		this->dashPossible = false;
		timeSinceLastDash = SDL_GetTicks();
		vel = DASH_SPEED;
	}
	if (dashMode) {
		dashFrames++;
		this->xPos += lastNonZeroInputs[1]*vel;
		this->yPos += lastNonZeroInputs[0]*vel;
	}
}

Walls::Walls(std::set<Directions> walls) {
	this->solidWalls.insert((WallRect){0, 0,
									CORNER_WALL_SIZE, CORNER_WALL_SIZE, 0}); // Upper left corner
	this->solidWalls.insert((WallRect){SCREEN_WIDTH-CORNER_WALL_SIZE, 0,
									CORNER_WALL_SIZE, CORNER_WALL_SIZE, 1}); // Upper right corner
	this->solidWalls.insert((WallRect){0, SCREEN_HEIGHT-CORNER_WALL_SIZE,
									CORNER_WALL_SIZE, CORNER_WALL_SIZE, 2}); // Lower left corner
	this->solidWalls.insert((WallRect){SCREEN_WIDTH-CORNER_WALL_SIZE, SCREEN_HEIGHT-CORNER_WALL_SIZE,
									CORNER_WALL_SIZE, CORNER_WALL_SIZE, 3}); // Lower right corner

	if (walls.find(Directions::UP) != walls.end()) {
		this->solidWalls.insert((WallRect){CORNER_WALL_SIZE, 0,
									 SCREEN_WIDTH-2*CORNER_WALL_SIZE, WALL_WIDTH, 4});
	}
	if (walls.find(Directions::DOWN) != walls.end()) {
		this->solidWalls.insert((WallRect){CORNER_WALL_SIZE, SCREEN_HEIGHT-WALL_WIDTH,
									 SCREEN_WIDTH-2*CORNER_WALL_SIZE, WALL_WIDTH, 5});
	}
	if (walls.find(Directions::LEFT) != walls.end()) {
		this->solidWalls.insert((WallRect){0, CORNER_WALL_SIZE,
									 WALL_WIDTH, SCREEN_HEIGHT-2*CORNER_WALL_SIZE, 6});
	}
	if (walls.find(Directions::RIGHT) != walls.end()) {
		this->solidWalls.insert((WallRect){SCREEN_WIDTH-WALL_WIDTH, CORNER_WALL_SIZE,
		                                   WALL_WIDTH, SCREEN_HEIGHT-2*CORNER_WALL_SIZE, 7});
	}
}

void Walls::render(SDL_Renderer*& renderer) {
	SDL_SetRenderDrawColor(renderer, 0xFF, 0, 0, 0xFF);
	for (auto& rect : this->solidWalls) {
		SDL_RenderFillRect(renderer, &(rect.r));
	}
}

void Walls::manageCollision(Player& ply) {
	bool changed = true;
	int vertiMovement = ply.getLastMovement()[0], horiMovement = ply.getLastMovement()[1];
	int plyLeft, plyRight, plyTop, plyBottom;

	for (auto& wall : this->solidWalls) {
		if (changed) {
			plyLeft = ply.getRect().x, plyRight = ply.getRect().x + ply.getRect().w;
			plyTop = ply.getRect().y, plyBottom = ply.getRect().y + ply.getRect().h;
		}
		int wallLeft = wall.r.x, wallRight = wall.r.x + wall.r.w;
		int wallTop = wall.r.y, wallBottom = wall.r.y + wall.r.h;

		if (plyBottom <= wallTop || plyTop >= wallBottom ||
				plyRight <= wallLeft || plyLeft >= wallRight) {
			changed = false;
		} else {
			changed = true;

			int tempArr[2] = { ply.getRect().x, ply.getRect().y };
			if (plyLeft < wallRight && plyRight > wallRight) {
				tempArr[0] = wall.r.x + wall.r.w;
			}
			if (plyLeft < wallLeft && plyRight > wallLeft) {
				tempArr[0] = wall.r.x - ply.getRect().w;
			}
			if (plyTop < wallBottom && plyBottom > wallBottom) {
				tempArr[1] = wall.r.y + wall.r.h;
			}
			if (plyTop < wallTop && plyBottom > wallTop) {
				tempArr[1] = wall.r.y - ply.getRect().h;
			}
			ply.setPosition(tempArr);
		}
	}
}

void evaluateDirection(const int inputs[2], const int lastInputs[2], int lastNonZeroInputs[2], int& rotation) {
	if (!( inputs[0] == lastInputs[0] && inputs[1] == lastInputs[1] )) { // Input hasn't changed
		return;
	}
	if (inputs[0] == 0 && inputs[1] == 0) { // No input
		return;
	}

	int sum = inputs[0] + inputs[1];
	if (sum == 1 || sum == -1) {
		if (inputs[0] == -1) { // Up
			rotation = 0;
		} else if (inputs[0] == 1) { // Down
			rotation = 180;
		} else if (inputs[1] == -1) { // Left
			rotation = 270;
		} else { // Right
			rotation = 90;
		}
	} else if (sum == 0) {
		if (inputs[0] == -1 && inputs[1] == 1) { // Upright
			rotation = 45;
		} else { // Downleft
			rotation = 225;
		}
	} else {
		if (inputs[0] == 1 && inputs[1] == 1) { // Downright
			rotation = 135;
		} else { // Upleft
			rotation = 315;
		}
	}

	lastNonZeroInputs[0] = inputs[0];
	lastNonZeroInputs[1] = inputs[1];
}

void loadMedia() {
	std::map<std::string, std::string> paths;
	// Insert paths and names for all non-entity based sprites
	paths.insert(std::pair<std::string, std::string>("../Sprites/Background1.png", "BG1"));

	for (auto& value : paths) {
		SDL_Surface* tempSurface = IMG_Load(value.first.c_str());
		if (tempSurface == null) {
			std::cout << "Could not load sprite for " << value.second << ": " << IMG_GetError() << std::endl;
			continue;
		}
		SDL_Texture* tempTexture = SDL_CreateTextureFromSurface(gRenderer, tempSurface);
		if (tempTexture == null) {
			std::cout << "Could not create texture for " << value.second << ": " << SDL_GetError() << std::endl;
			continue;
		}
		gTextures.insert(std::pair<std::string, SDL_Texture*>(value.second, tempTexture));
		SDL_FreeSurface(tempSurface);
	}
}

bool init() {
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
		std::cout << "SDL could not be initialized: " << SDL_GetError() << std::endl;
		return false;
	}
	
	gWindow = SDL_CreateWindow("olc::GameJam 2021", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (gWindow == null) {
		std::cout << "Could not create window: " << SDL_GetError() << std::endl;
		return false;
	}
	
	gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (gRenderer == null) {
		std::cout << "Could not create renderer: " << SDL_GetError() << std::endl;
		return false;
	}
	SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
	
	int imgFlags = IMG_INIT_PNG;
	if (! (IMG_Init(imgFlags) & imgFlags) ) {
		std::cout << "Could not initialize SDL_image: " << IMG_GetError() << std::endl;
		return false;
	}
	
	if (TTF_Init() < 0) {
		std::cout << "Could not initialize SDL_ttf: " << TTF_GetError() << std::endl;
		return false;
	}
	
	return true;
}

void close() {
	for (auto& gTexture : gTextures) {
		SDL_DestroyTexture(gTexture.second);
		gTexture.second = null;
	}

	SDL_DestroyWindow(gWindow);
	SDL_DestroyRenderer(gRenderer);
	gWindow = null;
	gRenderer = null;
	
	SDL_Quit();
	IMG_Quit();
	TTF_Quit();
}