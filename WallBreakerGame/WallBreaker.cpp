#include "WallBreaker.h"
#include "raylib.h"
#include "raymath.h"
#include <iostream>
#include <cmath>


int WallBreaker::CircleToRoundRectangleCollision(Vector2 circle_centre, float radius, Rectangle rect)
{
	if ((rect.x > circle_centre.x) && (rect.y > circle_centre.y)) //top left
	{ return 1; }
	else if ((rect.x + rect.width < circle_centre.x) && (rect.y > circle_centre.y)) //top right
	{ return 2; }
	else if ((rect.x > circle_centre.x) && (rect.y + rect.height < circle_centre.y)) //bot left
	{ return 3; }
	else if ((rect.x + rect.width < circle_centre.x) && (rect.y + rect.height < circle_centre.y)) //bot left
	{ return 4; }
	else if ((rect.x <= circle_centre.x) && (rect.x + rect.width >= circle_centre.x)) // top or bot
	{ return 5; }
	else if ((rect.y <= circle_centre.y) && (rect.y + rect.height >= circle_centre.y)) // left or right
	{ return 6; }
}

void WallBreaker::Main()
{
	InitWindow(screenWidth, screenHeight, "Wall Breaker");
	InitAudioDevice();

	Start();

	SetTargetFPS(144); // This will become important
	while (!WindowShouldClose())
	{
		Update();
	}
	UnloadSound(brick_sfx);
	CloseWindow();
}

void WallBreaker::Start()
{
#pragma region BRICKS
	float spaceForBricks = screenWidth - (BRICKS_PER_ROW * GAP + GAP);
	Vector2 brickSize = Vector2{ spaceForBricks / BRICKS_PER_ROW, BRICK_HEIGHT };

	bricks.clear();

	for (int row = 1; row < ROWS_OF_BRICKS; row++) {  }

	for (int row = 0; row < ROWS_OF_BRICKS; row++)
	{
		for (int col = 0; col < BRICKS_PER_ROW; col++)
		{
			if ((GetRandomValue(1, 100) % (100 / 50)) == 0) { // procedural generation via poking holes randomly, 50 -> 50%
				continue;
			}

			float x = GAP + (GAP + brickSize.x) * col;
			float y = GAP + (GAP + brickSize.y) * row;

			Rectangle rect = Rectangle{ x, y, brickSize.x, brickSize.y };

			Brick brick = Brick{ colors[row], rect };

			bricks.push_back(brick);
		}
	}
#pragma endregion 

#pragma region PLAYER
	player.position = Vector2{ screenWidth / 2, screenHeight * 9 / 10 };
	player.size = Vector2{ screenWidth / 10 , 20 };
	player.curLife = MAX_LIVES; // at the beginning
#pragma endregion

	powerUps.clear();
	ball.position = Vector2{ screenWidth / 2, screenHeight * 9 / 10 - 30 };
	ball.active = false;
	ball.power_up_timer = 0;
	ball.charged = false;

	brick_sfx = LoadSound("resources/Brick.wav");
}

