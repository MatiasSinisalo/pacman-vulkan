#include <stdio.h>
#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "globalRenderStructures.h"
#include <queue>

#define MAZEWIDTH 22
#define MAZEHEIGHT 27
#include <chrono>
class Game {
public: 
	struct gridCell {
		bool isWall;
		int gameObjectIndex;
	};

	struct gridPos {
		int y;
		int x;
	};

	struct moveTargetLocation {
		glm::vec3 targetLocation;
		bool isValid;
	};

	struct ghost {
		int ghostGameObjectIndex;
		gridPos ghostGridPos;
		gridPos target;
		moveTargetLocation nextTarget;
		bool canMove;
	

	};
	
	std::vector<gameObject> allGameObjects;
	std::vector<ghost> allGhosts;
	std::vector<gridPos> ghostStartingGridPositions;
	std::vector<int> coinIndexes;
	std::vector<int> mazeWallIndexes;
	int powerUpGameobjectIndex;
	std::vector<gridPos> powerUpGridPositions;
	bool powerUpSpawned = false;
	bool powerUpActive = false;
	float powerUpActiveTime = 0.0f;
	
	float timeEllapsedSincePowerUpPickUp;
	float time;
	int playerIndex;
	gridPos playerGridPos;

	gridCell mazeInformation[MAZEHEIGHT][MAZEWIDTH] = {};
	const char mazeLayout[MAZEHEIGHT][MAZEWIDTH] ={"#####################",
									"#X........#........X#",
									"#.###.###.#.###.###.#",
									"#.###.###.#.###.###.#",
									"#.###.###.#.###.###.#",
									"#...................#",
									"#.###.#.#####.#.###.#",
									"#.###.#.#####.#.###.#",
									"#.....#...#...#.....#",
									"#####.###.#.###.#####",
									"#####.#.......#.#####",
									"#####.#.##.##.#.#####",
									"#####.#.#,,,#.#.#####",
									"#.......#####.......#",
									"#####.#.......#.#####", 
									"#####.#.#####.#.#####", 
									"#####.#.#####.#.#####", 
									"#####.#...#...#.#####", 
									"#.........#.........#",
									"#.###..##...##..###.#", 
									"#...#...........#...#", 
									"###.#.#.#####.#.#.###", 
									"###.#.#.#####.#.#.###", 
									"#.....#...#...#.....#", 
									"#.#######.#.#######.#", 
									"#X.................X#", 
									"#####################"};
	
	 const float mazeStartPosX = -7.0f;
	 const float mazeStartPosY = -7.0f;
	 const float mazeGridScale = 0.55f;
	 int points = 0;
	
