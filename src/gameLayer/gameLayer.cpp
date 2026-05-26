#define RAUDIO_STANDALONE
#define GLM_ENABLE_EXPERIMENTAL
#include "gameLayer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include "platformInput.h"
#include <iostream>
#include <gl2d/gl2d.h>
#include <platformTools.h>
#include <tiledRenderer.h>
#include <bullet.h>
#include <vector>
#include <enemy.h>
#include <cstdio>
#include <ctime>
#include <glui/glui.h>
//#include <raudio.h>

enum class GameScreen
{
	MainMenu,
	LevelSelect,
	Playing,
	GameOver,
};

enum class GameLevel
{
	Level1 = 1,
	Level2 = 2,
};

struct GameplayData
{
	glm::vec2 playerPos = { 100, 100 };
	std::vector<Bullet> bullets;
	std::vector<Enemy> enemies;
	float health = 1.f;
	float spawnEnemyTimerSecconds = 3;

	GameScreen screen = GameScreen::MainMenu;
	GameLevel level = GameLevel::Level1;
	int menuIndex = 0;
};

GameplayData data;
gl2d::Renderer2D renderer;
constexpr int BACKGROUNDS = 4;
gl2d::Texture spaceShipsTexture;
gl2d::TextureAtlasPadding spaceShipsAtlas;
gl2d::Texture bulletsTexture;
gl2d::TextureAtlasPadding bulletsAtlas;
gl2d::Texture backgroundTexture[BACKGROUNDS];
TiledRenderer tiledRenderer[BACKGROUNDS];
gl2d::Texture healthBar;
gl2d::Texture health;
gl2d::Font menuFont;
//Sound shootSound;

constexpr float shipSize = 250.f;
constexpr float LEVEL2_DIFFICULTY = 1.75f;

// Edge-trigger cho dieu huong menu (tranh chuyen muc qua nhanh)
static bool g_upPrev = false;
static bool g_downPrev = false;

bool isUpJustPressed()
{
	bool held = platform::isButtonHeld(platform::Button::Up) ||
		platform::isButtonHeld(platform::Button::W);
	bool just = held && !g_upPrev;
	g_upPrev = held;
	return just;
}

bool isDownJustPressed()
{
	bool held = platform::isButtonHeld(platform::Button::Down) ||
		platform::isButtonHeld(platform::Button::S);
	bool just = held && !g_downPrev;
	g_downPrev = held;
	return just;
}

bool menuConfirmPressed()
{
	return platform::isLMousePressed();
}

bool intersectBullet(glm::vec2 bulletPos, glm::vec2 shipPos, float shipSize)
{
	return glm::distance(bulletPos, shipPos) <= shipSize;
}

float getLevelDifficultyMultiplier()
{
	return data.level == GameLevel::Level2 ? LEVEL2_DIFFICULTY : 1.f;
}

void renderBackgrounds()
{
	for (int i = 0; i < BACKGROUNDS; i++)
		tiledRenderer[i].render(renderer);
}

void renderMenuTextInBox(const glm::vec4& box, const char* text, gl2d::Color4f color, float maxSize = 2.5f)
{
	if (!text || !text[0] || menuFont.texture.id == 0)
		return;

	glm::vec2 center(box.x + box.z * 0.5f, box.y + box.w * 0.5f);
	float size = renderer.determineTextRescaleFitSmaller(text, menuFont, box, maxSize);
	renderer.renderText(center, text, menuFont, color, size, 3.f, 2.f, true);
}

