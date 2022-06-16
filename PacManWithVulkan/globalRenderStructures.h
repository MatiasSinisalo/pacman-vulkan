#if not defined INCLUDING_GLM
	#define INCLUDING_GLM TRUE
	#include <glm/glm.hpp>
	#include <glm/gtc/matrix_transform.hpp>
#endif

struct sprite {
	glm::mat4 model;
	int textureIndex;
};

struct gameObject {
	glm::vec3 pos;
	glm::vec3 scale;
	sprite drawObject;
};