	void createGameObjects() {

		bool isWall = false;
		//create the maze structure
		for (int j = 0; j < MAZEHEIGHT; j++) {
			for (int i = 0; i < MAZEWIDTH; i++) {
				gameObject newGameObject = {};
				newGameObject.pos = glm::vec3(mazeStartPosX + mazeGridScale * i, mazeStartPosY + mazeGridScale * j, 1.0f);
				newGameObject.scale = glm::vec3(mazeGridScale, mazeGridScale, 1.0f);
				sprite newSprite = {};
				newSprite.model = glm::scale(glm::mat4(1.0f), newGameObject.scale);
				glm::vec3 newPos = newGameObject.pos;
				newPos.x *= 1 / newGameObject.scale.x;
				newPos.y *= 1 / newGameObject.scale.y;
				newSprite.model = glm::translate(newSprite.model, newPos);
				if (mazeLayout[j][i] == '#') {
					newSprite.textureIndex = 2;
					isWall = true;
				}
				else {
					newSprite.textureIndex = 3;
				
				}
				newGameObject.drawObject = newSprite;
				allGameObjects.push_back(newGameObject);
				if (isWall) {
					mazeWallIndexes.push_back(allGameObjects.size() - 1);
					
 				}

				if (mazeLayout[j][i] == ',') {
					gridPos newPos;
					newPos.x = i;
					newPos.y = j;
					ghostStartingGridPositions.push_back(newPos);
				}

				mazeInformation[j][i].isWall = isWall;
				mazeInformation[j][i].gameObjectIndex = allGameObjects.size() - 1;
				isWall = false;
				
			}
		}
		//Save the locations of X for deciding where to spawn the power pill
		for (int j = 0; j < MAZEHEIGHT; j++) {
			for (int i = 0; i < MAZEWIDTH; i++) {
				if (mazeLayout[j][i] == 'X') {
					gridPos pos;
					pos.x = i;
					pos.y = j;
					powerUpGridPositions.push_back(pos);
				}
			}
		}
		//spawn the single power cell off screen for later use
		{
			gameObject newGameObject = {};
			newGameObject.pos.x = 1000.0f;
			newGameObject.pos.y = 1000.0f;
			newGameObject.scale = glm::vec3(0.2f, 0.2f, 1.0f);
			sprite newSprite = {};
			newSprite.model = glm::scale(glm::mat4(1.0f), newGameObject.scale);
			glm::vec3 newPos = newGameObject.pos;
			newPos.x *= 1 / newGameObject.scale.x;
			newPos.y *= 1 / newGameObject.scale.y;
			newSprite.model = glm::translate(newSprite.model, newPos);
			newSprite.textureIndex = 4;
			newGameObject.drawObject = newSprite;
			allGameObjects.push_back(newGameObject);
			powerUpGameobjectIndex = allGameObjects.size() - 1;
		}
		for (int j = 0; j < MAZEHEIGHT; j++) {
			for (int i = 0; i < MAZEWIDTH; i++) {

				if (mazeLayout[j][i] == '.') {
					gameObject newGameObject = {};
					newGameObject.pos = allGameObjects[mazeInformation[j][i].gameObjectIndex].pos;
					newGameObject.scale = glm::vec3(0.1f, 0.1f, 1.0f);
					sprite newSprite = {};
					newSprite.model = glm::scale(glm::mat4(1.0f), newGameObject.scale);
					glm::vec3 newPos = newGameObject.pos;
					newPos.x *= 1 / newGameObject.scale.x;
					newPos.y *= 1 / newGameObject.scale.y;
					newSprite.model = glm::translate(newSprite.model, newPos);
					newSprite.textureIndex = 4;
					newGameObject.drawObject = newSprite;

					allGameObjects.push_back(newGameObject);
					coinIndexes.push_back(allGameObjects.size() - 1);
					
				}
			}
		}



		for (int i = 0; i < 3; i++) {
			int textureIndex = 0;
			gameObject ghostGameObject = {};
			ghostGameObject.pos = glm::vec3(-1.5f, -0.75f, 1.0f);
			ghostGameObject.scale = glm::vec3(0.3f, 0.3f, 1.0f);
			sprite newSprite = {};
			newSprite.model = glm::scale(glm::mat4(1.0f), ghostGameObject.scale);
			glm::vec3 newPos = ghostGameObject.pos;
			newPos.x *= 1 / ghostGameObject.scale.x;
			newPos.y *= 1 / ghostGameObject.scale.y;
			newSprite.model = glm::translate(newSprite.model, newPos);
			newSprite.textureIndex = 1;
			ghostGameObject.drawObject = newSprite;
			allGameObjects.push_back(ghostGameObject);

			ghost newGhost = {};
			newGhost.canMove = false;
		
			newGhost.ghostGameObjectIndex = allGameObjects.size() - 1;
			allGhosts.push_back(newGhost);
		}
		
		

		
		gameObject PlayerGameObject = {};
		PlayerGameObject.pos = glm::vec3(0.2f, 0.0f, 1.0f);
		PlayerGameObject.scale = glm::vec3(0.3f, 0.3f, 1.0f);
		sprite newSprite = {};
		newSprite.model = glm::scale(glm::mat4(1.0f), PlayerGameObject.scale);
		glm::vec3 newPos = PlayerGameObject.pos;
		newPos.x *= 1 / PlayerGameObject.scale.x;
		newPos.y *= 1 / PlayerGameObject.scale.y;
		newSprite.model = glm::translate(newSprite.model, newPos);
		newSprite.textureIndex = 0;
		PlayerGameObject.drawObject = newSprite;

		allGameObjects.push_back(PlayerGameObject);
		playerIndex = allGameObjects.size() - 1;

		
	}