void renderMenuUi(int w, int h, int itemCount, const char* const labels[], const char* title)
{
	renderer.pushCamera();
	{
		glui::Frame f({ 0, 0, w, h });

		if (title && title[0])
		{
			glui::Box titleBox = glui::Box()
				.xLeftPerc(0.22f).yTopPerc(0.08f)
				.xDimensionPercentage(0.56f).yAspectRatio(1.f / 14.f);
			glm::vec4 titleRect = titleBox;
			renderMenuTextInBox(titleRect, title, Colors_White, 3.f);
		}

		glui::Box panel = glui::Box()
			.xLeftPerc(0.22f).yTopPerc(0.18f)
			.xDimensionPercentage(0.56f).yAspectRatio(0.55f);
		renderer.renderRectangle(panel, healthBar);

		for (int i = 0; i < itemCount; i++)
		{
			glui::Box row = glui::Box()
				.xLeftPerc(0.28f).yTopPerc(0.32f + i * 0.11f)
				.xDimensionPercentage(0.44f).yAspectRatio(1.f / 12.f);

			bool selected = (i == data.menuIndex);
			renderer.renderRectangle(row, selected ? health : healthBar);

			if (labels && labels[i])
			{
				glm::vec4 rowRect = row;
				renderMenuTextInBox(rowRect, labels[i],
					selected ? Colors_White : Colors_Gray, 2.2f);
			}
		}

		glui::Box hintBox = glui::Box()
			.xLeftPerc(0.22f).yTopPerc(0.88f)
			.xDimensionPercentage(0.56f).yAspectRatio(1.f / 28.f);
		renderMenuTextInBox(hintBox, "W/S: chon  |  Click: xac nhan", Colors_Gray, 1.4f);
	}
	renderer.popCamera();
}

bool menuNavigate(int itemCount)
{
	if (isUpJustPressed())
	{
		data.menuIndex = (data.menuIndex + itemCount - 1) % itemCount;
		return true;
	}
	if (isDownJustPressed())
	{
		data.menuIndex = (data.menuIndex + 1) % itemCount;
		return true;
	}
	return false;
}

void startNewRun()
{
	GameLevel level = data.level;
	data = {};
	data.level = level;
	data.screen = GameScreen::Playing;
	data.health = 1.f;
	data.spawnEnemyTimerSecconds = 3;
	data.playerPos = { 100, 100 };

	renderer.currentCamera.follow(data.playerPos,
		550, 0, 0, renderer.windowW, renderer.windowH);
}

void goToMainMenu()
{
	GameLevel level = data.level;
	data = {};
	data.level = level;
	data.screen = GameScreen::MainMenu;
	data.menuIndex = 0;
	data.playerPos = { 100, 100 };

	renderer.currentCamera.follow(data.playerPos,
		550, 0, 0, renderer.windowW, renderer.windowH);
}

bool initGame()
{
	std::srand((unsigned)std::time(0));
	gl2d::init();
	renderer.create();

	spaceShipsTexture.loadFromFileWithPixelPadding
	(RESOURCES_PATH "spaceShip/stitchedFiles/spaceships.png", 128, true);
	spaceShipsAtlas = gl2d::TextureAtlasPadding(5, 2,
		spaceShipsTexture.GetSize().x, spaceShipsTexture.GetSize().y);

	bulletsTexture.loadFromFileWithPixelPadding
	(RESOURCES_PATH "spaceShip/stitchedFiles/projectiles.png", 500, true);
	bulletsAtlas = gl2d::TextureAtlasPadding(3, 2,
		bulletsTexture.GetSize().x, bulletsTexture.GetSize().y);

	healthBar.loadFromFile(RESOURCES_PATH "healthBar.png", true);
	health.loadFromFile(RESOURCES_PATH "health.png", true);

	menuFont.createFromFile(RESOURCES_PATH "CommodorePixeled.ttf");

	backgroundTexture[0].loadFromFile(RESOURCES_PATH "background1.png", true);
	backgroundTexture[1].loadFromFile(RESOURCES_PATH "background2.png", true);
	backgroundTexture[2].loadFromFile(RESOURCES_PATH "background3.png", true);
	backgroundTexture[3].loadFromFile(RESOURCES_PATH "background4.png", true);

	tiledRenderer[0].texture = backgroundTexture[0];
	tiledRenderer[1].texture = backgroundTexture[1];
	tiledRenderer[2].texture = backgroundTexture[2];
	tiledRenderer[3].texture = backgroundTexture[3];

	tiledRenderer[0].paralaxStrength = 0;
	tiledRenderer[1].paralaxStrength = 0.2;
	tiledRenderer[2].paralaxStrength = 0.4;
	tiledRenderer[3].paralaxStrength = 0.7;

	goToMainMenu();
	return true;
}

