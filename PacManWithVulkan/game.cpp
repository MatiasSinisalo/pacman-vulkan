#include <stdio.h>
#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "globalRenderStructures.h"
#define MAZEWIDTH 1
#define MAZEHEIGHT 1


class Game {
public: 
	std::vector<gameObject> allGameObjects;
	
	void createGameObjects() {
		int textureIndex = 0;
		for (int i = 0; i < 2; i++) {
			gameObject newGameObject = {};
			newGameObject.pos = glm::vec3(0.0f + i, 0.0f, 1.0f);
			newGameObject.scale = glm::vec3(1.5f, 1.5f, 1.0f);

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
	
	}
	
	void colliding(gameObject first, gameObject second) {
		std::cout << first.pos.x << ", " << first.pos.y << "|";
		std::cout << second.pos.x << ", " << second.pos.y;
		if ((first.pos.x + 0.5f * first.scale.x) > (second.pos.x - 0.5f * second.scale.x) && (first.pos.x - 0.5f * first.scale.x) < (second.pos.x + 0.5f * second.scale.x) &&
			(first.pos.y + 0.5f * first.scale.y) > (second.pos.y - 0.5f * second.scale.y) && (first.pos.y - 0.5f * first.scale.y) < (second.pos.y + 0.5f * second.scale.y)

			) {
			std::cout << ", collision! ";

		}
		else {
			//std::cout << "no collision!\n";
		}
		std::cout << "\n";
	}

	void handleInput(GLFWwindow* window, std::vector<gameObject>& gameObjects, float ellapsed) {
		float ellapsedInSeconds = (ellapsed / pow(10, 6));
		int moveRight = glfwGetKey(window, GLFW_KEY_D);
		glm::vec3 oldpos = gameObjects[0].pos;
		if (moveRight != 0) {
			gameObjects[0].pos.x += 10.0f * ellapsedInSeconds;
		}

		int moveLeft = glfwGetKey(window, GLFW_KEY_A);
		if (moveLeft != 0) {
			gameObjects[0].pos.x -= 10.0f * ellapsedInSeconds;
		}

		int moveUp = glfwGetKey(window, GLFW_KEY_W);
		if (moveUp != 0) {
			gameObjects[0].pos.y -= 10.0f * ellapsedInSeconds;
		}

		int moveDown = glfwGetKey(window, GLFW_KEY_S);
		if (moveDown != 0) {
			gameObjects[0].pos.y += 10.0f * ellapsedInSeconds;
		}
		glm::vec3 newPos = -(oldpos - gameObjects[0].pos);
		newPos.x *= 1 / gameObjects[0].scale.x;
		newPos.y *= 1 / gameObjects[0].scale.y;
		gameObjects[0].drawObject.model = glm::translate(gameObjects[0].drawObject.model, newPos);
		colliding(gameObjects[0], gameObjects[1]);
	}
};