	gameObject* getPlayerObject() {
		gameObject* player = &allGameObjects[playerIndex];
		return player;
	}
	
	bool colliding(gameObject first, gameObject second) {
	//	std::cout << first.pos.x << ", " << first.pos.y << "|";
		//std::cout << second.pos.x << ", " << second.pos.y;
		if ((first.pos.x + 0.5f * first.scale.x) > (second.pos.x - 0.5f * second.scale.x) && (first.pos.x - 0.5f * first.scale.x) < (second.pos.x + 0.5f * second.scale.x) &&
			(first.pos.y + 0.5f * first.scale.y) > (second.pos.y - 0.5f * second.scale.y) && (first.pos.y - 0.5f * first.scale.y) < (second.pos.y + 0.5f * second.scale.y)

			) {
			//std::cout << ", collision! ";
			return true;
		}
		else {
			//std::cout << "no collision!\n";
			
		}
		//std::cout << "\n";
		return false;
	}

	bool pointIsInsideGameObject(gameObject first, gameObject second) {
		if ((first.pos.x) > (second.pos.x - 0.5f * second.scale.x) && (first.pos.x) < (second.pos.x + 0.5f * second.scale.x) &&
			(first.pos.y) > (second.pos.y - 0.5f * second.scale.y) && (first.pos.y) < (second.pos.y + 0.5f * second.scale.y)

			) {
			//std::cout << ", collision! ";
			return true;
		}
		else {
			//std::cout << "no collision!\n";

		}
		//std::cout << "\n";
		return false;

	}

	//TODO: move collisions out of input handling!
	void handleInput(GLFWwindow* window, float ellapsed) {
		float ellapsedInSeconds = (ellapsed / pow(10, 6));
		float velocity = 5.0f;
		int moveRight = glfwGetKey(window, GLFW_KEY_D);
		gameObject* player = getPlayerObject();
		glm::vec3 oldpos = player->pos;
		if (moveRight != 0) {
			player->pos.x += velocity * ellapsedInSeconds;
		}

		int moveLeft = glfwGetKey(window, GLFW_KEY_A);
		if (moveLeft != 0) {
			player->pos.x -= velocity * ellapsedInSeconds;
		}

		int moveUp = glfwGetKey(window, GLFW_KEY_W);
		if (moveUp != 0) {
			player->pos.y -= velocity * ellapsedInSeconds;
		}

		int moveDown = glfwGetKey(window, GLFW_KEY_S);
		if (moveDown != 0) {
			player->pos.y += velocity * ellapsedInSeconds;
		}

		for (int wall : mazeWallIndexes) {
			if (colliding(*player, allGameObjects[wall])) {
				player->pos = oldpos;
			}	
		}
		
		for (ghost &ghost : allGhosts) {
			gameObject* object = &allGameObjects[ghost.ghostGameObjectIndex];
			if (pointIsInsideGameObject(*player, *object) && !powerUpActive) {
				restartGame();
				return;
			}
			if (pointIsInsideGameObject(*player, *object) && powerUpActive) {
				setGameObjectPosition(object, glm::vec3(-1.5f, -0.75f, 1.0f));
			}
		}

		for (int coinIndex : coinIndexes) {
			gameObject *object = &allGameObjects[coinIndex];
			if (colliding(*player, *object))
			{
				points += 1;
				//this is just a placeholder until gameobject destruction is implemented
				setGameObjectPosition(object, glm::vec3(1000.0f, 1000.0f, 1.0f));
			}
		}

		


		glm::vec3 newPos = -(oldpos - player->pos);
		newPos.x *= 1 / player->scale.x;
		newPos.y *= 1 / player->scale.y;
		player->drawObject.model = glm::translate(player->drawObject.model, newPos);
		
	}