void spawnEnemy()
{
	glm::uvec2 shipTypes[] = { {0,0}, {0,1}, {2,0}, {3,1} };
	const float diff = getLevelDifficultyMultiplier();

	Enemy e;
	e.position = data.playerPos;

	glm::vec2 offset(2000, 0);
	offset = glm::vec2(glm::vec4(offset, 0, 1) * glm::rotate(glm::mat4(1.f),
		glm::radians((float)(rand() % 360)), glm::vec3(0, 0, 1)));

	e.position += offset;
	e.speed = (800 + rand() % 1000) * diff;
	e.turnSpeed = (2.2f + (rand() % 1000) / 500.f) * diff;
	e.type = shipTypes[rand() % 4];
	e.fireRange = 1.5 + (rand() % 1000) / 2000.f;
	e.fireTimeReset = 0.1 + (rand() % 1000) / 500;
	e.bulletSpeed = (rand() % 3000 + 1000) * diff;

	data.enemies.push_back(e);
}

// Menu chinh: 0=Bat dau, 1=Chon level, 2=Thoat
bool updateMainMenu(int w, int h)
{
	static const char* labels[] = { "Bat dau", "Chon level", "Thoat" };

	renderBackgrounds();
	renderMenuUi(w, h, 3, labels, "SPACE GAME");
	menuNavigate(3);

	if (!menuConfirmPressed())
		return true;

	switch (data.menuIndex)
	{
	case 0:
		startNewRun();
		break;
	case 1:
		data.screen = GameScreen::LevelSelect;
		data.menuIndex = 0;
		break;
	case 2:
		return false;
	default:
		break;
	}
	return true;
}

// Chon level: 0=Level1, 1=Level2 (enemy nhanh hon), 2=Quay lai
bool updateLevelSelect(int w, int h)
{
	static const char* labels[] = { "Level 1", "Level 2 (kho)", "Quay lai" };

	renderBackgrounds();
	renderMenuUi(w, h, 3, labels, "CHON LEVEL");
	menuNavigate(3);

	if (!menuConfirmPressed())
		return true;

	switch (data.menuIndex)
	{
	case 0:
		data.level = GameLevel::Level1;
		data.screen = GameScreen::MainMenu;
		data.menuIndex = 0;
		break;
	case 1:
		data.level = GameLevel::Level2;
		data.screen = GameScreen::MainMenu;
		data.menuIndex = 0;
		break;
	case 2:
		data.screen = GameScreen::MainMenu;
		data.menuIndex = 0;
		break;
	default:
		break;
	}
	return true;
}

// Game over: 0=Choi tiep, 1=Ve menu
bool updateGameOver(int w, int h)
{
	static const char* labels[] = { "Choi tiep", "Ve menu" };

	renderBackgrounds();
	renderMenuUi(w, h, 2, labels, "GAME OVER");
	menuNavigate(2);

	if (!menuConfirmPressed())
		return true;

	if (data.menuIndex == 0)
		startNewRun();
	else
		goToMainMenu();

	return true;
}

