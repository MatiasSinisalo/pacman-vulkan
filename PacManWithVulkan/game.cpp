#include <stdio.h>
#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "globalRenderStructures.h"


class Game {
public: 
	
	std::vector<gameObject> allGameObjects;
	std::vector<int> mazeWallIndexes;
	gameObject *player;
	gameObject* ghost;
	 const float mazeStartPosX = -7.0f;
	 const float mazeStartPosY = -7.0f;
	 const float mazeGridScale = 1.5f;
	void createGameObjects() {

		bool isWall = false;
		for (int j = 0; j < 10; j++) {
			for (int i = 0; i < 10; i++) {
				
				gameObject newGameObject = {};
				newGameObject.pos = glm::vec3(mazeStartPosX + mazeGridScale * i, mazeStartPosY + mazeGridScale * j, 1.0f);
				newGameObject.scale = glm::vec3(mazeGridScale, mazeGridScale, 1.0f);
				sprite newSprite = {};
				newSprite.model = glm::scale(glm::mat4(1.0f), newGameObject.scale);
				glm::vec3 newPos = newGameObject.pos;
				newPos.x *= 1 / newGameObject.scale.x;
				newPos.y *= 1 / newGameObject.scale.y;
				newSprite.model = glm::translate(newSprite.model, newPos);
				if (i == 0 || j == 0 || i == 9 || j == 9) {
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
				isWall = false;
				
			}
		}
	
		
		int textureIndex = 0;
		for (int i = 0; i < 2; i++) {
			gameObject newGameObject = {};
			newGameObject.pos = glm::vec3(0.0f + 5 * i, 0.0f, 1.0f);
			newGameObject.scale = glm::vec3(0.8f, 0.8f, 1.0f);

			sprite newSprite = {};
			newSprite.model = glm::scale(glm::mat4(1.0f), newGameObject.scale);
			glm::vec3 newPos = newGameObject.pos;
			newPos.x *= 1 / newGameObject.scale.x;
			newPos.y *= 1 / newGameObject.scale.y;
			newSprite.model = glm::translate(newSprite.model, newPos);

			if (textureIndex == 0) {
				newSprite.textureIndex = textureIndex;
				textureIndex = 1;
			}
			else {
				newSprite.textureIndex = textureIndex;
				textureIndex = 0;
			}
			newGameObject.drawObject = newSprite;
			
			allGameObjects.push_back(newGameObject);
			
		}
		player = &allGameObjects[allGameObjects.size() - 2];
		ghost = &allGameObjects[allGameObjects.size() - 1];
		
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


	void handleInput(GLFWwindow* window, std::vector<gameObject>& gameObjects, float ellapsed) {
		float ellapsedInSeconds = (ellapsed / pow(10, 6));
		int moveRight = glfwGetKey(window, GLFW_KEY_D);
		glm::vec3 oldpos = player->pos;
		if (moveRight != 0) {
			player->pos.x += 10.0f * ellapsedInSeconds;
		}

		int moveLeft = glfwGetKey(window, GLFW_KEY_A);
		if (moveLeft != 0) {
			player->pos.x -= 10.0f * ellapsedInSeconds;
		}

		int moveUp = glfwGetKey(window, GLFW_KEY_W);
		if (moveUp != 0) {
			player->pos.y -= 10.0f * ellapsedInSeconds;
		}

		int moveDown = glfwGetKey(window, GLFW_KEY_S);
		if (moveDown != 0) {
			player->pos.y += 10.0f * ellapsedInSeconds;
		}

		for (int wall : mazeWallIndexes) {
			if (colliding(*player, allGameObjects[wall])) {
				player->pos = oldpos;
			}	
		}

		glm::vec3 newPos = -(oldpos - player->pos);
		newPos.x *= 1 / player->scale.x;
		newPos.y *= 1 / player->scale.y;
		player->drawObject.model = glm::translate(player->drawObject.model, newPos);
		
	}

	void moveGhost(gameObject* ghost, float ellapsed) {
		float ellapsedInSeconds = (ellapsed / pow(10, 6));
		glm::vec3 oldpos = ghost->pos;
		
		glm::vec3 VectorTowardsPlayer = (player->pos - ghost->pos);
		glm::vec3 unitVectorToPlayer = VectorTowardsPlayer / glm::length(VectorTowardsPlayer);
		ghost->pos = ghost->pos + unitVectorToPlayer * ellapsedInSeconds;

		if (colliding(*player, *ghost)) {
			ghost->pos = oldpos;
		}

		glm::vec3 newPos = -(oldpos - ghost->pos);
		newPos.x *= 1 / ghost->scale.x;
		newPos.y *= 1 / ghost->scale.y;
		ghost->drawObject.model = glm::translate(ghost->drawObject.model, newPos);
	}


	void updatePlayerGridPos() {
		for (gameObject &object : allGameObjects) {
			//int oldIndex = object.drawObject.textureIndex;
			if (pointIsInsideGameObject(*player, object)) {
				object.drawObject.textureIndex = 1;
			}		
		}
		
	}

	void updateGame(GLFWwindow* window, float ellapsed) {
		handleInput(window, allGameObjects, ellapsed);
		updatePlayerGridPos();
		//moveGhost(ghost, ellapsed);
		//std::cout << "Player position: " << player->pos.x << ", " << player->pos.y << "\n";
		
		
	}
};