	//TODO: move this to ghost class
	void moveGameObjectToPosition(gameObject* object, glm::vec3 pos, float ellapsed, float speed) {
		
		float ellapsedInSeconds = (ellapsed / pow(10, 6));
		glm::vec3 oldpos = object->pos;
		
		glm::vec3 VectorTowardsTarget = (pos - object->pos);
		glm::vec3 unitVectorToTarget = VectorTowardsTarget / glm::length(VectorTowardsTarget);
		object->pos = object->pos + unitVectorToTarget * ellapsedInSeconds * speed;

		

		glm::vec3 moveVector = -(oldpos - object->pos);
		moveVector.x *= 1 / object->scale.x;
		moveVector.y *= 1 / object->scale.y;
		object->drawObject.model = glm::translate(object->drawObject.model, moveVector);
	}

	void setGameObjectPosition(gameObject* object, glm::vec3 newPos) {

			glm::vec3 oldpos = object->pos;
			object->pos = newPos;

			glm::vec3 moveVector = -(oldpos - newPos);
			moveVector.x *= 1 / object->scale.x;
			moveVector.y *= 1 / object->scale.y;
			object->drawObject.model = glm::translate(object->drawObject.model, moveVector);
			
		
	}


	//TODO: move this to ghost class
	glm::vec3 getTargetLocation(gridPos previus[MAZEHEIGHT][MAZEWIDTH], gridPos endPos, ghost ghostObject){
		glm::vec3 pathPoint;
		gridPos currentPos = endPos;
		bool explored[MAZEHEIGHT][MAZEWIDTH] = {};
		while ((currentPos.x == ghostObject.ghostGridPos.x && currentPos.y == ghostObject.ghostGridPos.y) == false) {
			explored[currentPos.y][currentPos.x] = true;
			glm::vec3 gridCellPosition = allGameObjects[mazeInformation[currentPos.y][currentPos.x].gameObjectIndex].pos;
			pathPoint = gridCellPosition;
			currentPos = previus[currentPos.y][currentPos.x];
		}
		return pathPoint;
	}
	//TODO: move this to ghost class
	moveTargetLocation getGhostTarget(gridPos target, ghost &ghostObject) {
		glm::vec3 newTargetLocation;
		std::queue<gridPos> toExplore;
		bool explored[MAZEHEIGHT][MAZEWIDTH] = {};
		gridPos previus[MAZEHEIGHT][MAZEWIDTH] = {};
		toExplore.push(ghostObject.ghostGridPos);

		bool pathFound = false;
		while (toExplore.size() > 0) {
			gridPos currentPos = toExplore.front();
			toExplore.pop();
			explored[currentPos.y][currentPos.x] = true;
			
			if (currentPos.x + 1 < MAZEWIDTH) {
				gridPos newPosition;
				newPosition.x = currentPos.x + 1;
				newPosition.y = currentPos.y;
				if (!explored[newPosition.y][newPosition.x] && !mazeInformation[newPosition.y][newPosition.x].isWall)
				{
					
					previus[newPosition.y][newPosition.x].y = currentPos.y;
					previus[newPosition.y][newPosition.x].x = currentPos.x;
					if (newPosition.x == target.x && newPosition.y == target.y) {
						pathFound = true;
						break;
					}
					toExplore.push(newPosition);
				}
			}
			if (currentPos.x - 1 > -1) {
				gridPos newPosition;
				newPosition.x = currentPos.x - 1;
				newPosition.y = currentPos.y;
				if (!explored[newPosition.y][newPosition.x] && !mazeInformation[newPosition.y][newPosition.x].isWall)
				{
					previus[newPosition.y][newPosition.x].y = currentPos.y;
					previus[newPosition.y][newPosition.x].x = currentPos.x;
					if (newPosition.x == target.x && newPosition.y == target.y) {
						pathFound = true;
						break;
					}
					toExplore.push(newPosition);
				}
			}

			if (currentPos.y + 1 < MAZEHEIGHT) {
				gridPos newPosition;
				newPosition.x = currentPos.x;
				newPosition.y = currentPos.y + 1;
				if (!explored[newPosition.y][newPosition.x] && !mazeInformation[newPosition.y][newPosition.x].isWall)
				{
					previus[newPosition.y][newPosition.x].y = currentPos.y;
					previus[newPosition.y][newPosition.x].x = currentPos.x;
					if (newPosition.x == target.x && newPosition.y == target.y) {
						pathFound = true;
						break;
					}
					toExplore.push(newPosition);
				}
			}
			if (currentPos.y - 1 > -1) {
				gridPos newPosition;
				newPosition.x = currentPos.x;
				newPosition.y = currentPos.y - 1;
				if (!explored[newPosition.y][newPosition.x] && !mazeInformation[newPosition.y][newPosition.x].isWall)
				{
					previus[newPosition.y][newPosition.x].y = currentPos.y;
					previus[newPosition.y][newPosition.x].x = currentPos.x;
					if (newPosition.x == target.x && newPosition.y == target.y) {
						pathFound = true;
						break;
					}
					toExplore.push(newPosition);
				}
			}

		}

		moveTargetLocation newTarget = {};
		newTarget.isValid = false;
		if (pathFound)
		{
			newTarget.isValid = true;
			newTarget.targetLocation = getTargetLocation(previus, target, ghostObject);
		}

		return newTarget;
	}
	