bool updatePlaying(float deltaTime, int w, int h)
{
	glm::vec2 move = {};
	if (platform::isButtonHeld(platform::Button::W) ||
		platform::isButtonHeld(platform::Button::Up))    move.y = -1;
	if (platform::isButtonHeld(platform::Button::S) ||
		platform::isButtonHeld(platform::Button::Down))  move.y = 1;
	if (platform::isButtonHeld(platform::Button::A) ||
		platform::isButtonHeld(platform::Button::Left))  move.x = -1;
	if (platform::isButtonHeld(platform::Button::D) ||
		platform::isButtonHeld(platform::Button::Right)) move.x = 1;

	if (move.x != 0 || move.y != 0)
	{
		move = glm::normalize(move);
		move *= deltaTime * 2000;
		data.playerPos += move;
	}

	renderer.currentCamera.follow(data.playerPos,
		deltaTime * 550, 1, 150, w, h);
	renderer.currentCamera.zoom = 0.5;

	renderBackgrounds();

	glm::vec2 mousePos = platform::getRelMousePosition();
	glm::vec2 screenCenter(w / 2.f, h / 2.f);
	glm::vec2 mouseDirection = mousePos - screenCenter;
	if (glm::length(mouseDirection) == 0.f) mouseDirection = { 1,0 };
	else mouseDirection = normalize(mouseDirection);

	if (platform::isLMousePressed())
	{
		Bullet b;
		b.position = data.playerPos;
		b.fireDirection = mouseDirection;
		data.bullets.push_back(b);
	}

	for (int i = 0; i < (int)data.bullets.size(); i++)
	{
		if (glm::distance(data.bullets[i].position, data.playerPos) > 5000)
		{
			data.bullets.erase(data.bullets.begin() + i);
			i--; continue;
		}

		if (!data.bullets[i].isEnemy)
		{
			bool breakBothLoops = false;
			for (int e = 0; e < (int)data.enemies.size(); e++)
			{
				if (intersectBullet(data.bullets[i].position,
					data.enemies[e].position, enemyShipSize))
				{
					data.enemies[e].life -= 0.1;
					if (data.enemies[e].life <= 0)
						data.enemies.erase(data.enemies.begin() + e);

					data.bullets.erase(data.bullets.begin() + i);
					i--;
					breakBothLoops = true;
					break;
				}
			}
			if (breakBothLoops) continue;
		}
		else
		{
			if (intersectBullet(data.bullets[i].position, data.playerPos, shipSize))
			{
				data.health -= 0.1;
				data.bullets.erase(data.bullets.begin() + i);
				i--; continue;
			}
		}

		data.bullets[i].update(deltaTime);
	}

	if (data.health <= 0)
	{
		data.screen = GameScreen::GameOver;
		data.menuIndex = 0;
		data.bullets.clear();
		data.enemies.clear();
		return true;
	}

	data.health += deltaTime * 0.05;
	data.health = glm::clamp(data.health, 0.f, 1.f);

	if (data.enemies.size() < 15)
	{
		data.spawnEnemyTimerSecconds -= deltaTime;
		if (data.spawnEnemyTimerSecconds < 0)
		{
			data.spawnEnemyTimerSecconds = rand() % 6 + 1;
			spawnEnemy();
			if (rand() % 3 == 0)
			{
				spawnEnemy();
				spawnEnemy();
			}
		}
	}

	for (int i = 0; i < (int)data.enemies.size(); i++)
	{
		if (glm::distance(data.playerPos, data.enemies[i].position) > 4000.f)
		{
			data.enemies.erase(data.enemies.begin() + i);
			i--; continue;
		}

		if (data.enemies[i].update(deltaTime, data.playerPos))
		{
			Bullet b;
			b.position = data.enemies[i].position;
			b.fireDirection = data.enemies[i].viewDirection;
			b.speed = data.enemies[i].bulletSpeed;
			b.isEnemy = true;
			data.bullets.push_back(b);
		}
	}

	for (auto& e : data.enemies)
		e.render(renderer, spaceShipsTexture, spaceShipsAtlas);

	renderSpaceShip(renderer, data.playerPos, shipSize,
		spaceShipsTexture, spaceShipsAtlas.get(3, 0), mouseDirection);

	for (auto& b : data.bullets)
		b.render(renderer, bulletsTexture, bulletsAtlas);

	renderer.pushCamera();
	{
		glui::Frame f({ 0, 0, w, h });

		glui::Box healthBox = glui::Box().xLeftPerc(0.65).yTopPerc(0.1)
			.xDimensionPercentage(0.3).yAspectRatio(1.f / 8.f);
		renderer.renderRectangle(healthBox, healthBar);

		glm::vec4 newRect = healthBox();
		newRect.z *= data.health;
		glm::vec4 textCoords = { 0, 1, 1, 0 };
		textCoords.z *= data.health;
		renderer.renderRectangle(newRect, health, Colors_White, {}, {}, textCoords);
	}
	renderer.popCamera();

	return true;
}

bool gameLogic(float deltaTime)
{
	int w = platform::getFrameBufferSizeX();
	int h = platform::getFrameBufferSizeY();

	glViewport(0, 0, w, h);
	glClear(GL_COLOR_BUFFER_BIT);
	renderer.updateWindowMetrics(w, h);

	bool keepRunning = true;

	switch (data.screen)
	{
	case GameScreen::MainMenu:
		keepRunning = updateMainMenu(w, h);
		break;
	case GameScreen::LevelSelect:
		keepRunning = updateLevelSelect(w, h);
		break;
	case GameScreen::GameOver:
		keepRunning = updateGameOver(w, h);
		break;
	case GameScreen::Playing:
		keepRunning = updatePlaying(deltaTime, w, h);
		break;
	default:
		break;
	}

	renderer.flush();
	return keepRunning;
}

void closeGame()
{
	menuFont.cleanup();
}