void WallBreaker::EvalCurFrame()
{
	if (gameOver)
	{
		if (IsKeyPressed(KEY_ENTER))
		{
			Start();
			gameOver = false;
		}

		return;
	}

	if (levelWon)
	{
		if (IsKeyPressed(KEY_ENTER))
		{
			Start();
			levelWon = false;
		}

		return;
	}

	if (IsKeyPressed(KEY_P))
		gamePaused = !gamePaused;

	if (gamePaused) return;


	if (!ball.active)
	{
		ball.position = Vector2{ player.position.x, screenHeight * 9 / 10 - 30 };

		if (IsKeyPressed(KEY_SPACE))
		{
			ball.active = true;
			if (IsKeyDown(KEY_LEFT))
			{
				ball.speed = Vector2{ -100, -250 };
				//std::cout << "case: " << "b" << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
			}
			else if (IsKeyDown(KEY_RIGHT))
			{
				ball.speed = Vector2{ 100, -250 };
				//std::cout << "case: " << "b" << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
			}
			else
			{
				ball.speed = Vector2{ 0, -250 };
				//std::cout << "case: " << "b" << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
			}
		}
	}
	else
	{
		ball.position.x += ball.speed.x * GetFrameTime();
		ball.position.y += ball.speed.y * GetFrameTime();
	}

	for (int i = 0; i < powerUps.size(); i++)
	{
		powerUps[i].position.x += powerUps[i].speed.x * GetFrameTime();
		powerUps[i].position.y += powerUps[i].speed.y * GetFrameTime();
	}

	// player position
	if (IsKeyDown(KEY_LEFT))
		player.position.x -= 250 * GetFrameTime();

	// we have reached the far left
	if (player.position.x - player.size.x / 2 <= 0)
		player.position.x = player.size.x / 2; // stick it to the far left

	// we have reached the far right
	if (player.position.x + player.size.x / 2 >= screenWidth)
		player.position.x = screenWidth - player.size.x / 2; // stick it to the far right

	if (IsKeyDown(KEY_RIGHT))
		player.position.x += 250 * GetFrameTime();

	// power up timer
	if (ball.charged) {
		ball.power_up_timer -= GetFrameTime();
		char* timer_text = new char[2];
		timer_text[0] = (static_cast<int>(ball.power_up_timer) + 1) + '0';
		timer_text[1] = 0;
		DrawText(timer_text,
			GetScreenWidth() / 2 - MeasureText("5", 30) / 2,
			GetScreenHeight() / 2 - 15,
			30, GRAY);
		if (ball.power_up_timer <= 0) {
			ball.charged = false;
		}
	}

	// Collision with the Bricks
	for (int i = 0; i < bricks.size(); i++)
	{
		if (CheckCollisionCircleRec(ball.position, ball.radius, bricks[i].rect))
		{
			PlaySound(brick_sfx);

			if ((GetRandomValue(1, 100) % (100 / 10)) == 0) { // random chance of getting power up when breaking a brick , 10 -> 10% chance
				if ((GetRandomValue(1, 100) % (100 / 50)) == 0) { // random power up , 50 -> 50% chance
					PowerUp health = PowerUp{ Vector2{ bricks[i].rect.x + bricks[i].rect.width / 2, bricks[i].rect.y + bricks[i].rect.height / 2 } };
					health.power_type = "health";
					health.power_value = 1;
					health.color = GREEN;
					powerUps.push_back(health);
				}
				else {
					PowerUp charged = PowerUp{ Vector2{ bricks[i].rect.x + bricks[i].rect.width / 2, bricks[i].rect.y + bricks[i].rect.height / 2 } };
					charged.power_type = "charged";
					charged.power_value = 5;
					charged.color = YELLOW;
					powerUps.push_back(charged);
				}
			}

			switch (CircleToRoundRectangleCollision(ball.position, ball.radius, bricks[i].rect)) {
			case 1:
				if (!ball.charged) {
					if ((ball.speed.x > 0) && (ball.speed.y > 0))
					{
						std::swap(ball.speed.x, ball.speed.y);
					}
					ball.speed.x = std::abs(ball.speed.x) * -1;
					ball.speed.y = std::abs(ball.speed.y) * -1;
					//std::cout << "case: " << 1 << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
				}
				break;
			case 2:
				if (!ball.charged) {
					if ((ball.speed.x > 0) && (ball.speed.y > 0))
					{
						std::swap(ball.speed.x, ball.speed.y);
					}
					ball.speed.x = std::abs(ball.speed.x) * 1;
					ball.speed.y = std::abs(ball.speed.y) * -1;
					//std::cout << "case: " << 2 << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
				}
				break;
			case 3:
				if (!ball.charged) {
					if ((ball.speed.x > 0) && (ball.speed.y < 0))
					{
						std::swap(ball.speed.x, ball.speed.y);
					}
					ball.speed.x = std::abs(ball.speed.x) * -1;
					ball.speed.y = std::abs(ball.speed.y) * 1;
					//std::cout << "case: " << 3 << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
				}
				break;
			case 4:
				if (!ball.charged) {
					if ((ball.speed.x < 0) && (ball.speed.y < 0))
					{
						std::swap(ball.speed.x, ball.speed.y);
					}
					ball.speed.x = std::abs(ball.speed.x) * 1;
					ball.speed.y = std::abs(ball.speed.y) * 1;
					//std::cout << "case: " << 4 << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
				}
				break;
			case 5:
				if (!ball.charged) {
					ball.speed.y *= -1;
					//std::cout << "case: " << 5 << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
				}
				break;
			case 6:
				if (!ball.charged) {
					ball.speed.x *= -1;
					//std::cout << "case: " << 6 << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
				}
				break;
			}


			// delete the brick
			bricks.erase(bricks.begin() + i);


			//std::cout << "case: " << "c" << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
			break; // Because the ball might hit two bricks at the same frame		
		}
	}

	// ball collision with the pedal
	if (CheckCollisionCircleRec(ball.position, ball.radius, player.GetRect()))
	{
		if (ball.speed.y > 0) // we are going downwards
		{
			ball.speed.x = (ball.position.x - player.position.x) / (player.size.x / 500);
			ball.speed.y *= -1;
			//std::cout << "case: " << "p" << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
		}
	}

	// powerUp collision with pedal
	for (int i = 0; i < powerUps.size(); i++)
	{
		if (CheckCollisionCircleRec(powerUps[i].position, powerUps[i].radius, player.GetRect())) {
			// apply effect
			if (powerUps[i].power_type == "health" && player.curLife < MAX_LIVES) {
				player.curLife++;
			}
			else if (powerUps[i].power_type == "charged") {
				ball.charged = true;
				ball.power_up_timer = powerUps[i].power_value;
			}

			// delete powerUp
			powerUps.erase(powerUps.begin() + i);
		}
	}

	// collision with the walls
	if ((ball.position.x + ball.radius >= screenWidth) || ball.position.x - ball.radius <= 0)
	{
		ball.speed.x *= -1;
		//std::cout << "case: " << "s" << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
	}
	if (ball.position.y - ball.radius <= 0) // we hit the top
	{
		ball.speed.y *= -1;
		//std::cout << "case: " << "t" << " speed: " << ball.speed.x << ", " << ball.speed.y << std::endl; // debug tool
	}
	if (ball.position.y + ball.radius >= screenHeight)
	{
		player.curLife--;
		ball.active = false;
		ball.speed = Vector2{ 0 , 0 };
	}
	for (int i = 0; i < powerUps.size(); i++)
	{
		if (powerUps[i].position.y + powerUps[i].radius >= screenHeight)
		{
			// delete powerUp
			powerUps.erase(powerUps.begin() + i);
		}
	}


	if (player.curLife == 0)
		gameOver = true;
	else
	{
		if (bricks.size() == 0)
			levelWon = true;
	}


}


void WallBreaker::DrawCurFrame()
{
	BeginDrawing();
	ClearBackground(BLACK);

	if (gameOver)
	{
		DrawText("Press Enter to play again",
			GetScreenWidth() / 2 - MeasureText("Press Enter to play again", 30) / 2,
			GetScreenHeight() / 2 - 15,
			30, GRAY);
	}
	else if (levelWon)
	{
		DrawText("You Won! Press Enter to go to next level!",
			GetScreenWidth() / 2 - MeasureText("You Won! Press Enter to go to next level!", 30) / 2,
			GetScreenHeight() / 2 - 15,
			30, GRAY);
	}
	else
	{
		player.Draw();
		ball.Draw();

		for (Brick b : bricks)
			b.Draw();

		for (PowerUp p : powerUps)
			p.Draw();

		// draw player lives!
		for (int i = 0; i < MAX_LIVES; i++)
		{
			if (i < player.curLife)
				DrawRectangle(10 + 40 * i, screenHeight - 20, 30, 10, LIGHTGRAY);
			else
				DrawRectangle(10 + 40 * i, screenHeight - 20, 30, 10, GRAY);
		}
	}





	EndDrawing();
}

void WallBreaker::Update()
{
	EvalCurFrame();
	DrawCurFrame();
}