	//TODO: update player and ghost gridPos seem to have duplicate code, make these into one function?
	void updatePlayerGridPos() {
		gameObject* player = getPlayerObject();
		for(int j = 0; j < MAZEHEIGHT; j++ ){
			int i = 0;
			for (gridCell &cell : mazeInformation[j]) {
				if (player != &allGameObjects[cell.gameObjectIndex] && pointIsInsideGameObject(*player, allGameObjects[cell.gameObjectIndex])) {
					playerGridPos.y = j;
					playerGridPos.x = i;
					
				
				}
				
				i++;
			}
		}
		
	}
	void updateGhostGridPositions() {
		for (ghost &ghostObject : allGhosts) {
			for (int j = 0; j < MAZEHEIGHT; j++) {
				int i = 0;
				for (gridCell& cell : mazeInformation[j]) {
					if (pointIsInsideGameObject(allGameObjects[ghostObject.ghostGameObjectIndex], allGameObjects[cell.gameObjectIndex])) {
						ghostObject.ghostGridPos.y = j;
						ghostObject.ghostGridPos.x = i;
						
					}
					else
					{
						

					}
					i++;
				}

			}
		}
		

	}
	
	//TODO: move this to ghost class
	void buildAllGhostPaths() {

		allGhosts[0].target = playerGridPos;
		allGhosts[1].target = playerGridPos;
		allGhosts[2].target = playerGridPos;
		
		if (playerGridPos.y + 3 < MAZEHEIGHT) {
			if (!mazeInformation[playerGridPos.y + 3][playerGridPos.x].isWall) {
				allGhosts[1].target.x = playerGridPos.x;
				allGhosts[1].target.y = playerGridPos.y + 3;
			}
		}
		if (playerGridPos.x + 3 < MAZEWIDTH) {
			if (!mazeInformation[playerGridPos.y][playerGridPos.x + 3].isWall) {
				allGhosts[2].target.x = playerGridPos.x + 3;
				allGhosts[2].target.y = playerGridPos.y;
			}
		}
		
		if (powerUpActive)
		{
			allGhosts[0].target = ghostStartingGridPositions[0];
			allGhosts[1].target = ghostStartingGridPositions[1];
			allGhosts[2].target = ghostStartingGridPositions[2];
		}


		for (ghost &ghostObject : allGhosts) {
			
			ghostObject.nextTarget = getGhostTarget(ghostObject.target, ghostObject);
			//if a ghost doesnt find a suitable path to target, we try again to find a path straight to the player
			if (!ghostObject.nextTarget.isValid) {
				ghostObject.nextTarget = getGhostTarget(playerGridPos, ghostObject);
			}
		}
	}
	//TODO: move this to ghost class
	void moveAllGhosts(float ellapsed) {
		for (ghost &ghostObject : allGhosts) {
			


			if (ghostObject.canMove) {
				/*
				* this is for debugging 
				for (glm::vec3 pos : ghostObject.ghostPath) {
					gameObject newGameObject = {};
					newGameObject.pos = pos;
					newGameObject.scale = glm::vec3(0.1f, 0.1f, 1.0f);
					sprite newSprite = {};
					newSprite.model = glm::scale(glm::mat4(1.0f), newGameObject.scale);
					glm::vec3 newPos = newGameObject.pos;
					newPos.x *= 1 / newGameObject.scale.x;
					newPos.y *= 1 / newGameObject.scale.y;
					newSprite.model = glm::translate(newSprite.model, newPos);
					newSprite.textureIndex = 4;
					newGameObject.drawObject = newSprite;

					allGameObjects.push_back(newGameObject);
				}
				*/
				gameObject* player = getPlayerObject();
				if (ghostObject.nextTarget.isValid) {
					moveGameObjectToPosition(&allGameObjects[ghostObject.ghostGameObjectIndex], ghostObject.nextTarget.targetLocation, ellapsed, 1.5f);
					
				}
				else {
					moveGameObjectToPosition(&allGameObjects[ghostObject.ghostGameObjectIndex], player->pos, ellapsed, 1.5f);
				}
			}

		}
	}

