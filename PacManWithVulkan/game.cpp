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
	gameObject *player;
	void createGameObjects() {

		
		for (int j = 0; j < 10; j++) {
			for (int i = 0; i < 10; i++) {


				if (i == 0 || j == 0 || i == 9 || j == 9) {
					gameObject newGameObject = {};
					newGameObject.pos = glm::vec3(-7.0f + 1.5 * i, -7.0f + 1.5 * j, 1.0f);
					newGameObject.scale = glm::vec3(1.5f, 1.5f, 1.0f);

					sprite newSprite = {};
					newSprite.model = glm::scale(glm::mat4(1.0f), newGameObject.scale);
					glm::vec3 newPos = newGameObject.pos;
					newPos.x *= 1 / newGameObject.scale.x;
					newPos.y *= 1 / newGameObject.scale.y;
					newSprite.model = glm::translate(newSprite.model, newPos);
					newSprite.textureIndex = 2;
					newGameObject.drawObject = newSprite;
					allGameObjects.push_back(newGameObject);
				}
			}
		}
	
		
		int textureIndex = 0;
		for (int i = 0; i < 2; i++) {
			gameObject newGameObject = {};
			newGameObject.pos = glm::vec3(0.0f + i, 0.0f, 1.0f);
			newGameObject.scale = glm::vec3(0.5f, 0.5f, 1.0f);

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

		
	}
	
	bool colliding(gameObject first, gameObject second) {
		std::cout << first.pos.x << ", " << first.pos.y << "|";
		std::cout << second.pos.x << ", " << second.pos.y;
		if ((first.pos.x + 0.5f * first.scale.x) > (second.pos.x - 0.5f * second.scale.x) && (first.pos.x - 0.5f * first.scale.x) < (second.pos.x + 0.5f * second.scale.x) &&
			(first.pos.y + 0.5f * first.scale.y) > (second.pos.y - 0.5f * second.scale.y) && (first.pos.y - 0.5f * first.scale.y) < (second.pos.y + 0.5f * second.scale.y)

			) {
			std::cout << ", collision! ";
			return true;
		}
		else {
			//std::cout << "no collision!\n";
			
		}
		std::cout << "\n";
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

		for (gameObject &object : gameObjects) {
			if (player != &object) {
				if (colliding(*player, object)) {
					player->pos = oldpos;

				}
			}			
		}

		glm::vec3 newPos = -(oldpos - player->pos);
		newPos.x *= 1 / player->scale.x;
		newPos.y *= 1 / player->scale.y;
		player->drawObject.model = glm::translate(player->drawObject.model, newPos);
		
	}
};