	void updateGame(GLFWwindow* window, float ellapsed) {
		float ellapsedInSeconds = (ellapsed / pow(10, 6));
		time += ellapsedInSeconds;
		
		handleInput(window, ellapsed);
		updatePlayerGridPos();
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		updateGhostGridPositions();
		buildAllGhostPaths();
		moveAllGhosts(ellapsed);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		auto ghostsEllapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
		std::cout << "ghost time: " << ghostsEllapsed << "\n";
		if (time > 1.0f) {
			allGhosts[0].canMove = true;
		}

		if (time > 3.0f) {
			allGhosts[1].canMove = true;
		}
		if (time > 5.0f) {
			allGhosts[2].canMove = true;
		}

		if (!powerUpSpawned) {
			timeEllapsedSincePowerUpPickUp += ellapsedInSeconds;

			if (timeEllapsedSincePowerUpPickUp > 20.0f) {
				int randomPositionIndex = rand() % powerUpGridPositions.size();
				int powerUpGridId = mazeInformation[powerUpGridPositions[randomPositionIndex].y][powerUpGridPositions[randomPositionIndex].x].gameObjectIndex;
				setGameObjectPosition(&allGameObjects[powerUpGameobjectIndex], allGameObjects[powerUpGridId].pos);
				timeEllapsedSincePowerUpPickUp = 0.0f;
				powerUpSpawned = true;
			}
		}
		
		gameObject* player = getPlayerObject();
		if (colliding(*player, allGameObjects[powerUpGameobjectIndex])) {
			setGameObjectPosition(&allGameObjects[powerUpGameobjectIndex], glm::vec3(1000.0f, 1000.0f, 1.0f));
			powerUpSpawned = false;
			powerUpActive = true;
		}
		
		if (powerUpActive)
		{
			powerUpActiveTime += ellapsedInSeconds;
			if (powerUpActiveTime > 10.0f) {
				powerUpActiveTime = 0.0f;
				powerUpActive = false;
			}

		}

		
		//std::cout << "Player position: " << player->pos.x << ", " << player->pos.y << "  |  " << "Points: " << points << "\n";
		
		
	}
	
	void restartGame() {
		allGameObjects.clear();
		allGhosts.clear();
		coinIndexes.clear();
		mazeWallIndexes.clear();
		powerUpGridPositions.clear();
		powerUpSpawned = false;
		powerUpActiveTime = 0.0f;
		powerUpActive = false;
		createGameObjects();
		time = 0.0f;
	}
	
};



